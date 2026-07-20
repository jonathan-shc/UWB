<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h`

@file cherry_ccc.h — CCC/Aliro-session interface (seam the adapter drives).

**depends on** [`modules/woz_uwb/src/aliro/include/cherry/cherry.h`](cherry.h.md), [`modules/woz_uwb/src/aliro/include/cherry/cherry_common.h`](cherry_common.h.md), [`modules/woz_uwb/src/aliro/include/cherry/cherry_session.h`](cherry_session.h.md)  ·  **used by** [`modules/woz_aliro/src/aliro_ranging.c`](../modules.woz_aliro.src/aliro_ranging.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_adapter.c`](../modules.woz_uwb.src.aliro/aliro_uwb_adapter.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_internal.h`](../modules.woz_uwb.src.aliro/aliro_uwb_internal.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_session.c`](../modules.woz_uwb.src.aliro/aliro_uwb_session.c.md), [`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_adapter.h`](../modules.woz_uwb.src.aliro.include.aliro_uwb_adapter/aliro_uwb_adapter.h.md), [`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h`](../modules.woz_uwb.src.aliro.include.aliro_uwb_adapter/aliro_uwb_session.h.md), [`modules/woz_uwb/src/ccc/cherry_ccc_shim.c`](../modules.woz_uwb.src.ccc/cherry_ccc_shim.c.md)

## API

### `struct cherry_ccc_capabilities`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:26`

@brief Device CCC/Aliro capability set advertised to the peer.

### `struct cherry_ccc_session_event_session_status`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:84`

@brief Payload of a SESSION_STATUS event, giving the new session state and the reason for the
change.

### `struct cherry_ccc_session_event_error`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:92`

@brief Payload of a SESSION_ERROR event, giving the error status that triggered it.

### `struct cherry_ccc_controller_session_report`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:100`

@brief Opaque controller-side CCC session report; instances cross the API boundary only by
pointer.

### `struct cherry_ccc_controlee_session_report`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:105`

@brief Opaque CCC controlee (lock) session report structure, not defined outside the cherry
library.

#### `struct cherry_ccc_session *session`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:113`

@brief Opaque CCC session the event pertains to, defined by the shim.

#### `struct cherry_ccc_session_event_session_status *status`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:118`

@brief Payload pointer for a SESSION_STATUS event.

#### `struct cherry_ccc_session_event_error *error`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:122`

@brief Payload pointer for a SESSION_ERROR event.

#### `struct cherry_ccc_controller_session_report *controller_report`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:127`

@brief Pointer to the controller-side session report payload; report payloads
cross the seam only by pointer on this lock.

#### `struct cherry_ccc_controlee_session_report *controlee_report`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:132`

@brief Pointer to the controlee session report payload, carrying a distance and
timestamp snapshot.

#### `struct cherry_common_diag_report *diagnostics`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:137`

@brief Pointer to the diagnostic report snapshot (ranging samples, signal
metrics) for this CCC event.

### `struct cherry_ccc_aliro_session_config`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:176`

Negotiated Aliro ranging params (filled in-place across M1-M4).

### `struct cherry_session *cherry_ccc_session_to_base(struct cherry_ccc_session *session)`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:181`

@brief Return the base session for a CCC session (the base is the first member).

### `static inline void *cherry_ccc_session_get_user_data(struct cherry_ccc_session *session)`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:188`

@brief Fetch the CCC session's user_data via its base session.
@param session CCC session to query.
@return The user_data pointer associated with the session's base.

### `void cherry_ccc_event_free(struct cherry_ccc_event *event)`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:197`

@brief Release a CCC event and any heap payload it owns.
@param event Event to free.

### `static inline void cherry_ccc_session_destroy(struct cherry_ccc_session *session)`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:204`

@brief Destroy a CCC session, delegating to the base session.
@param session CCC session to destroy.

### `static inline enum cherry_err cherry_ccc_session_start(struct cherry_ccc_session *session)`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:215`

@brief Start a CCC session, delegating to the base session.
@param session CCC session to start.
@return Error code from cherry_session_start.

### `static inline enum cherry_err cherry_ccc_session_stop(struct cherry_ccc_session *session)`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:226`

@brief Stop a CCC session, delegating to the base session.
@param session CCC session to stop.
@return Error code from cherry_session_stop.

### `static inline enum cherry_err cherry_ccc_session_set_antennas(struct cherry_ccc_session *session, uint8_t tx_antenna_set, uint8_t rx_antenna_set)`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:239`

@brief Select round-1 antennas for a CCC session, delegating to the base session.
@param session CCC session to configure.
@param tx_antenna_set Antenna set to use for transmission.
@param rx_antenna_set Antenna set to use for reception.
@return Error code from cherry_session_set_antennas.

### `cherry_ccc_session_set_diagnostics`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:272`

@brief Opaque CCC session being configured, defined by the shim.

### `struct cherry_ccc_session`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:273`

Opaque CCC session (defined by the shim).

<details><summary>Undocumented (2)</summary>

- `cherry`
- `cherry_common_diag_cfg`

</details>
