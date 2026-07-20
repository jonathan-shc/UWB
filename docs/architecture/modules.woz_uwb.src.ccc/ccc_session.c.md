<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/ccc/ccc_session.c`

@file ccc_session.c — Aliro/CCC ranging seam implementation. See ccc_session.h.

**depends on** [`modules/woz_uwb/src/ccc/ccc_session.h`](ccc_session.h.md)

## API

### `uint16_t ccc_session_n_round(const struct ccc_ran_session *s)`
`modules/woz_uwb/src/ccc/ccc_session.c:7`

Rounds per ranging block, from N_RAN_S and the slot parameters (0 if the denominator is 0).

**called by** `ccc_session_to_ran_params`

### `int ccc_session_to_ran_params(const struct ccc_ran_session *s, struct ccc_ran_params *out)`
`modules/woz_uwb/src/ccc/ccc_session.c:22`

Map an Aliro session onto the CCC MAC's ranging-schedule parameters.

**calls** `ccc_session_n_round`
