<!-- generated documentation — edit the source, not this file -->
# `ports/esp32-idf/components/aliro_crypto/include/aliro_crypto.h`

Aliro crypto public API: key derivation, AES-GCM secure channels, and wire message
seal/open framing shared by the reader and device sides of an Aliro session.

**used by** [`ports/esp32-idf/components/aliro_crypto/src/aliro_crypto.c`](../ports.esp32-idf.components.aliro_crypto.src/aliro_crypto.c.md), [`ports/esp32-idf/components/aliro_reader/aliro_ranging.c`](../ports.esp32-idf.components.aliro_reader/aliro_ranging.c.md), [`ports/esp32-idf/components/aliro_reader/aliro_reader.c`](../ports.esp32-idf.components.aliro_reader/aliro_reader.c.md)

## API

### `int aliro_msg_open(struct aliro_secchan *sc, const uint8_t *wire, size_t wire_len, uint8_t *plain, size_t plain_cap, size_t *plain_len)`
`ports/esp32-idf/components/aliro_crypto/include/aliro_crypto.h:150`

Inverse of aliro_msg_seal: open a wire SDU into the engine plaintext form,
verifying the tag. Returns <0 on a tag mismatch (drop the connection then).
