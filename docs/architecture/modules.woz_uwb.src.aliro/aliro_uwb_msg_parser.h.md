<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.h`

@file aliro_uwb_msg_parser.h — TLV attribute iteration and big-endian reads.

**depends on** [`modules/woz_uwb/src/aliro/aliro_uwb_msg_spec.h`](aliro_uwb_msg_spec.h.md), [`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h`](../modules.woz_uwb.src.aliro.include.aliro_uwb_adapter/aliro_uwb_session.h.md)  ·  **used by** [`modules/woz_uwb/src/aliro/aliro_uwb_msg.c`](aliro_uwb_msg.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.c`](aliro_uwb_msg_parser.c.md)

## API

### `struct aliro_uwb_msg_attribute`
`modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.h:14`

A single type/length/value attribute overlaid on the message bytes.

### `struct aliro_uwb_msg_parser`
`modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.h:21`

Cursor walking the attributes of one message payload.
