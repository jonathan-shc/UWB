<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h`

@file aliro_uwb_session.h — per-session public interface.

**depends on** [`modules/woz_uwb/src/aliro/include/cherry/cherry.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry.h.md), [`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry_ccc.h.md)  ·  **used by** [`modules/woz_aliro/src/aliro_ranging.c`](../modules.woz_aliro.src/aliro_ranging.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_internal.h`](../modules.woz_uwb.src.aliro/aliro_uwb_internal.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg.h`](../modules.woz_uwb.src.aliro/aliro_uwb_msg.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg_builder.h`](../modules.woz_uwb.src.aliro/aliro_uwb_msg_builder.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.h`](../modules.woz_uwb.src.aliro/aliro_uwb_msg_parser.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_session.c`](../modules.woz_uwb.src.aliro/aliro_uwb_session.c.md)

## API

### `struct aliro_uwb_message`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:24`

@brief Framed Aliro BLE message with 4-byte header followed by TLV payload.
@param len Number of valid bytes in @p data.
@param data Message bytes (4-byte header followed by TLV attributes).

### `struct aliro_uwb_session_event`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:52`

@brief Session event handed to the client, carrying status, error, or distance reports.
@param session Opaque per-session context.
@param type Event type (status, error, controller report, controlee report, or diagnostics).
@param cherry_event Underlying Cherry event object.
@param data Union holding the event payload: session status, error code and diagnostic context,
controller distance and timestamp estimate, controlee distance and timestamp estimate, or
diagnostic report snapshot.
