<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/driver/uwb_selftest.c`

@file uwb_selftest.c — Kconfig-gated one-shot UWB init self-test (no iPhone).

**depends on** [`modules/woz_uwb/src/ccc/ccc_shim.h`](../modules.woz_uwb.src.ccc/ccc_shim.h.md), [`modules/woz_uwb/src/facade/woz_uwb_facade.h`](../modules.woz_uwb.src.facade/woz_uwb_facade.h.md)  ·  **discussed in** [`docs/porting-esp32.md`](../../porting-esp32.md), [`docs/porting.md`](../../porting.md)

## API

### `static void uwb_selftest_work(struct k_work *work)`
`modules/woz_uwb/src/driver/uwb_selftest.c:27`

@brief One-shot worker: run the Aliro UWB start path and log the outcome.
@param work Kernel work item (unused).

### `static int uwb_selftest_init(void)`
`modules/woz_uwb/src/driver/uwb_selftest.c:59`

@brief Arm the one-shot self-test at application init.
@return 0 on success.
