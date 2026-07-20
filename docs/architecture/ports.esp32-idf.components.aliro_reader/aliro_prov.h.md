<!-- generated documentation — edit the source, not this file -->
# `ports/esp32-idf/components/aliro_reader/aliro_prov.h`

Persistent reader provisioning storage: identity and credential trust anchors saved to and
loaded from NVS.
Declares aliro_prov_store for committing an identity/trust pair to NVS, and struct
aliro_trust_store, the set of trusted credential public keys against which a presented
credential is authenticated.

**used by** [`ports/esp32-idf/components/aliro_reader/aliro_prov.c`](aliro_prov.c.md), [`ports/esp32-idf/components/aliro_reader/aliro_prov_nvs.c`](aliro_prov_nvs.c.md), [`ports/esp32-idf/components/aliro_reader/aliro_reader.c`](aliro_reader.c.md)

## API

### `int aliro_prov_store(const struct aliro_reader_identity *id, const struct aliro_trust_store *ts)`
`ports/esp32-idf/components/aliro_reader/aliro_prov.h:108`

Persist identity+trust to NVS. 0 on success, negative on an NVS error.

### `struct aliro_trust_store`
`ports/esp32-idf/components/aliro_reader/aliro_prov.h:109`

Trusted credential public keys. A presented credential authenticates only if
its key is in here (or the store is empty and dev policy allows it). A raw-key
allowlist is the interim seam; real issuer-chain validation is the Phase-4
refinement that plugs in at aliro_prov_trust_check.
