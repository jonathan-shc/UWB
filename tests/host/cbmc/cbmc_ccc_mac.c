/* CBMC harness — CCC UWB MAC SP0 parsers memory safety.
 *
 * ccc_final_data_parse is variable-length: prove it never reads past the
 * declared length for any buffer up to FD_MAX. ccc_parse_mhr / ccc_pre_poll_parse
 * are contracted on fixed buffers, so they run against exactly-sized buffers to
 * prove they stay inside the 23 / 13 bytes the contract promises. All inputs are
 * nondeterministic. */
#include <stddef.h>
#include <stdint.h>

#include "ccc_mac.h"

/* Large enough that Final_Data with a few responder records (18 + 7*n) is
 * reachable, exercising the per-responder loop. */
#define FD_MAX 40

size_t nondet_size(void);

void harness(void)
{
	uint8_t vbuf[FD_MAX];
	size_t size = nondet_size();

	__CPROVER_assume(size <= FD_MAX);

	struct ccc_final_data fd;

	(void)ccc_final_data_parse(vbuf, size, &fd);

	uint8_t mbuf[CCC_MHR_LEN];
	struct ccc_mhr_fields mf;

	(void)ccc_parse_mhr(mbuf, &mf);

	uint8_t pbuf[CCC_PRE_POLL_LEN];
	struct ccc_pre_poll pp;

	(void)ccc_pre_poll_parse(pbuf, &pp);
}
