<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/driver/uwb_min.h`

@file uwb_min.h — Minimal DW3110 (DWM3000EVB) hardware bring-up driver.

**used by** [`modules/woz_uwb/src/ccc/ccc_shim_rx.c`](../modules.woz_uwb.src.ccc/ccc_shim_rx.c.md), [`modules/woz_uwb/src/driver/uwb_min.c`](uwb_min.c.md), [`modules/woz_uwb/src/shell/aliro_shell.c`](../modules.woz_uwb.src.shell/aliro_shell.c.md)

## API

### `struct uwb_selftest_result`
`modules/woz_uwb/src/driver/uwb_min.h:13`

@brief Self-test result emitted by @ref uwb_min_selftest.

### `struct uwb_twr_result`
`modules/woz_uwb/src/driver/uwb_min.h:31`

@brief Result of a raw static-STS SS-TWR initiator burst (@ref uwb_min_twr_poll).

### `struct uwb_twr_frame`
`modules/woz_uwb/src/driver/uwb_min.h:40`

@brief Outcome of one raw SS-TWR POLL/RESP exchange (@ref uwb_min_twr_exchange).
