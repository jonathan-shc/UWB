# openaliro on ESP32-S3: porting roadmap

Status: planning, grounded in the working Nordic add-on source (studied read-only from the
bootstrapped `ncs-door-lock-and-access-control` checkout). No ESP32 code committed yet. This is the
agreed architecture and de-risking order.

Target hardware on hand: ESP32-S3-WROOM (N16R8, 16 MB flash / 8 MB octal PSRAM, Wi-Fi + BLE,
no 802.15.4). Still to source: a DWM3000EVB (UWB) and an ST25R-class NFC reader.

## 1. Summary

This is not a spec reverse-engineering project. The DIY Nordic lock already provisions a credential
into Apple Wallet today, and the reader that does it is readable, layered Nordic source: the
`subsys/aliro/` subsystem (about 12 components, ~4.4k lines of the relevant parts) plus the
app-side `src/aliro/` (~5.1k lines). The port is reimplementing that ~10k lines of C++ on ESP-IDF,
tiered by how portable each piece is. The genuine rewrite is small and concentrated: ~650 lines of
BLE transport (Zephyr Bluetooth to NimBLE). Everything else is either common-SDK, PSA-portable, or
already seam-defined.

Decision: **ESP-IDF + esp-matter**. First real engineering is the UWB engine on silicon (Phase 1,
fully unblocked). The old "will Apple provision a DIY lock" gate is closed by the working nRF.

Constraint held throughout: **the working nRF5340DK stays untouched** (observe-only reference; no
reflash, reconfigure, reprovision, or key readout, and no connection to it without sign-off).

Licensing overlay (important): the add-on is `LicenseRef-Nordic-5-Clause`, which restricts use to
Nordic devices. The source is a reference to study, not to lift onto ESP32. A publishable port must
be clean-room reimplemented (reimplement behavior, do not copy files), matching the provenance
discipline already used in this project.

## 2. Port map (grounded in the add-on source)

The engine seam is confirmed. Nordic's Aliro reader talks to UWB only through the `UltraWideBand`
C++ interface (`subsys/aliro/uwb/.../uwb.h`), and this repo's engine already implements it (the
in-repo `integration/patches/custom_impl-uwb.patch` fills `_ConfigureRangingSession(sessionId,
ursk, ...)` to call `aliro_uwb_session_set_ursk`, and `_HandleBleMessage(...)` to route the Aliro
UWB messages, backed by `woz_uwb_facade`). So on ESP32 the engine keeps implementing the same
interface; the reader above it is what gets rebuilt.

Portability tiers (line counts are the Nordic reference, not a copy target):

| Component | ~Lines | Tier | ESP-IDF plan |
|---|---|---|---|
| door_lock_delegate + Matter Aliro cluster | 330 + SDK | A: common SDK | connectedhomeip is shared by NCS and esp-matter, so the Matter/provisioning side ports with modest change. This is why DIY Wallet provisioning already works. |
| crypto_utils + app crypto seam | 327 + part of 1311 | A: PSA-portable | Uses the PSA Crypto API (`psa/crypto.h`). ESP-IDF mbedTLS provides PSA; port at the API level, backend moves CC312 -> mbedTLS. |
| aliro_service (orchestration) | 975 | B: logic + seams | Portable C++; swap Zephyr workqueue/storage seams. |
| access_manager, disambiguation, access_document | 2171 + 352 + hdr | B: logic + seams | Credential matching / access decision logic; mostly portable. |
| reader_storage, storage, kpersistent_manager | 1374 + 654 + 329 | B: storage seam | Zephyr settings/NVS -> ESP-IDF NVS. |
| aliro_workqueue, time_utils, lock_sim | 83 + 95 + 245 | B: small glue | Zephyr workqueue/time -> FreeRTOS/esp_timer; lock_sim trivial. |
| gatt_server + l2cap_server | 354 + 300 | C: real rewrite | Zephyr `bluetooth/gatt.h` + L2CAP CoC -> NimBLE GATT + L2CAP CoC. The Aliro BLE transaction transport. The dominant rewrite, but only ~650 lines. |
| uwb/custom_impl over woz_uwb | small + engine | D: done + engine port | Keep the `UltraWideBand` impl; port the engine per section 4. Do NOT port `uwb/qm35_impl` (the Qorvo coprocessor path, ~3k lines, unused here). |
| platform/nfc (RFAL + ECP) | 647 | E: use esp-aliro | Espressif ships **esp-aliro** (github.com/espressif/esp-aliro), first-party Aliro-over-NFC on ESP32, license-clean. Use it for the tap applet instead of a clean-room RFAL reimplementation. NFC-only today; BLE + UWB are on its roadmap. |

Take-away: the frightening parts (provisioning, crypto) are the portable ones (common
connectedhomeip; PSA). The concentrated new work is BLE transport on NimBLE.

## 3. Decision: ESP-IDF, not Zephyr

Apple Home + Wallet requires Matter (no sideload path to Wallet). On ESP32-S3 (no 802.15.4) that is
Matter over Wi-Fi, which Apple Home accepts; the transport does not matter to Apple (the nRF used
Thread). esp-matter is the only viable ESP32 Matter path, and it shares connectedhomeip with NCS,
so the door_lock_delegate + Aliro cluster (Tier A) carries over. ESP-IDF also gives native NimBLE
(the L2CAP transaction), mbedTLS-PSA (the crypto), NVS (storage), FreeRTOS core-pinning, and PSRAM.
Espressif now also ships first-party Aliro (esp-aliro, NFC today; BLE + UWB roadmapped), which only
strengthens ESP-IDF over Zephyr.

### Toolchain (verified July 2026)

- Install **ESP-IDF v5.5.4**, not the newest. esp-matter recommends v5.5.4; ESP-IDF v6.0 exists but
  would break esp-matter. Pinning v5.5.4 lets Phase 1 (plain ESP-IDF, version-flexible) and the
  later Matter/Aliro phases (esp-matter, version-strict) share one toolchain, installed once.
- Stage the install: for Phase 1, plain ESP-IDF v5.5.4 with the `esp32s3` target is enough (a few
  GB). Add esp-matter + esp-aliro on top of the same v5.5.4 later, for Phases 4-5 (much larger;
  connectedhomeip is heavy). Confirm esp-aliro's own ESP-IDF pin + license from its repo before
  Phase 5; it is expected to align with esp-matter (verify).

## 4. Engine port surface (Phase 1, concrete)

The UWB engine (`modules/woz_uwb`) already compiles as pure C on host (`tests/host/sources.sh`,
13 core files). Minimal ESP-IDF port:

- OS seam: extend `modules/woz_uwb/src/facade/woz_alloc.h` (already wraps `k_malloc` and monotonic
  microseconds) into a `woz_os` seam with a FreeRTOS backend. Real call sites are few:
  `uwb_min.c` (`k_uptime_get` busy-wait deadlines ~188-307, `k_msleep` 31/122/362) and
  `fira_session.c` (`k_uptime_get` 71/125). `k_work`/`k_timer` uses are diagnostics/self-test/log
  only (`uwb_rxdiag.c`, `uwb_selftest.c`, `woz_logfmt.c`) and stub or defer.
- DW3000 platform shim, rewrite on ESP-IDF SPI-master + GPIO: `deps/dw3000/platform/dw3000_spi.c`
  (241), `dw3000_hw.c` (298), `deca_port.c` (60). Leave `deca_compat.c` (1352, vendor) logic intact.
- Link seam: the 5 `-Wl,--wrap=dwt_*` flags (`modules/woz_uwb/CMakeLists.txt`) via
  `target_link_options`. Only `--wrap=dwt_rxenable` is load-bearing (`ccc_shim_rx.c` programs the
  CCC key/IV on every RX-arm); the rest are diagnostics or have no live caller.
  `xtensa-esp32s3-elf-ld` is GNU binutils; verify `--wrap` once.
- Crypto seam: `ccc_crypto_mbedtls.c` (already selected by the existing scaffold's `prj.conf`).
- Placement: pin engine + DW3000 SPI to core 1, BLE/Wi-Fi on core 0. Hot buffers in internal SRAM,
  not PSRAM (jitter).

## 5. Phased plan

- Phase 1 (unblocked, no reader dependency): engine to ESP-IDF (section 4), canned URSK, range vs a
  second board. Then hold an idle NimBLE connection on core 0 and re-measure ranging on core 1 to
  confirm coexistence early. Proves the crown jewel on your board.
- Phase 2 (BLE transport, the real rewrite): reimplement gatt_server + l2cap_server on NimBLE
  (GATT + L2CAP CoC) as the Aliro transaction channel. Drive it with captured/spec message flows.
- Phase 3 (reader logic + storage): port aliro_service, access_manager, disambiguation,
  reader_storage, crypto_utils (PSA), workqueue/time on ESP-IDF. On auth completion the derived
  URSK enters the engine through the existing `UltraWideBand` / `woz_uwb_start_aliro()` seam.
- Phase 4 (Matter provisioning): esp-matter door lock over Wi-Fi + the Aliro cluster +
  door_lock_delegate, so a fresh ESP32 lock self-commissions into Apple Home and self-provisions
  into Wallet (Tier A, common SDK).
- Phase 5 (NFC tap): integrate esp-aliro (first-party Aliro-over-NFC) rather than reimplementing
  RFAL. Reader hardware follows esp-aliro's example (verify which IC it uses before buying).

Phases 2-4 can proceed in parallel with each other once Phase 1 is up, since the BLE transport,
reader logic, and Matter layer are separable.

## 6. Hardware bill of materials

- ESP32-S3-WROOM N16R8 dev board (on hand).
- DWM3000EVB (DW3110) for UWB on FSPI (SPI2). Reuse the pin choices in `ports/esp32s3/dw3000.overlay`.
- ST25R-class NFC reader (ST25R3916 / ST25R300). Assumption: PN532-class parts are too limited for
  Apple Express / ECP timing; the reference uses ST25R. Verify against the chosen part.
- Note: Matter-over-Wi-Fi on S3 is not low-power like the nRF Thread sleepy-end-device. Fine for a
  mains-powered reader, wrong for a battery lock.

## 7. Risks and assumptions

- Verified: the Aliro Door Lock cluster is in connectedhomeip (the `SetAliroReaderConfig` /
  `ClearAliroReaderConfig` commands and the ALIRO feature flag), so esp-matter has it and Tier A
  carries over. Provisioning "from any wallet" via any Matter controller is the documented design.
- Assumption (Likely, verify): NimBLE's L2CAP CoC + GATT cover what the Aliro BLE transaction needs.
- Assumption (Likely, verify): ESP-IDF mbedTLS PSA covers the crypto_utils PSA usage (ECDH, key
  import/export/derive) that the reader needs.
- Assumption: `--wrap` on `xtensa-esp32s3-elf-ld` behaves as on any GNU ld.
- Coexistence (one S3 doing BLE + UWB) holds: the nRF already runs engine + BLE host on one core,
  and UWB is a separate radio, so only CPU/SPI/IRQ time is shared. Confirmed by the Phase 1 spike.
- Licensing: the Nordic reference is study-only (Nordic-device restriction). Effort must be
  clean-room reimplementation, which is slower than lifting but is the only publishable path.
