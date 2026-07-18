/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 *
 * aliro_reader — the Aliro reader credential-auth transaction (Phase 3.2/3.3) on
 * top of the aliro_ble L2CAP transport. Drives AUTH0 -> AUTH1 -> EXCHANGE, runs
 * the ECDH + key schedule (aliro_crypto) to derive the URSK, then hands it to the
 * UWB engine (woz_uwb_start_aliro). Wire codec = aliro_apdu; crypto = aliro_crypto.
 *
 * Heavy diagnostic logging by design: this path can only complete end-to-end
 * once the reader is provisioned (Phase 4: real reader identity + issuer trust)
 * and a real credential is present. Until then it exercises + logs each step.
 * The reader identity here is a dev placeholder generated at start.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"

#include "aliro_ble.h"
#include "aliro_apdu.h"
#include "aliro_crypto.h"
#include "aliro_prim.h"
#include "woz_uwb_facade.h"
#include "aliro_reader.h"

static const char *TAG = "aliro_reader";

/* PROVISIONAL advertised BLE-UWB protocol version. Real value is the provisioned
 * Matter attribute (Phase 4); 0x0100 is the baseline the arbiter treats specially. */
static const uint16_t k_proto_versions[] = { 0x0100u };
#define ALIRO_VERSION 0x0100u

/* Dev reader identity (Phase 4 replaces with a provisioned identity + issuer
 * trust anchors). Generated once at reader start. */
static uint8_t s_reader_id[32];
static uint8_t s_reader_sign_priv[32];
static bool s_provisioned;

/* Canned ranging parameters for the 3.3 handoff. The real parameters are
 * negotiated in the M1-M4 exchange (post-auth, over BleSK) which is a later
 * increment; until then the derived URSK is bound to these demo params. */
static const struct {
	uint32_t session_id;
	uint8_t channel;
	uint8_t sync_code_index;
	uint16_t slot_duration_rstu;
	uint32_t block_duration_ms;
	uint8_t slot_per_round;
	uint32_t sts_index0;
} k_ranging = { 0x02b02fd4u, 9u, 9u, 2400u, 192u, 12u, 0x1196e79du };

enum txn_phase {
	PH_IDLE = 0,     /* connected; awaiting the peer's first message */
	PH_SENT_AUTH0,   /* AUTH0 sent; awaiting AUTH0Response */
	PH_SENT_AUTH1,   /* AUTH1 sent; awaiting AUTH1Response */
	PH_ESTABLISHED,  /* URSK derived, UWB armed */
	PH_FAILED,
};

static const char *phase_str(enum txn_phase p)
{
	switch (p) {
	case PH_IDLE:        return "IDLE";
	case PH_SENT_AUTH0:  return "SENT_AUTH0";
	case PH_SENT_AUTH1:  return "SENT_AUTH1";
	case PH_ESTABLISHED: return "ESTABLISHED";
	case PH_FAILED:      return "FAILED";
	default:             return "?";
	}
}

#define ALIRO_MAX_SESSIONS 2

static struct aliro_session {
	bool active;
	uint16_t conn_handle;
	enum txn_phase phase;
	uint32_t msgs_rx;

	uint8_t reader_eph_priv[ALIRO_P256_SCALAR];
	uint8_t reader_eph_pub[ALIRO_P256_POINT];
	uint8_t txid[ALIRO_TXID_LEN];
	uint8_t device_eph_pub[ALIRO_P256_POINT];
	uint8_t z[32];
	struct aliro_secchan sc;
	uint8_t ursk[ALIRO_URSK_LEN];
} s_sessions[ALIRO_MAX_SESSIONS];

static struct aliro_session *session_find(uint16_t conn_handle)
{
	for (int i = 0; i < ALIRO_MAX_SESSIONS; i++) {
		if (s_sessions[i].active && s_sessions[i].conn_handle == conn_handle) {
			return &s_sessions[i];
		}
	}
	return NULL;
}

static struct aliro_session *session_alloc(uint16_t conn_handle)
{
	for (int i = 0; i < ALIRO_MAX_SESSIONS; i++) {
		if (!s_sessions[i].active) {
			memset(&s_sessions[i], 0, sizeof(s_sessions[i]));
			s_sessions[i].active = true;
			s_sessions[i].conn_handle = conn_handle;
			s_sessions[i].phase = PH_IDLE;
			return &s_sessions[i];
		}
	}
	return NULL;
}

static int provision_dev_identity(void)
{
	uint8_t pub[ALIRO_P256_POINT];

	if (s_provisioned) {
		return 0;
	}
	if (aliro_ec_p256_keygen(s_reader_sign_priv, pub) != 0) {
		return -1;
	}
	/* Dev placeholder: reader identifier = signing public-key X coordinate.
	 * Phase 4 supplies the real provisioned reader identifier. */
	memcpy(s_reader_id, pub + 1, 32);
	s_provisioned = true;
	return 0;
}

static int send_framed(uint16_t conn, uint8_t opcode, const uint8_t *payload, size_t len)
{
	uint8_t frame[600];
	size_t flen;

	if (aliro_ble_frame(0x00, opcode, payload, len, frame, sizeof(frame), &flen) != 0) {
		ESP_LOGE(TAG, "[conn %u] frame build failed (op 0x%02x len %u)", conn,
			 opcode, (unsigned)len);
		return -1;
	}
	int rc = aliro_ble_send(conn, frame, flen);

	ESP_LOGI(TAG, "[conn %u] TX op 0x%02x, %u payload bytes (send rc=%d)", conn,
		 opcode, (unsigned)len, rc);
	return rc;
}

/* Phase 3.3: bind the derived URSK and start the UWB responder. */
static void arm_uwb_from_credential(struct aliro_session *s)
{
	ESP_LOGI(TAG, "[conn %u] URSK derived; arming UWB responder", s->conn_handle);
	ESP_LOG_BUFFER_HEXDUMP(TAG, s->ursk, ALIRO_URSK_LEN, ESP_LOG_INFO);

	struct woz_uwb_aliro_cfg cfg = {
		.session_id = k_ranging.session_id,
		.channel = k_ranging.channel,
		.sync_code_index = k_ranging.sync_code_index,
		.slot_duration_rstu = k_ranging.slot_duration_rstu,
		.block_duration_ms = k_ranging.block_duration_ms,
		.slot_per_round = k_ranging.slot_per_round,
		.sts_index0 = k_ranging.sts_index0,
		.uwb_time_us = 0u,
		.ursk = s->ursk,
		.ranging_config = NULL,
		.rc_len = 0,
	};

	/* Re-point the engine at the credential-derived URSK. Ranging params are
	 * canned until the M1-M4 negotiation is parsed. */
	woz_uwb_stop();
	int rc = woz_uwb_start_aliro(&cfg);

	ESP_LOGI(TAG, "[conn %u] woz_uwb_start_aliro(derived URSK) = %d %s",
		 s->conn_handle, rc, rc == 0 ? "(ranging armed)" : "(FAILED)");
}

/* Kick the reader-driven access protocol: ephemeral keys + txid -> AUTH0. */
static void start_auth(struct aliro_session *s)
{
	if (aliro_ec_p256_keygen(s->reader_eph_priv, s->reader_eph_pub) != 0 ||
	    aliro_random(s->txid, sizeof(s->txid)) != 0) {
		ESP_LOGE(TAG, "[conn %u] ephemeral keygen/txid failed", s->conn_handle);
		s->phase = PH_FAILED;
		return;
	}

	uint8_t apdu[160];
	size_t n;

	/* ExpeditedPhaseType/UserAuthenticationPolicy: standard path, no extra
	 * policy (enum values are provisioning/policy driven; 0x02/0x00 here). */
	if (aliro_apdu_build_auth0(0x02u, 0x00u, ALIRO_VERSION, s->reader_eph_pub,
				   s->txid, s_reader_id, apdu, sizeof(apdu), &n) != 0) {
		ESP_LOGE(TAG, "[conn %u] AUTH0 build failed", s->conn_handle);
		s->phase = PH_FAILED;
		return;
	}
	send_framed(s->conn_handle, ALIRO_OP_AUTH0, apdu, n);
	s->phase = PH_SENT_AUTH0;
}

static void on_auth0_response(struct aliro_session *s, const uint8_t *pl, size_t len)
{
	struct aliro_auth0_response r;

	if (aliro_apdu_parse_auth0_response(pl, len, &r) != 0) {
		ESP_LOGE(TAG, "[conn %u] AUTH0Response parse failed", s->conn_handle);
		s->phase = PH_FAILED;
		return;
	}
	memcpy(s->device_eph_pub, r.device_eph_pub, ALIRO_P256_POINT);

	uint8_t shared[ALIRO_SHARED_SECRET_LEN];

	if (aliro_ecdh_p256(s->reader_eph_priv, s->device_eph_pub, shared) != 0) {
		ESP_LOGE(TAG, "[conn %u] ECDH failed", s->conn_handle);
		s->phase = PH_FAILED;
		return;
	}
	aliro_crypto_derive_z(shared, s->txid, s->z);

	/* Sign the reader-usage transcript (device pubX, reader-eph pubX). */
	uint8_t td[160], sig[ALIRO_P256_SIG], apdu[128];
	size_t tn, n;

	if (aliro_apdu_build_authdata(ALIRO_AUTH_READER, s_reader_id,
				      s->device_eph_pub + 1, s->reader_eph_pub + 1,
				      s->txid, td, sizeof(td), &tn) != 0 ||
	    aliro_ecdsa_p256_sign(s_reader_sign_priv, td, tn, sig) != 0) {
		ESP_LOGE(TAG, "[conn %u] reader signature failed", s->conn_handle);
		s->phase = PH_FAILED;
		return;
	}
	if (aliro_apdu_build_auth1(0x01u, sig, apdu, sizeof(apdu), &n) != 0) {
		s->phase = PH_FAILED;
		return;
	}
	send_framed(s->conn_handle, ALIRO_OP_AUTH1, apdu, n);
	s->phase = PH_SENT_AUTH1;
}

static void on_auth1_response(struct aliro_session *s, const uint8_t *pl, size_t len)
{
	struct aliro_auth1_response r;

	if (aliro_apdu_parse_auth1_response(pl, len, &r) != 0) {
		ESP_LOGE(TAG, "[conn %u] AUTH1Response parse failed", s->conn_handle);
		s->phase = PH_FAILED;
		return;
	}

	/* Verify the device signature over the device-usage transcript. Trust of
	 * the presenting credential key itself is a Phase-4 issuer check. */
	const uint8_t *cred_pub = r.have_device_pub ? r.device_pub : s->device_eph_pub;
	uint8_t td[160];
	size_t tn;

	if (aliro_apdu_build_authdata(ALIRO_AUTH_DEVICE, s_reader_id,
				      s->device_eph_pub + 1, s->reader_eph_pub + 1,
				      s->txid, td, sizeof(td), &tn) != 0) {
		s->phase = PH_FAILED;
		return;
	}
	if (aliro_ecdsa_p256_verify(cred_pub, td, tn, r.device_sig) != 0) {
		ESP_LOGW(TAG, "[conn %u] device signature INVALID (expected until a "
			      "provisioned credential is present)", s->conn_handle);
		s->phase = PH_FAILED;
		return;
	}
	ESP_LOGI(TAG, "[conn %u] device signature OK", s->conn_handle);

	/* Establish the secure channel: HKDF key block -> session keys + URSK. */
	uint8_t salt[ALIRO_SALT_MAX], block[ALIRO_KEY_BLOCK_LEN];
	uint8_t enc[ALIRO_SESSION_KEY_LEN], dec[ALIRO_SESSION_KEY_LEN];
	size_t slen;

	if (aliro_salt_build(ALIRO_SALT_SESSION, s->txid, s->device_eph_pub + 1,
			     s->reader_eph_pub + 1, s_reader_id, ALIRO_VERSION, 0x02u,
			     0x00u, NULL, salt, &slen) != 0 ||
	    aliro_crypto_derive_block(s->z, salt, slen, s->device_eph_pub + 1, block) != 0) {
		ESP_LOGE(TAG, "[conn %u] key-block derivation failed", s->conn_handle);
		s->phase = PH_FAILED;
		return;
	}
	aliro_crypto_split(block, 1, enc, dec, s->ursk);
	aliro_secchan_init(&s->sc, enc, dec);

	/* EXCHANGE: seal the URSK-ready trigger and frame it. */
	uint8_t ex[16], ct[16], tag[ALIRO_GCM_TAG_LEN], payload[32];
	size_t exn;

	if (aliro_apdu_build_exchange(0, 0, 1, ex, sizeof(ex), &exn) != 0 ||
	    aliro_secchan_seal(&s->sc, NULL, 0, ex, exn, ct, tag) != 0) {
		ESP_LOGE(TAG, "[conn %u] EXCHANGE seal failed", s->conn_handle);
		s->phase = PH_FAILED;
		return;
	}
	memcpy(payload, ct, exn);
	memcpy(payload + exn, tag, ALIRO_GCM_TAG_LEN);
	send_framed(s->conn_handle, ALIRO_OP_EXCHANGE, payload, exn + ALIRO_GCM_TAG_LEN);

	s->phase = PH_ESTABLISHED;
	arm_uwb_from_credential(s);
}

/* Consume one inbound Aliro transaction SDU. */
static void transaction_feed(struct aliro_session *s, const uint8_t *data, uint16_t len)
{
	s->msgs_rx++;

	uint8_t type, opcode;
	const uint8_t *pl;
	size_t pl_len;

	if (aliro_ble_unframe(data, len, &type, &opcode, &pl, &pl_len) != 0) {
		ESP_LOGW(TAG, "[conn %u] msg #%u (%u B): not a valid envelope",
			 s->conn_handle, (unsigned)s->msgs_rx, (unsigned)len);
		ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);
		pl = data;
		pl_len = len;
		type = 0xff;
		opcode = 0xff;
	}
	ESP_LOGI(TAG, "[conn %u] msg #%u: type=0x%02x op=0x%02x, %u payload B, phase=%s",
		 s->conn_handle, (unsigned)s->msgs_rx, type, opcode, (unsigned)pl_len,
		 phase_str(s->phase));

	switch (s->phase) {
	case PH_IDLE:
		/* First peer message = SELECT / proprietary-info. Version negotiation
		 * rides the GATT characteristic; here we log it and drive AUTH0.
		 * (Bench-tunable: some peers expect the reader to send AUTH0 on
		 * channel-up rather than after this first message.) */
		ESP_LOGI(TAG, "[conn %u] peer opened; starting access protocol",
			 s->conn_handle);
		start_auth(s);
		break;
	case PH_SENT_AUTH0:
		on_auth0_response(s, pl, pl_len);
		break;
	case PH_SENT_AUTH1:
		on_auth1_response(s, pl, pl_len);
		break;
	case PH_ESTABLISHED:
		ESP_LOGI(TAG, "[conn %u] post-auth message (ranging control) — not "
			      "yet handled", s->conn_handle);
		break;
	default:
		ESP_LOGW(TAG, "[conn %u] message in phase %s ignored", s->conn_handle,
			 phase_str(s->phase));
		break;
	}
}

/* ---- aliro_ble transport callbacks ---- */

static void on_connected(uint16_t conn_handle)
{
	struct aliro_session *s = session_alloc(conn_handle);

	if (s == NULL) {
		ESP_LOGE(TAG, "[conn %u] no free session slot", conn_handle);
		return;
	}
	ESP_LOGI(TAG, "[conn %u] Aliro session created", conn_handle);
}

static void on_disconnected(uint16_t conn_handle)
{
	struct aliro_session *s = session_find(conn_handle);

	if (s != NULL) {
		ESP_LOGI(TAG, "[conn %u] Aliro session destroyed (%u msgs, phase=%s)",
			 conn_handle, (unsigned)s->msgs_rx, phase_str(s->phase));
		s->active = false;
	}
}

static void on_data(uint16_t conn_handle, const uint8_t *data, uint16_t len)
{
	struct aliro_session *s = session_find(conn_handle);

	if (s == NULL) {
		ESP_LOGW(TAG, "[conn %u] data for unknown session (%u bytes)",
			 conn_handle, (unsigned)len);
		return;
	}
	transaction_feed(s, data, len);
}

int aliro_reader_start(void)
{
	if (aliro_crypto_init() != 0) {
		ESP_LOGE(TAG, "crypto init failed");
		return -1;
	}
	if (provision_dev_identity() != 0) {
		ESP_LOGE(TAG, "dev identity provisioning failed");
		return -1;
	}
	ESP_LOGW(TAG, "using DEV reader identity (Phase 4 supplies the real one)");

	const struct aliro_ble_config cfg = {
		.proto_versions = k_proto_versions,
		.proto_versions_count = sizeof(k_proto_versions) / sizeof(k_proto_versions[0]),
		.features = {
			.timesync_procedure_0 = true,
			.timesync_procedure_1 = false,
			.le_coded_phy = false,
		},
		.cb = {
			.on_data = on_data,
			.on_connected = on_connected,
			.on_disconnected = on_disconnected,
		},
	};

	int rc = aliro_ble_start(&cfg);

	ESP_LOGI(TAG, "aliro_reader_start: transport %s (SPSM 0x%04x)",
		 rc == 0 ? "up" : "FAILED", aliro_ble_spsm());
	return rc;
}
