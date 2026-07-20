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

### `struct cherry_ccc_event`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:108`

CCC notification delivered to the adapter.

### `struct cherry_ccc_aliro_session_config`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:155`

Negotiated Aliro ranging params (filled in-place across M1-M4).

### `static inline void *cherry_ccc_session_get_user_data(struct cherry_ccc_session *session)`
`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h:188`

@brief Fetch the CCC session's user_data via its base session.
@param session CCC session to query.
@return The user_data pointer associated with the session's base.

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
