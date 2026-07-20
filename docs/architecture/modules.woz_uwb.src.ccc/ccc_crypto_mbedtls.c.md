<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/ccc/ccc_crypto_mbedtls.c`

@file ccc_crypto_mbedtls.c — AES-ECB block via mbedTLS, backing the CCC key schedule on SoCs
without a PSA provider (e.g. ESP32-S3).

**depends on** [`modules/woz_uwb/src/ccc/ccc_kdf.h`](ccc_kdf.h.md)  ·  **discussed in** [`docs/porting-esp32.md`](../../porting-esp32.md), [`docs/porting.md`](../../porting.md)

## API

### `int crypto_aes_ecb_encrypt(const uint8_t *key, size_t key_bits, const uint8_t in[16], uint8_t out[16])`
`modules/woz_uwb/src/ccc/ccc_crypto_mbedtls.c:17`

@brief Encrypt one AES-ECB block using mbedTLS, supporting 128-bit and 256-bit keys, as the
portable crypto seam selected by CONFIG_WOZ_CRYPTO_MBEDTLS (see docs/porting.md; same contract as
ccc_crypto_psa.c).
