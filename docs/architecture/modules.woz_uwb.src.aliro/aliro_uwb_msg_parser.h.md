<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.h`

@file aliro_uwb_msg_parser.h — TLV attribute iteration and big-endian reads.

**depends on** [`modules/woz_uwb/src/aliro/aliro_uwb_msg_spec.h`](aliro_uwb_msg_spec.h.md), [`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_session.h`](../modules.woz_uwb.src.aliro.include.aliro_uwb_adapter/aliro_uwb_session.h.md)  ·  **used by** [`modules/woz_uwb/src/aliro/aliro_uwb_msg.c`](aliro_uwb_msg.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.c`](aliro_uwb_msg_parser.c.md)

## API

### `struct aliro_uwb_msg_attribute *aliro_uwb_msg_next_attribute(struct aliro_uwb_msg_parser *parser)`
`modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.h:36`

Advance to the next attribute; NULL at end-of-payload or on overrun.

### `struct aliro_uwb_msg_attribute`
`modules/woz_uwb/src/aliro/aliro_uwb_msg_parser.h:45`

A single type/length/value attribute overlaid on the message bytes.
