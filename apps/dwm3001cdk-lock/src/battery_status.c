/* SPDX-License-Identifier: ISC */
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
