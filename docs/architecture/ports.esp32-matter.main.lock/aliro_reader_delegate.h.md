<!-- generated documentation — edit the source, not this file -->
# `ports/esp32-matter/main/lock/aliro_reader_delegate.h`

Declares AliroReaderDelegate, the Aliro (Apple Home Key) reader-provisioning and BLE-UWB half of
the Matter DoorLock cluster delegate, bridging controller commands to the on-device reader
identity, trust store, and BLE advertising state.

**used by** [`ports/esp32-matter/main/app_main.cpp`](../ports.esp32-matter.main/app_main.cpp.md), [`ports/esp32-matter/main/lock/aliro_reader_delegate.cpp`](aliro_reader_delegate.cpp.md)

## API

### `class AliroReaderDelegate: public chip::app::Clusters::DoorLock::Delegate`
`ports/esp32-matter/main/lock/aliro_reader_delegate.h:51`

AliroReaderDelegate — the Aliro (Apple Home Key) reader-provisioning half of
the Door Lock cluster Delegate, for a BLE + UWB ("Express") reader.
Apple Home writes the reader identity with SetAliroReaderConfig (signing key,
verification key, group identifier, and — because kAliroBLEUWB is advertised —
the group resolving key) and reads the reader attributes back to confirm; this
delegate holds that config and advertises the expedited + BLE-UWB protocol
versions the reader speaks.
Aliro credential *keys* (issuer / endpoint, credential types 6/7/8) do NOT
flow through this Delegate. They arrive on the generic
emberAfPluginDoorLockSet/GetCredential path and are stored by BoltLockManager
(its storage is credential-type indexed and already sized to hold them), so
this class implements only the reader-config attributes plus the
"number of keys supported" getters the server consults to validate those
credential writes.
State is in-memory only: Apple provisions the reader within one setup session,
so a read-back within that session sees the values written. Persisting the
provisioned identity across reboot (so a Wallet key stays valid, and the UWB
reader can start after a power cycle) is wired into aliro_prov when the reader
components are merged in.
