<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/facade/woz_uwb_facade.c`

UWB facade: binds the CCC credential-based STS engine to the DW3000 radio, exposes Aliro DS-TWR
responder start/stop and range query, and manages platform dependencies (HFCLK boost, SPI init,
callbacks).

**depends on** [`modules/woz_uwb/src/ccc/aliro_kdf.h`](../modules.woz_uwb.src.ccc/aliro_kdf.h.md), [`modules/woz_uwb/src/ccc/ccc_shim.h`](../modules.woz_uwb.src.ccc/ccc_shim.h.md), [`modules/woz_uwb/src/facade/woz_uwb_facade.h`](woz_uwb_facade.h.md), [`modules/woz_uwb/src/fira/fira_session.h`](../modules.woz_uwb.src.fira/fira_session.h.md)  ·  **discussed in** [`docs/porting.md`](../../porting.md)

## API

### `static void woz_hfclk_ensure_128mhz(void)`
`modules/woz_uwb/src/facade/woz_uwb_facade.c:26`

@brief One-shot boost of the app-core HFCLK to 128 MHz for the DW3000 SPI bus.
nRF5340-specific platform seam: the app core boots with HFCLK divided to
64 MHz. Other SoCs (e.g. ESP32-S3) clock their SPI controller independently,
so this compiles to a no-op there. See docs/porting.md.

**called by** `woz_uwb_bind_ursk`, `woz_uwb_start_aliro`

### `int woz_uwb_bind_ursk(const uint8_t *ursk, size_t ursk_len)`
`modules/woz_uwb/src/facade/woz_uwb_facade.c:44`

@brief Bind the CCC STS from the add-on-supplied plaintext URSK; returns 0 on success.
@param ursk Pointer to the URSK bytes.
@param ursk_len Length of the URSK.
@return 0 on success, -EINVAL if ursk is NULL.

**calls** `woz_hfclk_ensure_128mhz`

### `int woz_uwb_start_aliro(const struct woz_uwb_aliro_cfg *c)`
`modules/woz_uwb/src/facade/woz_uwb_facade.c:59`

@brief Start the CCC DS-TWR responder bound to a live Aliro credential; returns 0 on success.
@param c Configuration struct (channel, sync_code_index, ursk, ranging_config, sts_index0,
slot_per_round).
@return 0 on success, -EINVAL if config is NULL or ursk is NULL, -EIO if radio initialization
fails.

**calls** `woz_hfclk_ensure_128mhz`

### `void woz_uwb_stop(void)`
`modules/woz_uwb/src/facade/woz_uwb_facade.c:88`

@brief Quiesce the radio and unbind the CCC STS shim.

### `bool woz_uwb_last_range_cm(int32_t *cm_out)`
`modules/woz_uwb/src/facade/woz_uwb_facade.c:102`

@brief Retrieve the last valid DS-TWR distance measurement in centimeters.
@param cm_out Pointer to store the distance in cm.
@return True if a valid range has been seen since initialization; false otherwise.

### `bool woz_uwb_trusted_range_cm(int32_t *cm_out)`
`modules/woz_uwb/src/facade/woz_uwb_facade.c:115`

@brief Retrieve the last valid DS-TWR distance in centimeters, gated by range-integrity
consensus.
@param cm_out Pointer to store the distance in cm.
@return True only when a valid range has been seen AND it is trusted by the layer-4 consensus
gate; false if no valid range exists or the range is not yet trusted. When CONFIG_WOZ_ALIRO is
not defined, behaves identically to woz_uwb_last_range_cm().
