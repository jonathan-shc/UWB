<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/aliro/aliro_uwb_internal.h`

@file aliro_uwb_internal.h — private context types and shared helpers.

**depends on** [`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_adapter.h`](../modules.woz_uwb.src.aliro.include.aliro_uwb_adapter/aliro_uwb_adapter.h.md), [`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h`](../modules.woz_uwb.src.aliro.include.aliro_uwb_adapter/aliro_uwb_session.h.md), [`modules/woz_uwb/src/aliro/include/cherry/cherry.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry.h.md), [`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry_ccc.h.md)  ·  **used by** [`modules/woz_uwb/src/aliro/aliro_uwb_adapter.c`](aliro_uwb_adapter.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg.h`](aliro_uwb_msg.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_session.c`](aliro_uwb_session.c.md)

## API

### `struct aliro_uwb_adapter`
`modules/woz_uwb/src/aliro/aliro_uwb_internal.h:16`

@brief Session-independent reader state shared across all ranging sessions for an Aliro UWB
adapter.

### `struct aliro_uwb_session`
`modules/woz_uwb/src/aliro/aliro_uwb_internal.h:54`

@brief Per-approach ranging-setup session record holding the CCC state machine and derived keys
for one Aliro exchange.
