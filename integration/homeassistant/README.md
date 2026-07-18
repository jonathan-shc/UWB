# Home Assistant bridge

`aliro_mqtt_bridge.py` republishes the lock's console log to MQTT as two Home
Assistant Discovery entities:

| Entity | Platform | Source line |
| --- | --- | --- |
| Distance (mm) | `sensor` | `rng  blk=N d=Xmm  tof=Y` (`modules/woz_uwb/src/ccc/ccc_shim_rx.c:498`) |
| Access | `event`, types `granted` / `denied` | `ACCESS GRANTED` / `ACCESS DENIED` (`access_manager_impl.cpp:756`/`:762`) |

Nothing on the device changes. The bridge only reads the console, so it runs
alongside the existing Apple Home and Wallet setup without taking a Matter
fabric slot.

## Prerequisites

The range line is compiled behind `CONFIG_WOZ_PRETTY_SHELL` and gated at runtime
by `uwb_rxdiag_rng_get()`, so issue `aliro frames on` on the shell first.
Without it the access events still flow but distance stays unpublished.

```
pip install paho-mqtt pyserial   # pyserial only for a real serial port
```

## Usage

```
# live, against a broker
aliro_mqtt_bridge.py --port /dev/tty.usbmodem1234 --broker 192.168.1.10

# parse only: no broker, no board, reads a captured log on stdin
aliro_mqtt_bridge.py --port - --dry-run < captured.log
```

`--node` (default `aliro-lock`) sets the MQTT node id and the Home Assistant
device name, so a second lock just needs a different value.

Topics are `aliro/<node>/distance`, `aliro/<node>/access`, and
`aliro/<node>/status` for availability, which is also the last-will topic.
Discovery configs are published retained under `homeassistant/`.

## Notes

Lines matching neither pattern are ignored, including the `DIST tof=… d=…mm`
diagnostic that carries the same distance value: the parser requires the
`rng`/`blk=` prefix, so a block is never counted twice.

`--dry-run` prints each topic and payload instead of connecting, which is the
quickest way to check a parser change against a captured log.
