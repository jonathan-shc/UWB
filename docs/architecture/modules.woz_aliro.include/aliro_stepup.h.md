<!-- generated documentation — edit the source, not this file -->
# `modules/woz_aliro/include/aliro_stepup.h`

Aliro step-up (Access Document) phase: builds the mdoc DeviceRequest, unwraps and decrypts the
SessionData DeviceResponse, decodes the CBOR document per spec 7.2/8.4.2, and runs the six-step
Access Document verification of spec 7.4. Reference-completeness codec + verifier; the verdict is
logged and stored, never gates the unlock (the provisioned trust store remains the sole gate).

**depends on** [`modules/woz_aliro/include/aliro_crypto.h`](aliro_crypto.h.md)  ·  **used by** [`modules/woz_aliro/src/aliro_reader.c`](../modules.woz_aliro.src/aliro_reader.c.md), [`modules/woz_aliro/src/aliro_stepup.c`](../modules.woz_aliro.src/aliro_stepup.c.md), [`modules/woz_aliro/src/aliro_stepup_parse.c`](../modules.woz_aliro.src/aliro_stepup_parse.c.md)

## API

### `struct aliro_stepup_issuer`
`modules/woz_aliro/include/aliro_stepup.h:147`

---- verifier (§7.4) ----

### `struct aliro_stepup_job`
`modules/woz_aliro/include/aliro_stepup.h:201`

---- ESP worker seam (implemented per-platform; see aliro_stepup_worker.c) ----
Copies the collected SessionData response + keys + verify inputs and runs
aliro_stepup_run() off the BLE-host task, so parse/verify never touches the
auth segment or the ranging arm window. Returns 0 if queued.

<details><summary>Undocumented (5)</summary>

- `aliro_stepup_digest`
- `aliro_stepup_item`
- `aliro_stepup_doc`
- `aliro_stepup_verify_ctx`
- `aliro_stepup_verdict`

</details>
