<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_adapter.h`

@file aliro_uwb_adapter.h — reader-device public interface.

**depends on** [`modules/woz_uwb/src/aliro/include/cherry/cherry.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry.h.md), [`modules/woz_uwb/src/aliro/include/cherry/cherry_ccc.h`](../modules.woz_uwb.src.aliro.include.cherry/cherry_ccc.h.md)  ·  **used by** [`modules/woz_aliro/src/aliro_ranging.c`](../modules.woz_aliro.src/aliro_ranging.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_adapter.c`](../modules.woz_uwb.src.aliro/aliro_uwb_adapter.c.md), [`modules/woz_uwb/src/aliro/aliro_uwb_internal.h`](../modules.woz_uwb.src.aliro/aliro_uwb_internal.h.md), [`modules/woz_uwb/src/aliro/aliro_uwb_msg.c`](../modules.woz_uwb.src.aliro/aliro_uwb_msg.c.md)

## API

### `struct aliro_uwb_preferred_hopping_configs`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_adapter.h:46`

@brief Ordered hopping preferences (at least one default sequence required).

### `struct aliro_uwb_adapter_reader_config`
`modules/woz_uwb/src/aliro/include/aliro_uwb_adapter/aliro_uwb_adapter.h:54`

@brief Reader-side selection preferences (borrowed for the adapter's lifetime).
