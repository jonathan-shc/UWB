#!/usr/bin/env python3
"""Replace the fixed 100% battery value with a coarse nRF52833 VDD estimate.

This is intentionally a low-battery indicator, not a fuel gauge.  The DWM3001CDK
regulates its battery input, so VDD is expected to remain nearly flat for most of
the discharge and only fall near the end.  That is useful here: HomeKit should
stay at 100% while the rail is healthy, then step down conservatively when the
rail begins to sag.

The nRF52833 SAADC supports VDD as an internal input.  We sample it with the
0.6-V internal reference and 1/6 gain, yielding a 0..3.6-V input range.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "apps" / "dwm3001cdk-lock"
SRC = APP / "src"
BAT_C = SRC / "battery_status.c"
BAT_H = SRC / "battery_status.h"
OVERLAY = APP / "overlay-battery-status.conf"
COMMISSION = SRC / "matter_commission.c"

BATTERY_H = r'''/* SPDX-License-Identifier: ISC */
#pragma once
#include <stdint.h>

/* Return a coarse battery state estimate as 0..100 percent. */
uint8_t battery_status_percent(void);

/* Last measured nRF VDD in millivolts, or 0 when no valid sample exists yet. */
uint16_t battery_status_vdd_mv(void);
'''

BATTERY_C = r'''/* SPDX-License-Identifier: ISC */
#include "battery_status.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>

#include <stdint.h>

#define BAT_ADC_CHANNEL 0u
#define BAT_ADC_RESOLUTION 12u
#define BAT_ADC_REF_MV 600

static uint16_t s_last_vdd_mv;
static bool s_adc_ready;

static const struct device *const s_adc = DEVICE_DT_GET(DT_NODELABEL(adc));

static const struct adc_channel_cfg s_channel_cfg = {
    .gain = ADC_GAIN_1_6,
    .reference = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = BAT_ADC_CHANNEL,
    .differential = false,
    .input_positive = NRF_SAADC_VDD,
};

static int battery_sample_vdd_mv(uint16_t *out_mv)
{
    int16_t sample = 0;
    int32_t mv;
    int rc;
    struct adc_sequence seq = {
        .channels = BIT(BAT_ADC_CHANNEL),
        .buffer = &sample,
        .buffer_size = sizeof(sample),
        .resolution = BAT_ADC_RESOLUTION,
        .oversampling = 4,
        .calibrate = false,
    };

    if (!device_is_ready(s_adc)) {
        return -1;
    }

    if (!s_adc_ready) {
        rc = adc_channel_setup(s_adc, &s_channel_cfg);
        if (rc != 0) {
            return rc;
        }
        s_adc_ready = true;
    }

    rc = adc_read(s_adc, &seq);
    if (rc != 0 || sample < 0) {
        return rc != 0 ? rc : -1;
    }

    mv = sample;
    rc = adc_raw_to_millivolts(BAT_ADC_REF_MV, ADC_GAIN_1_6,
                               BAT_ADC_RESOLUTION, &mv);
    if (rc != 0 || mv <= 0 || mv > 3600) {
        return rc != 0 ? rc : -1;
    }

    *out_mv = (uint16_t)mv;
    return 0;
}

uint16_t battery_status_vdd_mv(void)
{
    uint16_t mv;

    if (battery_sample_vdd_mv(&mv) == 0) {
        s_last_vdd_mv = mv;
    }
    return s_last_vdd_mv;
}

uint8_t battery_status_percent(void)
{
    const uint16_t mv = battery_status_vdd_mv();

    /* Measurement unavailable: keep the previously safe HomeKit behaviour. */
    if (mv == 0u) {
        return 100u;
    }

    /*
     * Deliberately coarse thresholds for a regulated rail.  We are not trying
     * to infer LiPo state-of-charge from VDD; we only want the last part of the
     * discharge to become visible before brown-out/shutdown.
     *
     * These are starting points to be calibrated on the user's DWM3001CDK +
     * Adafruit 6091 + 1S LiPo setup from observed HomeKit behaviour.
     */
    if (mv >= 3150u) return 100u;
    if (mv >= 3050u) return 60u;
    if (mv >= 2950u) return 35u;
    if (mv >= 2850u) return 20u;
    if (mv >= 2750u) return 10u;
    return 5u;
}
'''

BAT_H.write_text(BATTERY_H)
BAT_C.write_text(BATTERY_C)

# The Zephyr ADC driver must be enabled for the internal VDD channel.
overlay = OVERLAY.read_text() if OVERLAY.exists() else "CONFIG_CUSTOM_BATTERY_STATUS=y\n"
if "CONFIG_ADC=y" not in overlay:
    overlay += "CONFIG_ADC=y\n"
OVERLAY.write_text(overlay)

# Ensure the commission layer can call the battery module.
text = COMMISSION.read_text()
if '#include "battery_status.h"' not in text:
    anchor = '#include "status_led.h" /* the lock LED; a tile tap has to move it too */\n'
    if anchor not in text:
        raise SystemExit("status_led include anchor not found in matter_commission.c")
    text = text.replace(anchor, anchor + '#ifdef CONFIG_CUSTOM_BATTERY_STATUS\n#include "battery_status.h"\n#endif\n', 1)

# Add a five-minute refresh worker.  Matter reads s_info.battery_percent, so a
# boot-only sample would otherwise remain frozen for the lifetime of the node.
worker_block = r'''
#ifdef CONFIG_CUSTOM_BATTERY_STATUS
static struct k_work_delayable s_battery_refresh_work;

static void battery_refresh_work(struct k_work *work)
{
    ARG_UNUSED(work);
    s_info.battery_percent = battery_status_percent();
    (void)k_work_reschedule(&s_battery_refresh_work, K_MINUTES(5));
}
#endif
'''
marker = 'static struct matter_im_server s_im;\n'
if worker_block not in text:
    if marker not in text:
        raise SystemExit("s_im marker not found in matter_commission.c")
    text = text.replace(marker, marker + worker_block, 1)

init_sig = 'int matter_commission_init(void)\n{\n'
if init_sig not in text:
    raise SystemExit("matter_commission_init signature not found")

init_block = r'''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
    s_info.battery_percent = battery_status_percent();
    k_work_init_delayable(&s_battery_refresh_work, battery_refresh_work);
    (void)k_work_schedule(&s_battery_refresh_work, K_MINUTES(5));
#endif
'''

# Replace the earlier one-shot migration if present, otherwise insert once.
old = r'''#ifdef CONFIG_CUSTOM_BATTERY_STATUS
	s_info.battery_percent = battery_status_percent();
#endif
'''
if old in text:
    text = text.replace(old, init_block, 1)
elif init_block not in text:
    text = text.replace(init_sig, init_sig + init_block, 1)

COMMISSION.write_text(text)

print("Implemented coarse nRF52833 VDD battery estimate with 5-minute refresh.")
print("Thresholds: >=3150=100, >=3050=60, >=2950=35, >=2850=20, >=2750=10, else=5.")
