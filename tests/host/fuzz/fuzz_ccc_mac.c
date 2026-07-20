/* fuzz_ccc_mac.c — the CCC UWB MAC SP0 frame parsers.
 *
 * ccc_final_data_parse is variable-length and self-checking, so it takes the raw
 * buffer directly. ccc_parse_mhr and ccc_pre_poll_parse are contracted on fixed
 * buffers (23 / 13 bytes); we honour that contract by only calling them once the
 * input is at least that long, which keeps the harness itself in-bounds while
 * still driving every branch of the parsers with adversarial content. */
#include <stddef.h>
#include <stdint.h>

#include "ccc_mac.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct ccc_final_data fd;

	(void)ccc_final_data_parse(data, size, &fd);

	if (size >= CCC_MHR_LEN) {
		struct ccc_mhr_fields f;

		(void)ccc_parse_mhr(data, &f);
	}
	if (size >= CCC_PRE_POLL_LEN) {
		struct ccc_pre_poll p;

		(void)ccc_pre_poll_parse(data, &p);
	}
	return 0;
}
