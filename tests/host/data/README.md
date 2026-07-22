# Aliro Lab captures

Serial logs consumed by `tools/aliro_lab.py`.

- `aliro_lab_sample.log` — a synthetic fixture with deterministic timings. The
  unit tests assert exact phase offsets against it, so treat it as a golden
  file: do not edit it casually.
- `aliro_lab_capture.log` — a real reader serial capture (present once one is
  committed). The suite only smoke-checks it — it parses, drives a bolt, and
  scores no FAIL — so real-world timing jitter will not break `make test`.

## Capturing a real one

The **matter-lock** app stamps the full walk-up through the bolt. Flash a
trace-capable image once, then capture as many walk-ups as you like without
reflashing — the trace is OFF at boot and toggled at the `matter>` console:

```
cd ports/esp32/apps/matter-lock
make lab-flash        # ONE TIME: build CONFIG_WOZ_ALIRO_LAB=y into build-lab/ and flash

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

`make build` / `make flash` never enable the trace (it lives only in `build-lab/`,
and even there it is off until `lab on`), so nothing leaks into a production image
and there is nothing to remember to turn off. `make lab-clean` drops the variant
tree; `make lab-build` builds it without flashing.
