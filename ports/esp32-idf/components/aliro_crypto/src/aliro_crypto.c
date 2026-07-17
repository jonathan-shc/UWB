/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 *
 * aliro_crypto — credential-auth key schedule. See aliro_crypto.h.
 *
 * The KDF chain (ECDH/Kpersistent -> 160-byte block -> URSK + secure-channel
 * keys) is built on the portable SHA-256/HKDF/X9.63 core (aliro_hash.c) and the
 * AEAD/EC primitives (aliro_prim.h). The exact transcript inputs and labels are
 * filled in from the reverse-engineered derivation facts.
 */
#include "aliro_crypto.h"

#include <string.h>

#include "aliro_hash.h"
#include "aliro_prim.h"

int aliro_crypto_init(void)
{
	return aliro_prim_init();
}

void aliro_crypto_ursk_from_block(const uint8_t block[ALIRO_KEY_BLOCK_LEN],
				  uint8_t ursk[ALIRO_URSK_LEN])
{
	memcpy(ursk, block + ALIRO_URSK_OFFSET, ALIRO_URSK_LEN);
}

/* ---- key schedule ---- */

void aliro_crypto_derive_z(const uint8_t shared_secret[ALIRO_SHARED_SECRET_LEN],
			   const uint8_t txid[ALIRO_TXID_LEN], uint8_t z[32])
{
	/* Single-block X9.63: SHA-256( shared_secret | 00000001 | txid ). */
	aliro_x963_kdf(shared_secret, ALIRO_SHARED_SECRET_LEN, txid, ALIRO_TXID_LEN,
		       z, 32);
}

int aliro_crypto_derive_block(const uint8_t z[32], const uint8_t *salt,
			      size_t salt_len,
			      const uint8_t device_pub_x[ALIRO_EC_PUBX_LEN],
			      uint8_t block[ALIRO_KEY_BLOCK_LEN])
{
	/* HKDF-SHA256(salt, IKM=z, info=device_pub_x, L=160). */
	return aliro_hkdf(salt, salt_len, z, 32, device_pub_x, ALIRO_EC_PUBX_LEN,
			  block, ALIRO_KEY_BLOCK_LEN);
}

int aliro_crypto_derive_key32(const uint8_t z[32], const uint8_t *salt,
			      size_t salt_len,
			      const uint8_t device_pub_x[ALIRO_EC_PUBX_LEN],
			      uint8_t out[32])
{
	return aliro_hkdf(salt, salt_len, z, 32, device_pub_x, ALIRO_EC_PUBX_LEN,
			  out, 32);
}

void aliro_crypto_split(const uint8_t block[ALIRO_KEY_BLOCK_LEN], int with_c,
			uint8_t enc_key[ALIRO_SESSION_KEY_LEN],
			uint8_t dec_key[ALIRO_SESSION_KEY_LEN],
			uint8_t ursk[ALIRO_URSK_LEN])
{
	/* Five 32-byte segments S0..S4. The config flag "derive shared key C"
	 * shifts the base of the two directional keys: with C -> S0/S1, without
	 * C -> S1/S2. URSK is S4 (offset 128) unconditionally. */
	size_t base = with_c ? 0u : 32u;

	memcpy(enc_key, block + base, ALIRO_SESSION_KEY_LEN);
	memcpy(dec_key, block + base + 32u, ALIRO_SESSION_KEY_LEN);
	memcpy(ursk, block + ALIRO_URSK_OFFSET, ALIRO_URSK_LEN);
}

/* ---- secure channel ---- */

void aliro_secchan_init(struct aliro_secchan *sc,
			const uint8_t enc_key[ALIRO_SESSION_KEY_LEN],
			const uint8_t dec_key[ALIRO_SESSION_KEY_LEN])
{
	memcpy(sc->enc_key, enc_key, ALIRO_SESSION_KEY_LEN);
	memcpy(sc->dec_key, dec_key, ALIRO_SESSION_KEY_LEN);
	sc->enc_ctr = 0;
	sc->dec_ctr = 0;
}

void aliro_crypto_gcm_nonce(uint64_t direction, uint32_t counter,
			    uint8_t nonce[ALIRO_GCM_NONCE_LEN])
{
	/* 8-byte BE direction || 4-byte BE counter. */
	for (int i = 0; i < 8; i++) {
		nonce[i] = (uint8_t)(direction >> (56 - i * 8));
	}
	nonce[8] = (uint8_t)(counter >> 24);
	nonce[9] = (uint8_t)(counter >> 16);
	nonce[10] = (uint8_t)(counter >> 8);
	nonce[11] = (uint8_t)counter;
}

int aliro_secchan_seal(struct aliro_secchan *sc, const uint8_t *aad,
		       size_t aad_len, const uint8_t *pt, size_t pt_len,
		       uint8_t *ct, uint8_t tag[ALIRO_GCM_TAG_LEN])
{
	uint8_t nonce[ALIRO_GCM_NONCE_LEN];
	int rc;

	if (sc->enc_ctr == 0xffffffffu) {
		return -1; /* counter exhausted: no wrap */
	}
	aliro_crypto_gcm_nonce(0, sc->enc_ctr, nonce);
	rc = aliro_aes256_gcm_encrypt(sc->enc_key, nonce, sizeof(nonce), aad,
				      aad_len, pt, pt_len, ct, tag,
				      ALIRO_GCM_TAG_LEN);
	if (rc == 0) {
		sc->enc_ctr++;
	}
	return rc;
}

int aliro_secchan_open(struct aliro_secchan *sc, const uint8_t *aad,
		       size_t aad_len, const uint8_t *ct, size_t ct_len,
		       const uint8_t tag[ALIRO_GCM_TAG_LEN], uint8_t *pt)
{
	uint8_t nonce[ALIRO_GCM_NONCE_LEN];
	int rc;

	if (sc->dec_ctr == 0xffffffffu) {
		return -1;
	}
	aliro_crypto_gcm_nonce(1, sc->dec_ctr, nonce);
	rc = aliro_aes256_gcm_decrypt(sc->dec_key, nonce, sizeof(nonce), aad,
				      aad_len, ct, ct_len, tag, ALIRO_GCM_TAG_LEN,
				      pt);
	if (rc == 0) {
		sc->dec_ctr++;
	}
	return rc;
}

/* ---- CreateSalt transcript (provisional layout — see header) ---- */

/* 36-byte domain label, sliced into three 12-byte labels by salt type. The
 * trailing NUL (index 36) is never read: slices span index 0..35. */
static const char k_salt_label[] =
	"VolatileFastVolatile\x2a\x2a\x2a\x2aPersistent\x2a\x2a";
static const uint8_t k_salt_const[2] = { 0x5c, 0x02 };

static int append(uint8_t *out, size_t *pos, size_t cap, const void *src, size_t n)
{
	if (*pos + n > cap) {
		return -1;
	}
	memcpy(out + *pos, src, n);
	*pos += n;
	return 0;
}

int aliro_salt_build(enum aliro_salt_type type, const uint8_t txid[ALIRO_TXID_LEN],
		     const uint8_t span_s1[ALIRO_EC_PUBX_LEN],
		     const uint8_t reader_value[ALIRO_EC_PUBX_LEN],
		     const uint8_t reader_id[32], uint16_t proto_version,
		     uint8_t exp_phase_type, uint8_t user_auth_policy,
		     const uint8_t s3opt[ALIRO_EC_PUBX_LEN], uint8_t *out,
		     size_t *out_len)
{
	size_t pos = 0;
	uint8_t ver[2] = { (uint8_t)(proto_version >> 8), (uint8_t)proto_version };
	uint8_t policy[2] = { exp_phase_type, user_auth_policy };
	int rc = 0;

	/* Confirmed append order. Item 6 = the single negotiated protocol version
	 * (big-endian); item 9 = the expedited-phase-type + user-auth-policy bytes.
	 * The two still-unresolved Salt sub-fields (items 4 and 10) are omitted —
	 * the interop seam to confirm at bench. */
	rc |= append(out, &pos, ALIRO_SALT_MAX, span_s1, ALIRO_EC_PUBX_LEN);       /* 1 */
	rc |= append(out, &pos, ALIRO_SALT_MAX,
		     k_salt_label + (size_t)type * 12u, 12);                      /* 2 */
	rc |= append(out, &pos, ALIRO_SALT_MAX, reader_id, 32);                    /* 3 */
	rc |= append(out, &pos, ALIRO_SALT_MAX, k_salt_const, sizeof(k_salt_const)); /* 5 */
	rc |= append(out, &pos, ALIRO_SALT_MAX, ver, sizeof(ver));                 /* 6 */
	rc |= append(out, &pos, ALIRO_SALT_MAX, reader_value, ALIRO_EC_PUBX_LEN);  /* 7 */
	rc |= append(out, &pos, ALIRO_SALT_MAX, txid, ALIRO_TXID_LEN);            /* 8 */
	rc |= append(out, &pos, ALIRO_SALT_MAX, policy, sizeof(policy));           /* 9 */
	if (type != ALIRO_SALT_SESSION && s3opt != NULL) {                        /* 11 */
		rc |= append(out, &pos, ALIRO_SALT_MAX, s3opt, ALIRO_EC_PUBX_LEN);
	}
	if (rc != 0) {
		return -1;
	}
	*out_len = pos;
	return 0;
}
