<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/ccc/cherry_ccc_shim.c`

@file cherry_ccc_shim.c — cherry_ccc_* seam (Aliro responder) implemented over the lock-native
FiRa MAC; maps each call onto woz_uwb_facade.

**depends on** [`modules/woz_uwb/src/aliro/include/cherry/cherry.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry.h.md), [`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry_ccc.h.md), [`modules/woz_uwb/src/aliro/include/cherry/cherry_session.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry_session.h.md), [`modules/woz_uwb/src/ccc/aliro_round_config.h`](aliro_round_config.h.md), [`modules/woz_uwb/src/facade/woz_alloc.h`](../modules.woz_uwb.src.facade/woz_alloc.h.md), [`modules/woz_uwb/src/facade/woz_uwb_facade.h`](../modules.woz_uwb.src.facade/woz_uwb_facade.h.md), [`ports/esp32-idf/components/woz_uwb/compat/zephyr/logging/log.h`](../ports.esp32-idf.components.woz_uwb.compat.zephyr.logging/log.h.md), [`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/util.h`](../ports.esp32-idf.components.woz_uwb.compat.zephyr.sys/util.h.md)

## API

#### `struct cherry_session`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:36`

Base session object; first member of cherry_ccc_session so the base functions can up-cast.

#### `struct cherry_ccc_aliro_session_config *config`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:38`

Borrowed pointer to the adapter's negotiated params, valid until destroy.

### `static inline struct cherry_ccc_session *to_ccc(struct cherry_session *base)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:45`

@brief Up-cast a base session pointer (base is the first member).

**called by** `cherry_session_destroy`, `cherry_session_start`, `cherry_session_stop`

### `static void emit_status(struct cherry_ccc_session *s, enum cherry_ccc_session_state st)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:52`

Allocate + dispatch a SESSION_STATUS event to the CCC callback (event and data heap-allocated).

**called by** `cherry_session_destroy`, `cherry_session_start`, `cherry_session_stop`

### `struct cherry_ccc_session_event_session_status *status = qmalloc(sizeof(*status))`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:57`

Heap-allocated session status event data; contains a state enum to be reported via the
CCC callback.

### `static void emit_error(struct cherry_ccc_session *s, enum cherry_err err)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:77`

@brief Allocate + dispatch a SESSION_ERROR event to the CCC callback.

**called by** `cherry_session_start`

### `struct cherry_ccc_session_event_error *e = qmalloc(sizeof(*e))`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:82`

Heap-allocated error event data; contains an error code to be reported via the CCC
callback.

### `struct cherry *cherry_create(const char *device, cherry_core_cb_t core_cb, void *user_data)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:103`

Allocate and initialize a Cherry context with the given core callback and user data; device
parameter is unused; returns NULL if allocation fails.

### `void cherry_destroy_sync(struct cherry *ctx)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:117`

Deallocate a Cherry context; null input is safely ignored.

### `cherry_ccc_session_create_aliro_responder`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:126`

Allocate and initialize an Aliro responder CCC session with the given callback, user context, and
configuration; returns NULL if callback, config, or allocation fails.

### `struct cherry`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:127`

Opaque Cherry context: a holder for the (unused) core callback the adapter passes at create.

### `struct cherry_session *cherry_ccc_session_to_base(struct cherry_ccc_session *session)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:151`

Cast a CCC session pointer to its embedded base session structure; null propagates (the header
wrappers rely on it).

### `void *cherry_session_get_user_data(struct cherry_session *session)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:157`

Retrieve the user data pointer stored in the base session; returns NULL if session is null.

### `void cherry_session_destroy(struct cherry_session *session)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:164`

Stop the UWB radio, emit a DEINIT status event, and deallocate the session; safe if session or
its base pointer is null.

**calls** `emit_status`, `to_ccc`

### `enum cherry_err cherry_session_start(struct cherry_session *session)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:184`

Start an Aliro UWB session by building a RangingConfiguration byte array from the session config,
calling woz_uwb_start_aliro, and emitting IDLE then ACTIVE status events; returns
CHERRY_ERR_INVALID_PARAMETER if session or config is null, CHERRY_ERR_SESSION_CONFIG if URSK is
not set, or CHERRY_ERR_SESSION_INIT if the UWB start fails.

**calls** `emit_error`, `emit_status`, `to_ccc`

### `struct cherry_ccc_aliro_session_config *config`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:187`

Borrowed pointer to the adapter's negotiated params, valid until destroy.

### `struct woz_uwb_aliro_cfg fcfg`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:191`

Configuration struct for woz_uwb_start_aliro; holds session ID, channel, sync code,
timing, slot geometry, STS index, UWB time, URSK key, and the RangingConfiguration byte
array required to derive the CCC SaltedHash.

### `enum cherry_err cherry_session_stop(struct cherry_session *session)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:262`

Stop the UWB radio and emit an IDLE status event; returns CHERRY_ERR_INVALID_PARAMETER if session
or its base pointer is null, otherwise CHERRY_ERR_NONE.

**calls** `emit_status`, `to_ccc`

### `enum cherry_err cherry_ccc_session_set_ursk(struct cherry_ccc_session *session, const uint8_t *ursk)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:278`

Copy the URSK (Unique Responder Session Key) into the session and mark it as present; returns
CHERRY_ERR_INVALID_PARAMETER if session or ursk is null, otherwise CHERRY_ERR_NONE.

### `enum cherry_err cherry_ccc_session_set_protocol_version(struct cherry_ccc_session *session, uint16_t selected_protocol_version)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:290`

Validate that the session exists; the selected_protocol_version parameter is accepted but
ignored; returns CHERRY_ERR_INVALID_PARAMETER if session is null, otherwise CHERRY_ERR_NONE.

### `enum cherry_err cherry_ccc_session_set_sts_index(struct cherry_ccc_session *session, uint32_t sts_index)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:299`

Store the STS index on the session config; returns CHERRY_ERR_INVALID_PARAMETER if session or its
config is null, otherwise CHERRY_ERR_NONE.

### `enum cherry_err cherry_ccc_session_set_initiation_time(struct cherry_ccc_session *session, uint64_t initiation_time_us)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:311`

Store the UWB initiation timestamp in microseconds on the session config; returns
CHERRY_ERR_INVALID_PARAMETER if session or its config is null, otherwise CHERRY_ERR_NONE.

### `enum cherry_err cherry_ccc_session_set_round2_antennas(struct cherry_ccc_session *session, uint8_t tx_antenna_set, uint8_t rx_antenna_set)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:323`

Validate that the session exists; TX and RX antenna set parameters are accepted but ignored;
returns CHERRY_ERR_INVALID_PARAMETER if session is null, otherwise CHERRY_ERR_NONE.

### `enum cherry_err cherry_ccc_session_set_round2_antennas(struct cherry_ccc_session *session, uint8_t tx_antenna_set, uint8_t rx_antenna_set)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:323`

Validate that the session exists; TX and RX antenna set parameters are accepted but ignored;
returns CHERRY_ERR_INVALID_PARAMETER if session is null, otherwise CHERRY_ERR_NONE.

### `enum cherry_err cherry_session_set_antennas(struct cherry_session *session, uint8_t tx_antenna_set, uint8_t rx_antenna_set)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:334`

Validate that the session exists; TX and RX antenna set parameters are accepted but ignored;
returns CHERRY_ERR_INVALID_PARAMETER if session is null, otherwise CHERRY_ERR_NONE.

### `enum cherry_err cherry_session_set_diagnostics(struct cherry_session *session, struct cherry_common_diag_cfg config, bool controlee_only)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:344`

Validate that the session exists; diagnostics config and controlee_only parameters are accepted
but ignored; returns CHERRY_ERR_INVALID_PARAMETER if session is null, otherwise CHERRY_ERR_NONE.

### `enum cherry_err cherry_session_set_diagnostics(struct cherry_session *session, struct cherry_common_diag_cfg config, bool controlee_only)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:344`

Validate that the session exists; diagnostics config and controlee_only parameters are accepted
but ignored; returns CHERRY_ERR_INVALID_PARAMETER if session is null, otherwise CHERRY_ERR_NONE.

### `void cherry_ccc_event_free(struct cherry_ccc_event *event)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:356`

Free a CCC event handle.

### `void cherry_ccc_event_free(struct cherry_ccc_event *event)`
`modules/woz_uwb/src/ccc/cherry_ccc_shim.c:356`

Free a CCC event handle.

<details><summary>Undocumented (1)</summary>

- `cherry_common_diag_cfg`

</details>
