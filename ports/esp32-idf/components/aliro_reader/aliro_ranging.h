/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 *
 * aliro_ranging — the post-auth Aliro UWB ranging-setup (M1-M4) on ESP32. Thin
 * glue that drives the engine's reader adapter/session (modules/woz_uwb aliro_uwb_
 * adapter + session): on a completed credential-auth it creates a session bound to
 * the derived URSK, emits M1 over the BLE L2CAP channel, feeds inbound M2/M4 back
 * to the engine, and lets the engine negotiate the ranging parameters and start
 * the DW3000 responder itself (via cherry_ccc_shim -> woz_uwb_start_aliro). This
 * replaces the earlier canned-parameter handoff.
 *
 * The DW3000 is single-session, so exactly one ranging session runs at a time.
 * The whole lifecycle stays on the BLE-host task (create/feed/teardown + the
 * engine's transmit/event callbacks are all synchronous), so no locking is needed.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** One-time bring-up of the reader UWB adapter (cherry ctx + capabilities +
 *  reader config). Idempotent. Returns 0 on success, negative on failure (the
 *  reader still runs; ranging just won't start). */
int aliro_ranging_init(void);

/** Start the M1-M4 ranging setup for a connection: create the session bound to
 *  the 32-byte @ursk and emit M1 now (over the peer's L2CAP channel). Returns 0
 *  if M1 was sent, negative on failure or if a ranging session is already active
 *  (the DW3000 is single-session). */
int aliro_ranging_start(uint16_t conn_handle, const uint8_t *ursk);

/** Feed one inbound post-auth SDU (M2/M4/notification, raw bytes with its own
 *  4-byte header) to the active ranging session. M4 makes the engine start the
 *  responder with the negotiated parameters. Returns 0 if consumed, negative if
 *  there is no active session for this connection or the engine rejected it. */
int aliro_ranging_feed(uint16_t conn_handle, const uint8_t *data, size_t len);

/** Tear down the ranging session for a connection (on disconnect). No-op if none
 *  is active for @conn_handle. */
void aliro_ranging_stop(uint16_t conn_handle);

#ifdef __cplusplus
}
#endif
