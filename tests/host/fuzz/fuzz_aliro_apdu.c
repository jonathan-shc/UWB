/* fuzz_aliro_apdu.c — the Aliro BLE credential-auth wire codec.
 *
 * Two entry paths, both fed adversarial bytes:
 *  1. Envelope path: unframe the 4-byte L2CAP envelope, strip the ISO7816 status
 *     word, then parse the remainder as AUTH0 and AUTH1 responses — the real
 *     device->reader receive path.
 *  2. Raw path: run the TLV walker and both response parsers over the whole
 *     buffer, so the BER-TLV length decoder is exercised without the envelope
 *     gating which bytes reach it. */
#include <stddef.h>
#include <stdint.h>

#include "aliro_apdu.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	uint8_t type;
	uint8_t opcode;
	const uint8_t *payload;
	size_t plen;

	if (aliro_ble_unframe(data, size, &type, &opcode, &payload, &plen) == 0) {
		size_t body = plen;
		uint16_t sw;

		(void)aliro_apdu_strip_sw(payload, &body, &sw);

		struct aliro_auth0_response r0;
		struct aliro_auth1_response r1;

		(void)aliro_apdu_parse_auth0_response(payload, body, &r0);
		(void)aliro_apdu_parse_auth1_response(payload, body, &r1);
	}

	const uint8_t *val;
	size_t vlen;

	(void)aliro_tlv_find(data, size, ALIRO_TAG_DEVICE_PUBX, &val, &vlen);

	struct aliro_auth0_response raw0;
	struct aliro_auth1_response raw1;

	(void)aliro_apdu_parse_auth0_response(data, size, &raw0);
	(void)aliro_apdu_parse_auth1_response(data, size, &raw1);
	return 0;
}
