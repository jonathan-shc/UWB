<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h`

@file aliro_uwb_session.h — per-session public interface.

**depends on** [`modules/woz_uwb/src/aliro/include/cherry/cherry.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry.h.md), [`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry_ccc.h.md)  ·  **used by** [`modules/woz_uwb/src/aliro/aliro_uwb_internal.h`](../modules.woz_uwb.src.aliro/aliro_uwb_internal.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg.h`](../modules.woz_uwb.src.aliro/aliro_uwb_msg.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg_builder.h`](../modules.woz_uwb.src.aliro/aliro_uwb_msg_builder.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.h`](../modules.woz_uwb.src.aliro/aliro_uwb_msg_parser.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_session.c`](../modules.woz_uwb.src.aliro/aliro_uwb_session.c.md), [`ports/esp32-idf/components/aliro_reader/aliro_ranging.c`](../ports.esp32-idf.components.aliro_reader/aliro_ranging.c.md)

## API

#### `struct aliro_uwb_session *session`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:44`

@brief Opaque per-session context.

#### `struct cherry_ccc_session_event_session_status *status`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:52`

@brief Session status report (active, paused, stopped) and update reason
(initiation, measurement update, termination).

#### `struct cherry_ccc_session_event_error *error`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:56`

@brief Session error payload carrying error code and diagnostic context.

#### `struct cherry_ccc_controller_session_report *controller_report`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:61`

@brief Controller (phone) session report carrying its distance and timestamp
estimate.

#### `struct cherry_ccc_controlee_session_report *controlee_report`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:66`

@brief Controlee (lock) session report carrying its distance and timestamp
estimate.

#### `struct cherry_common_diag_report *diagnostics`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:71`

@brief Diagnostic report snapshot (ranging samples, signal strength, time sync)
for this update.

### `struct aliro_uwb_session *aliro_uwb_session_create(struct aliro_uwb_adapter *aliro_ctx, uint32_t session_id, aliro_uwb_session_cb_t callback, aliro_uwb_adapter_transmit_message_t transmit, void *user_data)`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:92`

@brief Create a session in the CREATED state. No CCC session is started here.
@param aliro_ctx Adapter supplying the Cherry context and reader configuration.
@param session_id Session identifier carried in the ranging-service messages.
@param callback Session event callback; must not be NULL.
@param transmit Message transmit callback; must not be NULL.
@param user_data Opaque pointer passed back to the callbacks.
@return New session, or NULL on bad parameters or allocation failure.

### `void aliro_uwb_session_event_free(struct aliro_uwb_session_event *event)`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:108`

@brief Free an event delivered to the session callback.
@param event Event to free.

### `struct aliro_uwb_message`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:127`

Framed Aliro BLE message (header + TLV payload).

### `enum aliro_uwb_err aliro_uwb_session_resume(struct aliro_uwb_session *session)`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:140`

@brief Build and send a resume request for a suspended Aliro UWB ranging session.
@param session Aliro UWB session to resume.
@return Aliro UWB error code indicating success or failure of the resume request.
