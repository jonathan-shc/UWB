/* Pass-through __wrap for the diagnostic dwt_configurestsmode intercept.
 *
 * ccc_shim_rx.c (the essential RX path) calls __real_dwt_configurestsmode,
 * which the linker only produces when -Wl,--wrap=dwt_configurestsmode is passed.
 * That flag in turn needs a __wrap_dwt_configurestsmode definition; in the
 * Nordic build it lives in the diagnostic uwb_rxdiag.c, which this port omits
 * (it needs k_work). Supply a minimal forwarding wrap so the essential path
 * links without pulling in the diagnostics. */
#include <stdint.h>

void __real_dwt_configurestsmode(uint8_t stsMode);

void __wrap_dwt_configurestsmode(uint8_t stsMode)
{
	__real_dwt_configurestsmode(stsMode);
}

/* Diagnostic decode-latency counter owned by the omitted uwb_rxdiag.c; ccc_shim_rx.c
 * reads it (extern) only in DIAG log lines. Define it here so those references
 * resolve (stays 0 without the rxdiag stamper — diagnostics only). */
uint32_t g_ccc_dbg_decode;
