<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h`

@file aliro_uwb_session.h — per-session public interface.

**depends on** [`modules/woz_uwb/src/aliro/include/cherry/cherry.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry.h.md), [`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry_ccc.h.md)  ·  **used by** [`modules/woz_aliro/src/aliro_ranging.c`](../modules.woz_aliro.src/aliro_ranging.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_internal.h`](../modules.woz_uwb.src.aliro/aliro_uwb_internal.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg.h`](../modules.woz_uwb.src.aliro/aliro_uwb_msg.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg_builder.h`](../modules.woz_uwb.src.aliro/aliro_uwb_msg_builder.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.h`](../modules.woz_uwb.src.aliro/aliro_uwb_msg_parser.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_session.c`](../modules.woz_uwb.src.aliro/aliro_uwb_session.c.md)

## API

### `struct aliro_uwb_message`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:20`

Framed Aliro BLE message (header + TLV payload).

### `struct aliro_uwb_session_event`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h:40`

Session event handed to the client.
