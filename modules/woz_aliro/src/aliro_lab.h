// Aliro Lab trace: structured "[ALAB]" lines at transaction phase boundaries,
// parsed by tools/aliro_lab.py into a scored walk-up report. Compiled out unless
// CONFIG_WOZ_ALIRO_LAB is set.
/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 *
 * aliro_lab — one line per event, `[ALAB] t=<us> ev=<name>[ <key>=<val>]`, with
 * t from woz_uptime_us(). Emit only from the BLE-host or Matter task — never
 * from the UWB RX/ISR path. UWB-side phase boundaries are latched by
 * aliro_lat_mark() (a plain store) and printed later by aliro_lab_dump(), which
 * runs once per walk-up from aliro_lat_report() (bolt) or the disconnect path,
 * whichever comes first. Implemented in aliro_lat.c.
 */
#ifndef ALIRO_LAB_H
#define ALIRO_LAB_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_WOZ_ALIRO_LAB)

/* One trace line, stamped now. */
void aliro_lab_ev(const char *ev);

/* One trace line with a single integer attribute, stamped now. */
void aliro_lab_evi(const char *ev, const char *key, long val);

/* Print a `ph.<name>` line for every latency phase stamped this walk-up, at the
 * phase's own timestamp. One-shot until aliro_lat_begin() opens the next
 * walk-up; call off the UWB path. */
void aliro_lab_dump(void);

#else

static inline void aliro_lab_ev(const char *ev)
{
	(void)ev;
}

static inline void aliro_lab_evi(const char *ev, const char *key, long val)
{
	(void)ev;
	(void)key;
	(void)val;
}

static inline void aliro_lab_dump(void)
{
}

#endif /* CONFIG_WOZ_ALIRO_LAB */

#ifdef __cplusplus
}
#endif

#endif /* ALIRO_LAB_H */
