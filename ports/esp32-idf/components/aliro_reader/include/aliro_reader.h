/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 *
 * aliro_reader — Aliro reader session/transaction layer (Phase 2.3+). Owns the
 * per-connection Aliro transaction on top of the aliro_ble transport: session
 * lifecycle, inbound message dispatch, diagnostics, and the hook that hands a
 * derived credential (URSK + ranging params) to the UWB engine.
 *
 * The M1-M4 cryptographic handshake and URSK derivation are Phase 3 and are
 * NOT implemented here; this layer observes/logs the transaction and marks the
 * exact seam where the crypto lands. See components/aliro_ble/SPEC.md.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Bring up the Aliro reader (starts the BLE transport + session layer).
 *  Returns 0 on success, negative on failure. */
int aliro_reader_start(void);

/* ---- Bench provisioning helpers (Phase 3.4) ---------------------------- *
 * Back the `aliro-prov` / `aliro-trust` console commands. Kept as plain calls
 * so the shell does not need the internal aliro_prov types. */

/** Print the reader identity (dev vs provisioned, reader_id), the trust store,
 *  and the most-recently-presented credential key. */
void aliro_reader_prov_print(void);

/** Trust the most-recently-presented credential public key and persist it to
 *  NVS. Returns 0 (added + saved), 1 (nothing presented yet, or already
 *  trusted), negative on a store error. */
int aliro_reader_trust_last(void);

#ifdef __cplusplus
}
#endif
