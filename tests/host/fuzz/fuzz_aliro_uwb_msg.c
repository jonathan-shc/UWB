/* fuzz_aliro_uwb_msg.c — the Aliro UWB ranging-service TLV parser.
 *
 * Models the real entry: a message is [4-byte header][TLV payload]. We hand the
 * whole buffer to the attribute cursor positioned past the header (exactly as
 * ALIRO_UWB_MSG_PARSER_INIT does) and walk every attribute, decoding each width.
 * The input bytes ARE the wire message, so libFuzzer/ASan see the same buffer
 * bounds the radio path would. */
#include <stddef.h>
#include <stdint.h>

#include "aliro_uwb_msg_parser.h"
#include "aliro_uwb_msg_spec.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	/* The session handler rejects anything shorter than the header. */
	if (size < ALIRO_HEADER_LENGTH) {
		return 0;
	}

	struct aliro_uwb_msg_parser parser = {
		.length = size,
		.offset = ALIRO_HEADER_LENGTH,
		.data = data,
	};
	struct aliro_uwb_msg_attribute *attr;

	while ((attr = aliro_uwb_msg_next_attribute(&parser)) != NULL) {
		uint8_t v8;
		uint16_t v16;
		uint32_t v32;
		uint64_t v64;

		/* Each reader validates the declared width before touching value
		 * bytes; call all four so the fuzzer exercises every path. */
		(void)aliro_uwb_msg_read_u8(attr, "u8", &v8);
		(void)aliro_uwb_msg_read_u16(attr, "u16", &v16);
		(void)aliro_uwb_msg_read_u32(attr, "u32", &v32);
		(void)aliro_uwb_msg_read_u64(attr, "u64", &v64);
	}
	return 0;
}
