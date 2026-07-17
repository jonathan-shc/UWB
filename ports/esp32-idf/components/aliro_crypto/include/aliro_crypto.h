/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 *
 * aliro_crypto — the credential-auth cryptography for the Aliro reader: the
 * P-256 / AES-256-GCM / SHA-256 suite and the key-derivation schedule that
 * turns a passed credential authentication into the 32-byte URSK (the UWB
 * ranging root) plus the secure-channel keys.
 *
 * This is Phase 3.1: the primitives and the key schedule, host-KAT verifiable.
 * The transaction state machine that drives it (AUTH0/AUTH1, EXCHANGE) is 3.2;
 * the handoff of the URSK into woz_uwb_start_aliro(cfg) is 3.3.
 *
 * Provenance: clean-room. The suite is standard (mbedTLS-PSA + a portable
 * SHA-256/KDF core). The wire/derivation facts come from the project's
 * reverse-engineering notes, not from any vendor source.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALIRO_URSK_LEN        32u
#define ALIRO_KEY_BLOCK_LEN   160u /* full derived block */
#define ALIRO_URSK_OFFSET     128u /* URSK = block[128 .. 159] */
#define ALIRO_SESSION_KEY_LEN 32u
#define ALIRO_SHARED_SECRET_LEN 32u /* ECDH X coordinate */
#define ALIRO_TXID_LEN        16u
#define ALIRO_EC_PUBX_LEN     32u   /* an EC point's X coordinate */
#define ALIRO_GCM_NONCE_LEN   12u
#define ALIRO_GCM_TAG_LEN     16u

/* Initialise the crypto backend (idempotent). 0 on success, negative on fail. */
int aliro_crypto_init(void);

/* Extract the 32-byte URSK from the derived 160-byte key block. */
void aliro_crypto_ursk_from_block(const uint8_t block[ALIRO_KEY_BLOCK_LEN],
				  uint8_t ursk[ALIRO_URSK_LEN]);

/*
 * ---- Credential-auth key schedule (standard/ECDH path) -------------------
 *
 * Stage 1: Z = SHA-256( shared_secret(32) | 0x00000001 | txid(16) ), the
 * single-block ANSI-X9.63 concat KDF over the raw ECDH shared secret. Z is the
 * IKM for every stage-2 HKDF.
 */
void aliro_crypto_derive_z(const uint8_t shared_secret[ALIRO_SHARED_SECRET_LEN],
			   const uint8_t txid[ALIRO_TXID_LEN], uint8_t z[32]);

/*
 * Stage 2: block = HKDF-SHA256(salt, IKM=z, info=device_pub_x(32), L=160). The
 * salt is the CreateSalt transcript (aliro_salt_build). Returns 0 on success.
 */
int aliro_crypto_derive_block(const uint8_t z[32], const uint8_t *salt,
			      size_t salt_len,
			      const uint8_t device_pub_x[ALIRO_EC_PUBX_LEN],
			      uint8_t block[ALIRO_KEY_BLOCK_LEN]);

/*
 * Single 32-byte keyed derivation off z: Kpersistent (salt type 2) and the
 * Auth1 cryptogram key (salt type 0) both use this, differing only in the salt.
 * block = HKDF-SHA256(salt, IKM=z, info=device_pub_x, L=32). Returns 0 on ok.
 */
int aliro_crypto_derive_key32(const uint8_t z[32], const uint8_t *salt,
			      size_t salt_len,
			      const uint8_t device_pub_x[ALIRO_EC_PUBX_LEN],
			      uint8_t out[32]);

/*
 * Split the 160-byte block into the two directional session keys + URSK. The
 * reference derives up to two optional "shared" keys (C/D) alongside; a config
 * flag shifts which segments are the symmetric keys. with_c=1: enc=S0, dec=S1;
 * with_c=0: enc=S1, dec=S2 (S0 unused). URSK = S4 (offset 128) either way.
 */
void aliro_crypto_split(const uint8_t block[ALIRO_KEY_BLOCK_LEN], int with_c,
			uint8_t enc_key[ALIRO_SESSION_KEY_LEN],
			uint8_t dec_key[ALIRO_SESSION_KEY_LEN],
			uint8_t ursk[ALIRO_URSK_LEN]);

/*
 * ---- Secure channel (AES-256-GCM, directional per-message counters) ------
 *
 * Nonce = 8-byte big-endian direction (0 outbound/seal, 1 inbound/open) followed
 * by a 4-byte big-endian per-direction counter. Separate seal/open counters,
 * start at 0, no wrap. SessionCrypto sends no AAD; the BLE channel authenticates
 * a 4-byte AAD (caller-supplied here).
 */
struct aliro_secchan {
	uint8_t enc_key[ALIRO_SESSION_KEY_LEN];
	uint8_t dec_key[ALIRO_SESSION_KEY_LEN];
	uint32_t enc_ctr;
	uint32_t dec_ctr;
};

void aliro_secchan_init(struct aliro_secchan *sc,
			const uint8_t enc_key[ALIRO_SESSION_KEY_LEN],
			const uint8_t dec_key[ALIRO_SESSION_KEY_LEN]);
void aliro_crypto_gcm_nonce(uint64_t direction, uint32_t counter,
			    uint8_t nonce[ALIRO_GCM_NONCE_LEN]);
/* Seal/open advance the matching counter on success. Return 0 on success;
 * open returns <0 on a tag mismatch (hard auth failure) — never trust the
 * plaintext then. */
int aliro_secchan_seal(struct aliro_secchan *sc, const uint8_t *aad,
		       size_t aad_len, const uint8_t *pt, size_t pt_len,
		       uint8_t *ct, uint8_t tag[ALIRO_GCM_TAG_LEN]);
int aliro_secchan_open(struct aliro_secchan *sc, const uint8_t *aad,
		       size_t aad_len, const uint8_t *ct, size_t ct_len,
		       const uint8_t tag[ALIRO_GCM_TAG_LEN], uint8_t *pt);

/*
 * ---- CreateSalt transcript builder --------------------------------------
 *
 * Reproduces the recovered append order for the stage-2 HKDF salt. The domain
 * label and the fixed 0x5c02 constant are exact; the assignment of the 32-byte
 * spans and the widths of the negotiated-version fields are still being pinned
 * (they are populated by the transaction state machine, Phase 3.2), so treat
 * this as the interop seam to confirm at bench. Returns 0 on success, and the
 * assembled length in *out_len.
 */
enum aliro_salt_type {
	ALIRO_SALT_CRYPTOGRAM = 0, /* label "VolatileFast" */
	ALIRO_SALT_SESSION = 1,    /* label "Volatile****" */
	ALIRO_SALT_KPERSISTENT = 2 /* label "Persistent**" */
};

#define ALIRO_SALT_MAX 192u

int aliro_salt_build(enum aliro_salt_type type, const uint8_t txid[ALIRO_TXID_LEN],
		     const uint8_t span_s1[ALIRO_EC_PUBX_LEN],
		     const uint8_t reader_value[ALIRO_EC_PUBX_LEN],
		     const uint8_t reader_id[32], uint16_t proto_version_reader,
		     uint16_t proto_version_device,
		     const uint8_t s3opt[ALIRO_EC_PUBX_LEN] /* NULL for type 1 */,
		     uint8_t *out, size_t *out_len);

#ifdef __cplusplus
}
#endif
