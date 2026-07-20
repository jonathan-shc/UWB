<!-- generated documentation — edit the source, not this file -->
# `ports/esp32-idf/components/aliro_reader/`

| subsystem | about |
|---|---|
| [`ports/esp32-idf/components/aliro_reader/aliro_apdu.c`](aliro_apdu.c.md) | Aliro APDU TLV codec: builds command payloads (AUTH0, AUTH1, AuthData, EXCHANGE) and parses |
| [`ports/esp32-idf/components/aliro_reader/aliro_apdu.h`](aliro_apdu.h.md) | APDU framing and parsing for the Aliro Access Protocol: builds outbound command APDUs via a |
| [`ports/esp32-idf/components/aliro_reader/aliro_prov.c`](aliro_prov.c.md) | Aliro reader provisioning state: default dev identity, and serialization/deserialization of the |
| [`ports/esp32-idf/components/aliro_reader/aliro_prov.h`](aliro_prov.h.md) | Persistent reader provisioning storage: identity and credential trust anchors saved to and |
| [`ports/esp32-idf/components/aliro_reader/aliro_prov_nvs.c`](aliro_prov_nvs.c.md) | NVS-backed persistence for Aliro reader provisioning: loads and stores the serialized reader |
| [`ports/esp32-idf/components/aliro_reader/aliro_ranging.c`](aliro_ranging.c.md) | UWB ranging bring-up and lifecycle for the Aliro reader: initializes the reader's UWB |
| [`ports/esp32-idf/components/aliro_reader/aliro_ranging.h`](aliro_ranging.h.md) | Aliro M1-M4 ranging-setup interface: negotiates UWB ranging parameters with the device and |
| [`ports/esp32-idf/components/aliro_reader/aliro_reader.c`](aliro_reader.c.md) | Aliro reader engine: drives the Access Protocol (AUTH0/AUTH1/EXCHANGE) handshake over BLE, |
