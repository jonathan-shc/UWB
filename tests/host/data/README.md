# Aliro Lab captures

Serial logs consumed by `tools/aliro_lab.py`.

- `aliro_lab_sample.log` — a synthetic fixture with deterministic timings. The
  unit tests assert exact phase offsets against it, so treat it as a golden
  file: do not edit it casually.
- `aliro_lab_capture.log` — a real reader serial capture (present once one is
  committed). The suite only smoke-checks it — it parses, drives a bolt, and
  scores no FAIL — so real-world timing jitter will not break `make test`.

## Capturing a real one

The **matter-lock** app stamps the full walk-up through the bolt. `make lab-flash`
builds the trace variant into an isolated `build-lab/` and flashes it — the normal
`make build`/`make flash` stays default-off, so there is no menuconfig toggle to
set and nothing to remember to turn back off:

```
cd ports/esp32/apps/matter-lock
make lab-flash                # builds CONFIG_WOZ_ALIRO_LAB=y into build-lab/, flashes
make term LOG=walkup.log
# walk up to the reader with the provisioned iPhone, let it unlock, walk away
# ctrl-t q to quit tio

python3 ../../../tools/aliro_lab.py walkup.log                  # eyeball the report
cp walkup.log ../../../tests/host/data/aliro_lab_capture.log   # keep it
```

Nothing to undo afterwards: the trace lives only in `build-lab/`, and a plain
`make build` never enables it. `make lab-clean` drops the variant tree.
