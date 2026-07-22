# Aliro Lab captures

Serial logs consumed by `tools/aliro_lab.py`.

- `aliro_lab_sample.log` — a synthetic fixture with deterministic timings. The
  unit tests assert exact phase offsets against it, so treat it as a golden
  file: do not edit it casually.
- `aliro_lab_capture.log` — a real reader serial capture (present once one is
  committed). The suite only smoke-checks it — it parses, drives a bolt, and
  scores no FAIL — so real-world timing jitter will not break `make test`.

## Capturing a real one

The **matter-lock** app stamps the full walk-up through the bolt. The trace ships
in every build (like the sibling `uwbdiag` trace) but is OFF at boot, so there is
no special build — flash normally, then capture as many walk-ups as you like,
toggling the trace at the `matter>` console:

```
cd ports/esp32/apps/matter-lock
make flash            # a normal build/flash — the trace is compiled in, off at boot

make lab              # per walk-up: opens the console, captures, auto-scores + opens the report
#   at the console:  lab on
#   walk up to the reader with the provisioned iPhone, let it unlock, walk away
#   at the console:  lab off        (then ctrl-t q to quit)
# make lab writes walkup-<timestamp>.log(.html) and runs tools/aliro_lab.py for you
```

To keep a capture as the committed reference artifact:

```
cp walkup-<timestamp>.log ../../../tests/host/data/aliro_lab_capture.log
```

To strip the trace entirely from a hardened production image, set
`CONFIG_WOZ_ALIRO_LAB=n` (menuconfig or a defaults fragment).
