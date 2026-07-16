/** @file fira_session.h — Range + URSK store for the CCC Pre-POLL responder. */

#ifndef WOZ_UWB_FIRA_SESSION_H_
#define WOZ_UWB_FIRA_SESSION_H_

#include <stdbool.h>
#include <stdint.h>

/** @brief Fetch the most recent valid DS-TWR range; out-params optional (NULL to skip). */
bool fira_session_last_range(int32_t *cm_out, uint16_t *addr_out,
			     uint8_t *nlos_out, uint32_t *block_out,
			     int64_t *age_ms_out);

#if defined(CONFIG_WOZ_ALIRO)
/** @brief Stash an Aliro URSK for the CCC Pre-POLL STS decode; NULL clears it. */
void fira_session_set_provisioned_ursk(const uint8_t *ursk);

/** @brief The stashed Aliro URSK (32 bytes), or NULL if none — for the Pre-POLL decode. */
const uint8_t *fira_session_get_ursk(void);

/** @brief Latch a CCC DS-TWR range so it flows up the Aliro mRangingData seam. */
void fira_session_set_ccc_range_cm(int32_t cm, uint32_t block);

/** @brief STS-index slot clock (inert without a MAC time base); returns 0. */
uint32_t fira_session_current_slot(void);

/* ── Range-integrity gate (anti-spoof / anti Ghost-Peak) ─────────────────────
 *
 * A DS-TWR distance is trustworthy only when it is (1) physically plausible,
 * (2) backed by a well-correlated STS, (3) carried on a clean first path, and
 * (4) consistent with recent blocks. Layers 1 and 4 are enforced in the range
 * store (fira_session_set_ccc_range_cm); layers 2 and 3 are pure predicates the
 * responder RX path evaluates, since it owns the DW3000 diagnostics. Every
 * threshold below is a bring-up default — tune on the bench.
 */

/* Layer 1 — plausibility band. Below -NEG_TOL is physically impossible (an
 * early-first-path / Ghost-Peak spoof drives ToF sharply negative); above MAX
 * is outside any Aliro proximity envelope. A small negative is legitimate
 * point-blank calibration slop and reads as 0 cm rather than being dropped. */
#define FIRA_RANGE_NEG_TOL_CM 30	/* legit point-blank slop; drop beyond */
#define FIRA_RANGE_MAX_CM     3000	/* usable envelope (30 m); tune to radio */

/** @brief Layer 1: true if @p cm is a physically plausible DS-TWR distance. */
bool fira_session_range_plausible(int32_t cm);

/* Layer 2 — STS quality floor. dwt_readstsquality() returns >=0 for good STS,
 * <0 for bad, and its signed index is "good" at >= ~60% of the STS length. A
 * spoofed early path cannot reproduce the scrambled sequence, so its STS
 * quality collapses. Raise MIN toward the 60%-of-length index to tighten. */
#define FIRA_STS_QUALITY_MIN 0	/* index floor; 0 = defer to driver verdict */

/** @brief Layer 2: true if the STS correlated well enough to trust its timestamp.
 *  @param driver_verdict  dwt_readstsquality() return (>=0 good, <0 bad).
 *  @param quality_index   the signed STS quality index it wrote. */
bool fira_session_sts_quality_ok(int32_t driver_verdict, int16_t quality_index);

/* Layer 3 — first-path integrity. A clean line-of-sight first path carries most
 * of the channel power, so total-channel-power / first-path-power stays small;
 * an injected early path (or heavy NLOS bounce) inflates it. Per Decawave APS006
 * the ratio in dB is 10*log10((C<<17)/(F1^2+F2^2+F3^2)); the per-PRF constant and
 * accumulator count cancel, so only F1..F3 and the channel power are needed. The
 * ceiling is stored x100 to stay integer: 398 ~ 6.0 dB. */
#define FIRA_FP_RATIO_X100 398	/* ~6 dB total-vs-first-path gap ceiling */

/** @brief Layer 3: true if the Ipatov first path looks like a genuine LOS path.
 *  @param f1,f2,f3   ipatovF1..F3 first-path magnitude points.
 *  @param cir_power  ipatovPower (channel-area power estimate). */
bool fira_session_first_path_ok(uint32_t f1, uint32_t f2, uint32_t f3,
				uint32_t cir_power);

/* Layer 4 — cross-block consensus. A single injected block cannot move an
 * unlock decision alone: a range is "trusted" only once K consecutive plausible
 * blocks agree to within SPREAD. This does not gate the latched last-range (the
 * shell/telemetry still track live values); it is an extra signal the unlock
 * policy should AND with distance and age. */
#define FIRA_RANGE_TRUST_K   3	/* consecutive agreeing blocks to trust */
#define FIRA_RANGE_SPREAD_CM 50	/* max block-to-block delta to stay agreed */

/** @brief Layer 4: true once >= K consecutive plausible, mutually consistent
 *  ranges have been latched. Cleared by any implausible or outlier block. */
bool fira_session_range_trusted(void);
#endif /* CONFIG_WOZ_ALIRO */

#endif /* WOZ_UWB_FIRA_SESSION_H_ */
