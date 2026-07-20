/** @file test_twin.c — walk-up-unlock digital twin.
 *
 * Drives the whole flow deterministically, no hardware: bind an Aliro URSK,
 * stand up the real Pre-POLL listener, and run a full DS-TWR round from
 * genuinely CCM*-encrypted frames + injected radio timestamps, exactly as the
 * silicon path would. Where test_prepoll_round.c stops at the internal range
 * store, this asserts the *facade unlock seam* the lock's open-decision uses
 * (woz_uwb_last_range_cm / woz_uwb_trusted_range_cm), and then drives the
 * range-integrity gate through the same store seam the RX path feeds, to check
 * the two security properties the distance boundary rests on:
 *   - an early-first-path / Ghost-Peak spoof cannot reduce the reported distance
 *     or flip the unlock bit (the true-reject that is otherwise bench-gated);
 *   - a lone honest block cannot open — trust needs K agreeing blocks.
 *
 * The frame/timestamp machinery mirrors test_prepoll_round.c so the legit round
 * is a real decode, not a stub.
 */
#include <string.h>

#include <deca_device_api.h>

#include "aliro_kdf.h" /* ALIRO_URSK_LEN */
#include "ccc_kdf.h"
#include "ccc_mac.h"
#include "ccc_shim.h"
#include "fira_session.h"
#include "woz_uwb_facade.h"
#include "test.h"

/* The CCC STS substitution wrap — callable directly on the host (no ld --wrap). */
extern int32_t __wrap_dwt_rxenable(int32_t mode);

#define TW_SID    0x51a7c0deu
#define TW_STS0   0x00400000u
#define TW_IDX1   5000u /* first Pre-POLL's Poll_STS_Index */
#define TW_STRIDE 96u   /* per-block index stride the decode must learn */
#define TW_BLOCK  7u

/* Session crypto constants, derived in setup exactly as prepoll_decode does. */
static uint8_t g_ursk[ALIRO_URSK_LEN];
static uint8_t g_mupsk1[CCC_MUPSK1_LEN];
static uint8_t g_ks[CCC_KEYSOURCE_LEN];
static uint8_t g_dest[CCC_DEST_SHORT_ADDR_LEN];
static uint8_t g_src_long[CCC_SRC_LONG_ADDR_LEN];

/** Build an encrypted Pre-POLL frame; returns its on-air length. */
static uint16_t mk_prepoll(uint8_t *out, uint32_t fc, uint32_t poll_idx)
{
	struct ccc_mhr_fields f;
	struct ccc_pre_poll pp;
	uint8_t plain[CCC_PRE_POLL_LEN];

	memset(&pp, 0, sizeof(pp));
	pp.uwb_session_id = TW_SID;
	pp.poll_sts_index = poll_idx;
	pp.ranging_block = TW_BLOCK;
	ccc_pre_poll_pack(&pp, plain);

	memset(&f, 0, sizeof(f));
	f.dest_short_addr = (uint16_t)(g_dest[0] | ((uint16_t)g_dest[1] << 8));
	f.frame_counter = fc;
	memcpy(f.key_source, g_ks, CCC_KEYSOURCE_LEN);
	f.msg_id = CCC_MSG_ID_PRE_POLL;
	f.payload_len = CCC_PRE_POLL_LEN;
	ccc_build_mhr(&f, out);
	ccc_sp0_encrypt(g_mupsk1, g_src_long, fc, out, CCC_MHR_LEN, plain, CCC_PRE_POLL_LEN,
			&out[CCC_MHR_LEN], &out[CCC_MHR_LEN + CCC_PRE_POLL_LEN]);
	return CCC_MHR_LEN + CCC_PRE_POLL_LEN + CCC_SP0_MIC_LEN;
}

/** Build an encrypted Final_Data (1 responder) keyed on the armed POLL index. */
static uint16_t mk_final_data(uint8_t *out, uint32_t fc, uint32_t armed_idx, uint32_t t_round1,
			      uint32_t t_reply2)
{
	struct ccc_mhr_fields f;
	struct ccc_final_data fd;
	uint8_t plain[64];
	uint8_t dudsk[CCC_DUDSK_LEN];
	size_t pl = 0;

	memset(&fd, 0, sizeof(fd));
	fd.uwb_session_id = TW_SID;
	fd.ranging_block = TW_BLOCK;
	fd.final_sts_index = armed_idx + 2u;
	fd.ranging_ts_final_tx = t_round1 + t_reply2; /* t5-t1 */
	fd.num_responders = 1u;
	fd.responders[0].timestamp = t_round1; /* t4-t1 */
	ccc_final_data_pack(&fd, plain, sizeof(plain), &pl);

	memset(&f, 0, sizeof(f));
	f.dest_short_addr = (uint16_t)(g_dest[0] | ((uint16_t)g_dest[1] << 8));
	f.frame_counter = fc;
	memcpy(f.key_source, g_ks, CCC_KEYSOURCE_LEN);
	f.msg_id = CCC_MSG_ID_FINAL_DATA;
	f.payload_len = (uint8_t)pl;
	ccc_build_mhr(&f, out);
	ccc_shim_dudsk_for_index(armed_idx, dudsk);
	ccc_sp0_encrypt(dudsk, g_src_long, fc, out, CCC_MHR_LEN, plain, pl, &out[CCC_MHR_LEN],
			&out[CCC_MHR_LEN + pl]);
	return (uint16_t)(CCC_MHR_LEN + pl + CCC_SP0_MIC_LEN);
}

/** Load a frame + Ipatov timestamp into the stub, then feed it to try_prepoll. */
static void stash_frame(const uint8_t *frame, uint16_t len, uint64_t ip40)
{
	memcpy(woz_host_rx.rxdata, frame, len);
	woz_host_rx.rxdata_len = len;
	woz_host_rx.rx_ts40 = ip40;
}

/** Fire a captured RX/TX callback the way dwt_isr would. */
static void rx_event(dwt_cb_t cb, uint32_t status)
{
	dwt_cb_data_t d;

	memset(&d, 0, sizeof(d));
	d.status = status;
	d.datalength = woz_host_rx.rxdata_len;
	cb(&d);
}

/* Good-frame status: CIA done (timestamp valid) + PHR + CRC good. */
#define ST_GOOD (DWT_INT_CIADONE_BIT_MASK | DWT_INT_RXPHD_BIT_MASK | DWT_INT_RXFCG_BIT_MASK)

void test_twin(void)
{
	uint8_t frame[128];
	uint16_t len;
	uint8_t rc[17];
	struct woz_uwb_aliro_cfg c;
	uint8_t mupsk2[CCC_MUPSK2_LEN], uad[CCC_UAD_LEN];
	const uint32_t widx = TW_IDX1 + 2u * TW_STRIDE; /* warmed POLL index */
	uint32_t fc = 200u;
	int32_t cm = -1;

	t_group("provision + bring up the responder");
	for (size_t i = 0; i < sizeof(g_ursk); i++) {
		g_ursk[i] = (uint8_t)(0x5Au + i);
	}
	for (size_t i = 0; i < sizeof(rc); i++) {
		rc[i] = (uint8_t)(0x20u + i);
	}
	ccc_derive_mupsk1(g_ursk, g_mupsk1);
	ccc_derive_mupsk2(g_ursk, mupsk2);
	ccc_derive_uad(mupsk2, TW_STS0, uad);
	ccc_uad_addresses(uad, g_ks, g_dest, g_src_long);

	memset(&c, 0, sizeof(c));
	c.session_id = TW_SID;
	c.channel = 9u;
	c.sync_code_index = 9u;
	c.slot_per_round = 12u;
	c.sts_index0 = TW_STS0;
	c.ursk = g_ursk;
	c.ranging_config = rc;
	c.rc_len = sizeof(rc);
	woz_host_rx_reset();
	T_EQ("start", woz_uwb_start_aliro(&c), 0);
	T_EQ("start.armed", woz_host_rx.rxenable_calls, 1);
	__wrap_dwt_rxenable(DWT_START_RX_IMMEDIATE); /* program a key while bound */
	/* The range store is a process-global singleton shared across suites; clear
	 * its trust run (an implausible value resets trust without latching) so this
	 * twin's trust assertions stand on their own regardless of suite order. */
	fira_session_set_ccc_range_cm(-9999, 0u);

	t_group("legit approach ranges through the full DS-TWR pipeline");
	/* Bootstrap: two Pre-POLL decodes learn the index and its stride. */
	len = mk_prepoll(frame, fc++, TW_IDX1);
	stash_frame(frame, len, 0x1000000ull);
	ccc_shim_rx_try_prepoll(len);
	len = mk_prepoll(frame, fc++, TW_IDX1 + TW_STRIDE);
	stash_frame(frame, len, 0x2000000ull);
	ccc_shim_rx_try_prepoll(len);
	/* Pre-POLL event arms the SP3 POLL window off the warm. */
	stash_frame(frame, len, 0x3000000ull);
	rx_event(woz_host_rx.cbs.cbRxOk, ST_GOOD);
	T_OK("arm.awaiting_poll", ccc_shim_rx_awaiting_poll());
	/* Next block's Pre-POLL arrives while armed: stash + defer its decode. */
	len = mk_prepoll(frame, fc++, TW_IDX1 + 2u * TW_STRIDE);
	stash_frame(frame, len, 0x3100000ull);
	ccc_shim_rx_try_prepoll(len);
	/* POLL result (cper=0) fires the delayed Response TX. */
	woz_host_rx.rx_ts40 = 0x40000000ull; /* t2: POLL RX */
	rx_event(woz_host_rx.cbs.cbRxOk, DWT_INT_CIADONE_BIT_MASK);
	T_EQ("poll.resp_tx", woz_host_rx.starttx_calls, 1);
	/* TXFRS arms the Final window. */
	woz_host_rx.tx_ts40 = 0x40000000ull + 100000u; /* t3 = t2 + 100k DTU */
	rx_event(woz_host_rx.cbs.cbTxDone, DWT_INT_TXFRS_BIT_MASK);
	/* Final result: good STS, valid timestamp, revert to SP0. */
	woz_host_rx.rx_ts40 = 0x40000000ull + 300000u; /* t6 = t3 + 200k DTU */
	woz_host_rx.stsq_ret = 0;
	woz_host_rx.stsq_val = 100;
	rx_event(woz_host_rx.cbs.cbRxOk, DWT_INT_CIADONE_BIT_MASK);
	/* Final_Data decrypt latches the DS-TWR range: round1=101k, reply2=199k,
	 * with reply1=100k, round2=200k injected => tof 500 ticks = 234 cm. */
	len = mk_final_data(frame, fc++, widx, 101000u, 199000u);
	stash_frame(frame, len, 0x3200000ull);
	ccc_shim_rx_try_prepoll(len);

	/* The unlock seam (facade), not the internal store, is what the lock reads. */
	T_OK("range.seen", woz_uwb_last_range_cm(&cm));
	T_EQ("range.cm", cm, 234);
	T_OK("range.plausible", fira_session_range_plausible(cm));
	/* One honest block is not yet trustworthy: the unlock accessor stays shut. */
	T_OK("one_block.not_trusted", !woz_uwb_trusted_range_cm(&cm));

	t_group("Ghost-Peak spoof cannot reduce distance or open the lock");
	/* An early-first-path injection drives the DS-TWR ToF out of the plausible
	 * band; layer 1 must drop it so the reported distance is NOT reduced and the
	 * unlock bit is NOT set. Fed through the store seam the RX path uses. */
	fira_session_set_ccc_range_cm(-400, 90u);
	T_OK("spoof.range_not_reduced", woz_uwb_last_range_cm(&cm) && cm == 234);
	T_OK("spoof.not_trusted", !woz_uwb_trusted_range_cm(&cm));

	t_group("only a sustained honest approach earns the unlock bit");
	/* The spoof reset trust to 0; K consecutive agreeing plausible blocks (as a
	 * real walk-up produces) are required before the facade surfaces a distance. */
	fira_session_set_ccc_range_cm(120, 91u);
	T_OK("earn.one", !woz_uwb_trusted_range_cm(&cm));
	fira_session_set_ccc_range_cm(122, 92u);
	T_OK("earn.two", !woz_uwb_trusted_range_cm(&cm));
	fira_session_set_ccc_range_cm(121, 93u);
	T_OK("earn.k_reached", woz_uwb_trusted_range_cm(&cm));
	T_EQ("earn.distance", cm, 121);

	woz_uwb_stop();
}
