<!-- generated documentation — edit the source, not this file -->
# `ports/esp32-idf/components/aliro_reader/aliro_apdu.h`

APDU framing and parsing for the Aliro Access Protocol: builds outbound command APDUs via a
TLV writer and parses the AUTH0/AUTH1 response APDUs exchanged during the reader-device
handshake.

**used by** [`ports/esp32-idf/components/aliro_reader/aliro_apdu.c`](aliro_apdu.c.md), [`ports/esp32-idf/components/aliro_reader/aliro_reader.c`](aliro_reader.c.md)

## API

### `int aliro_tlv_w_finish(struct aliro_tlv_w *w, size_t *out_len)`
`ports/esp32-idf/components/aliro_reader/aliro_apdu.h:72`

zero length

### `struct aliro_auth0_response`
`ports/esp32-idf/components/aliro_reader/aliro_apdu.h:123`

Fields parsed from an AUTH0Response APDU: the device's mandatory ephemeral
public key, plus the optional cryptogram sent when the device recognises the
reader and offers the fast path.

### `struct aliro_auth1_response`
`ports/esp32-idf/components/aliro_reader/aliro_apdu.h:135`

Fields parsed from an AUTH1Response APDU: the device's mandatory signature
over the transcript, plus the device public key and signaling bitmap it sends
when the standard (non-fast) path is taken.
