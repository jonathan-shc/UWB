<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/ccc/ccc_shim_wrap.c`

@file ccc_shim_wrap.c — per-frame STS interception (ld --wrap=dwt_configurestsiv) substituting
CCC STS for the FiRa MAC; target only.

**depends on** [`modules/woz_uwb/src/ccc/ccc_shim.h`](ccc_shim.h.md), [`ports/esp32-idf/components/woz_uwb/compat/zephyr/logging/log.h`](../ports.esp32-idf.components.woz_uwb.compat.zephyr.logging/log.h.md), [`ports/esp32-idf/components/woz_uwb/compat/zephyr/sys/byteorder.h`](../ports.esp32-idf.components.woz_uwb.compat.zephyr.sys/byteorder.h.md)

## API

### `void ccc_shim_wrap_log_reset(void)`
`modules/woz_uwb/src/ccc/ccc_shim_wrap.c:41`

Reset the frame logging counter to zero, used for re-enabling diagnostics after the first N
frames.

### `static void pack_key(dwt_sts_cp_key_t *out, const uint8_t dursk[CCC_DURSK_LEN])`
`modules/woz_uwb/src/ccc/ccc_shim_wrap.c:47`

@brief Pack a 16-byte `dURSK` into the DW3000 STS-key register image.

**called by** `__wrap_dwt_configurestsiv`

### `static void pack_iv(dwt_sts_cp_iv_t *out, const uint8_t sts_v[CCC_STS_V_LEN])`
`modules/woz_uwb/src/ccc/ccc_shim_wrap.c:62`

Pack a 16-byte STS-V into the DW3000 STS-IV image (whole-16 reverse then per-word LE, same as
pack_key).

**called by** `__wrap_dwt_configurestsiv`

### `void __wrap_dwt_configurestsiv(dwt_sts_cp_iv_t *pStsIv)`
`modules/woz_uwb/src/ccc/ccc_shim_wrap.c:78`

Intercept a DW3000 STS IV configuration, mapping the FiRa blob index to CCC index space, deriving
the dURSK and STS-V, and configuring the radio with CCC secrets. Logs key and IV register
contents if tracing is enabled. Falls through to the real dwt_configurestsiv if shim is inactive.

**calls** `pack_iv`, `pack_key`
