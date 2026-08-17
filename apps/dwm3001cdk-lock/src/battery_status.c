/* SPDX-License-Identifier: ISC */
#include "battery_status.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(battery_diag, CONFIG_LOG_DEFAULT_LEVEL);

#define BAT_ADC_CHANNEL 0u
#define BAT_ADC_RESOLUTION 12u
#define BAT_ADC_REF_MV 600

/* Diagnostic branch only: sample frequently in RAM, persist sparsely. */
#define BAT_DIAG_SAMPLE_MS 2000
#define BAT_DIAG_STORE_MS 10000
#define BAT_DIAG_SLOTS 256u
#define BAT_DIAG_TREE "battery_diag"
#define BAT_DIAG_KEY_PREFIX BAT_DIAG_TREE "/s"

struct battery_diag_record {
    uint32_t seq;
    uint32_t uptime_s;
    uint16_t raw;
    uint16_t vdd_mv;
};

static uint16_t s_last_vdd_mv;
static bool s_adc_ready;
static uint32_t s_diag_seq;
static int64_t s_last_store_ms;
static struct k_work_delayable s_diag_work;

static const struct device *const s_adc = DEVICE_DT_GET(DT_NODELABEL(adc));

static const struct adc_channel_cfg s_channel_cfg = {
    .gain = ADC_GAIN_1_6,
    .reference = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = BAT_ADC_CHANNEL,
    .differential = false,
    .input_positive = NRF_SAADC_VDD,
};

static int battery_sample_vdd(uint16_t *out_mv, uint16_t *out_raw)
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
        return -ENODEV;
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
        return rc != 0 ? rc : -EIO;
    }

    mv = sample;
    rc = adc_raw_to_millivolts(BAT_ADC_REF_MV, ADC_GAIN_1_6,
                               BAT_ADC_RESOLUTION, &mv);
    if (rc != 0 || mv <= 0 || mv > 3600) {
        return rc != 0 ? rc : -ERANGE;
    }

    if (out_raw != NULL) {
        *out_raw = (uint16_t)sample;
    }
    *out_mv = (uint16_t)mv;
    return 0;
}

static int battery_sample_vdd_mv(uint16_t *out_mv)
{
    return battery_sample_vdd(out_mv, NULL);
}

/* Load callback for the persistent ring. Every retained sample is printed at
 * boot so a battery-only run can be inspected after the board powers off. */
static int battery_diag_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    struct battery_diag_record rec;
    ssize_t got;

    if (name == NULL || name[0] != 's' || len != sizeof(rec)) {
        return 0;
    }

    got = read_cb(cb_arg, &rec, sizeof(rec));
    if (got != sizeof(rec)) {
        return got < 0 ? (int)got : -EINVAL;
    }

    if (rec.seq >= s_diag_seq) {
        s_diag_seq = rec.seq + 1u;
    }

    LOG_INF("[BDIAG OLD] seq=%u t=%us raw=%u vdd=%umV",
            rec.seq, rec.uptime_s, rec.raw, rec.vdd_mv);
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(battery_diag, BAT_DIAG_TREE, NULL,
                               battery_diag_set, NULL, NULL);

static int battery_diag_store(uint16_t raw, uint16_t mv, int64_t now_ms)
{
    struct battery_diag_record rec = {
        .seq = s_diag_seq,
        .uptime_s = (uint32_t)(now_ms / 1000),
        .raw = raw,
        .vdd_mv = mv,
    };
    char key[24];
    unsigned int slot = (unsigned int)(s_diag_seq % BAT_DIAG_SLOTS);
    int rc;

    (void)snprintf(key, sizeof(key), BAT_DIAG_KEY_PREFIX "%03u", slot);
    rc = settings_save_one(key, &rec, sizeof(rec));
    if (rc == 0) {
        LOG_INF("[BDIAG SAVE] seq=%u slot=%u t=%us raw=%u vdd=%umV",
                rec.seq, slot, rec.uptime_s, rec.raw, rec.vdd_mv);
        s_diag_seq++;
    } else {
        LOG_WRN("[BDIAG] settings save rc=%d", rc);
    }
    return rc;
}

static void battery_diag_work(struct k_work *work)
{
    uint16_t mv = 0u;
    uint16_t raw = 0u;
    int64_t now_ms = k_uptime_get();
    int rc;

    ARG_UNUSED(work);

    rc = battery_sample_vdd(&mv, &raw);
    if (rc == 0) {
        s_last_vdd_mv = mv;
        LOG_INF("[BDIAG LIVE] t=%lldms raw=%u vdd=%umV",
                (long long)now_ms, raw, mv);

        if (s_last_store_ms == 0 || (now_ms - s_last_store_ms) >= BAT_DIAG_STORE_MS) {
            if (battery_diag_store(raw, mv, now_ms) == 0) {
                s_last_store_ms = now_ms;
            }
        }
    } else {
        LOG_WRN("[BDIAG] ADC sample rc=%d", rc);
    }

    (void)k_work_reschedule(&s_diag_work, K_MSEC(BAT_DIAG_SAMPLE_MS));
}

static int battery_diag_init(void)
{
    int rc = settings_subsys_init();

    if (rc != 0) {
        LOG_WRN("[BDIAG] settings init rc=%d", rc);
    } else {
        LOG_INF("[BDIAG] dumping retained discharge samples; sort by seq if needed");
        rc = settings_load_subtree(BAT_DIAG_TREE);
        if (rc != 0) {
            LOG_WRN("[BDIAG] settings load rc=%d", rc);
        }
    }

    LOG_INF("[BDIAG] starting: ADC every %d ms, flash every %d ms, %u-slot ring",
            BAT_DIAG_SAMPLE_MS, BAT_DIAG_STORE_MS, BAT_DIAG_SLOTS);
    k_work_init_delayable(&s_diag_work, battery_diag_work);
    (void)k_work_schedule(&s_diag_work, K_SECONDS(2));
    return 0;
}

SYS_INIT(battery_diag_init, APPLICATION, 95);

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
     * Deliberately coarse thresholds for a regulated rail. This diagnostic
     * branch exists to measure the real collapse curve before changing them.
     */
    if (mv >= 3150u) return 100u;
    if (mv >= 3050u) return 60u;
    if (mv >= 2950u) return 35u;
    if (mv >= 2850u) return 20u;
    if (mv >= 2750u) return 10u;
    return 5u;
}
