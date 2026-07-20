<!-- generated documentation — edit the source, not this file -->
# `ports/esp32-idf/components/aliro_reader/aliro_prov.c`

Aliro reader provisioning state: default dev identity, and serialization/deserialization of the
reader identity plus trusted-credential store to/from a self-describing binary blob.
Also implements the trust-store membership check and add-with-dedup operations used to decide
whether a presented credential public key is trusted.

**depends on** [`ports/esp32-idf/components/aliro_reader/aliro_prov.h`](aliro_prov.h.md)

## API

### `void aliro_prov_dev_default(struct aliro_reader_identity *id, struct aliro_trust_store *ts)`
`ports/esp32-idf/components/aliro_reader/aliro_prov.c:43`

Populate the built-in clearly-marked dev identity + an empty trust store.

### `int aliro_prov_serialize(const struct aliro_reader_identity *id, const struct aliro_trust_store *ts, uint8_t *out, size_t cap, size_t *out_len)`
`ports/esp32-idf/components/aliro_reader/aliro_prov.c:57`

Serialise identity+trust to a self-describing blob. 0 + *out_len on success,
-1 on overflow (cap < the assembled length).

### `int aliro_prov_deserialize(const uint8_t *buf, size_t len, struct aliro_reader_identity *id, struct aliro_trust_store *ts)`
`ports/esp32-idf/components/aliro_reader/aliro_prov.c:99`

Parse a blob written by aliro_prov_serialize. 0 on success; -1 if malformed
(bad magic/version/length/count). Outputs are untouched on failure.

### `struct aliro_reader_identity *id,`
`ports/esp32-idf/components/aliro_reader/aliro_prov.c:100`

The reader's provisioned identity. reader_id rides AUTH0 and both ECDSA
transcripts (tag 0x4D); sign_priv signs the reader-usage transcript. is_dev
marks the built-in bench identity, never a real deployment.

### `int aliro_prov_trust_check(const struct aliro_trust_store *ts, const uint8_t cred_pub[ALIRO_CRED_PUB_LEN])`
`ports/esp32-idf/components/aliro_reader/aliro_prov.c:158`

Trust decision for a presented credential public key:
0  trusted    (cred_pub matches a stored key)
1  no-anchors (store empty; caller applies dev-open policy)
-1  rejected   (store non-empty and no match).

**called by** `aliro_prov_trust_add`

### `int aliro_prov_trust_add(struct aliro_trust_store *ts, const uint8_t cred_pub[ALIRO_CRED_PUB_LEN])`
`ports/esp32-idf/components/aliro_reader/aliro_prov.c:172`

Trusted credential public keys. A presented credential authenticates only if
its key is in here (or the store is empty and dev policy allows it). A raw-key
allowlist is the interim seam; real issuer-chain validation is the Phase-4
refinement that plugs in at aliro_prov_trust_check.

### `int aliro_prov_trust_add(struct aliro_trust_store *ts, const uint8_t cred_pub[ALIRO_CRED_PUB_LEN])`
`ports/esp32-idf/components/aliro_reader/aliro_prov.c:172`

Add a credential key to the store. 0 added; 1 already present (dedup); -1 full
or the point is not an uncompressed P-256 point (leading byte != 0x04).

**calls** `aliro_prov_trust_check`
