/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 *
 * aliro_ranging — see aliro_ranging.h. Drives the engine reader adapter/session
 * for the post-auth M1-M4 ranging-setup exchange. Provenance: original glue over
 * the reverse-engineered engine adapter; call contract mirrors the reference
 * reader (integration/patches/custom_impl-uwb.patch).
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"

#include "aliro_ble.h"
#include "woz_uwb_facade.h"

#include "cherry/cherry.h"
#include "cherry/cherry_ccc.h"
#include "aliro_uwb_adapter/aliro_uwb_adapter.h"
#include "aliro_uwb_adapter/aliro_uwb_session.h"

#include "aliro_ranging.h"

static const char *TAG = "aliro_ranging";

#define ALIRO_VERSION        0x0100u
/* Reader-chosen UWB session id (any non-zero); advertised in M1, echoed in M2. */
#define ALIRO_UWB_SESSION_ID 0x02b02fd4u
/* Upper bound on an inbound ranging SDU (mirrors the reference kMaxBleMessage). */
#define ALIRO_RANGING_MSG_MAX 256u

/* Reader-side selection preferences. BORROWED for the adapter's lifetime (the
 * adapter stores the pointer, not a copy), so this must have static storage. */
static struct aliro_uwb_adapter_reader_config s_reader_cfg = {
	.min_ran_multiplier = 1u,
	.preferred_hopping_configs = {
		.configs = { ALIRO_HOPPING_CONFIG_DISABLED,
			     ALIRO_HOPPING_CONFIG_CONTINUOUS_DEFAULT },
		.count = 2u,
	},
	.mac_mode = 0u,           /* 1 ranging round, offset 0, single antenna */
	.r1_antennas = { 0u, 0u },
	.r2_antennas = { 0u, 0u },
};

static struct cherry *s_cherry;
static struct aliro_uwb_adapter *s_adapter;

/* The single active ranging session (the DW3000 is single-session). Owned and
 * mutated only on the BLE-host task. s_sess is cleared when the engine frees the
 * session (a DEINIT status event) or when we tear it down. */
static struct aliro_uwb_session *s_sess;
static uint16_t s_sess_conn;
static bool s_sess_active;

/* ---- engine callbacks (invoked synchronously on the BLE-host task) ---- */

/* Send an adapter-built message verbatim over the peer's L2CAP channel. The
 * bytes already carry the 4-byte Aliro header; hand them straight to the BLE
 * send. We own the message and MUST free it (even if we don't send). */
static void uwb_tx_cb(struct aliro_uwb_message *message,
		      struct aliro_uwb_session *session, void *user_data,
		      bool timeout)
{
	(void)session;
	(void)timeout;
	uint16_t conn = (uint16_t)(uintptr_t)user_data;

	if (message != NULL) {
		int rc = aliro_ble_send(conn, message->data, message->len);

		ESP_LOGI(TAG, "[conn %u] UWB TX %u bytes (rc=%d)", conn,
			 (unsigned)message->len, rc);
	}
	aliro_uwb_session_message_free(message);
}

/* Session notifications. On DEINIT the engine frees the session right after this
 * returns, so never touch event->session here; identify via user_data. Every
 * event must be freed. */
static void uwb_ev_cb(struct aliro_uwb_session_event *event, void *user_data)
{
	uint16_t conn = (uint16_t)(uintptr_t)user_data;

	if (event->type == ALIRO_UWB_SESSION_EVENT_TYPE_SESSION_STATUS &&
	    event->data.status != NULL) {
		switch (event->data.status->session_state) {
		case CHERRY_CCC_SESSION_STATE_ACTIVE:
			ESP_LOGI(TAG, "[conn %u] UWB ranging ACTIVE (negotiated "
				      "params live)", conn);
			break;
		case CHERRY_CCC_SESSION_STATE_IDLE:
			ESP_LOGI(TAG, "[conn %u] UWB session IDLE", conn);
			break;
		case CHERRY_CCC_SESSION_STATE_DEINIT:
			ESP_LOGI(TAG, "[conn %u] UWB session DEINIT (freed)", conn);
			if (s_sess_conn == conn) {
				s_sess = NULL;
				s_sess_active = false;
			}
			break;
		default:
			break;
		}
	} else if (event->type == ALIRO_UWB_SESSION_EVENT_TYPE_SESSION_ERROR) {
		ESP_LOGW(TAG, "[conn %u] UWB session ERROR", conn);
	}
	aliro_uwb_session_event_free(event);
}

/* ---- public API ---- */

int aliro_ranging_init(void)
{
	if (s_adapter != NULL) {
		return 0; /* already up */
	}
	s_cherry = cherry_create("dw3000-fira", NULL, NULL);
	if (s_cherry == NULL) {
		ESP_LOGE(TAG, "cherry_create failed");
		return -1;
	}

	/* Advertised CCC capabilities. Deep-copied by create_reader, so these
	 * locals need not outlive the call. Values match the reference reader. */
	uint16_t proto[]       = { ALIRO_VERSION };
	uint16_t uwb_configs[] = { 0x0000u };
	uint8_t  pulse_combos[] = { 0x00u };
	struct cherry_ccc_capabilities ccc = {
		.slot_bitmask = 0xFFu,
		.sync_code_index_bitmask = 0x00000F00u, /* SYNC codes 9..12 */
		.hopping_config_bitmask = 0x1Au,        /* default + continuous + none */
		.channel_bitmask = 0x03u,               /* ch5 + ch9 */
		.protocol_versions = { .len = 1u, .items = proto },
		.uwb_configs = { .len = 1u, .items = uwb_configs },
		.pulse_shape_combos = { .len = 1u, .items = pulse_combos },
		.minimum_ran_multiplier = 1u,
		.qorvo_vendor_feature_1_supported = false,
	};
	struct cherry_core_event_device_capabilities caps = {
		.status_err = CHERRY_ERR_NONE,
		.fira_capabilities = NULL,
		.ccc_capabilities = &ccc,
		.radar_capabilities = NULL,
	};

	s_adapter = aliro_uwb_adapter_create_reader(s_cherry, &caps, &s_reader_cfg);
	if (s_adapter == NULL) {
		ESP_LOGE(TAG, "aliro_uwb_adapter_create_reader failed");
		cherry_destroy_sync(s_cherry);
		s_cherry = NULL;
		return -1;
	}
	ESP_LOGI(TAG, "UWB ranging adapter ready");
	return 0;
}

int aliro_ranging_start(uint16_t conn_handle, const uint8_t *ursk)
{
	if (s_adapter == NULL || ursk == NULL) {
		ESP_LOGW(TAG, "[conn %u] ranging start: adapter not ready", conn_handle);
		return -1;
	}
	if (s_sess_active) {
		ESP_LOGW(TAG, "[conn %u] ranging busy (active on conn %u); DW3000 is "
			      "single-session", conn_handle, s_sess_conn);
		return -1;
	}

	/* Release the demo responder so the radio is free for the negotiated
	 * session (the engine re-starts it with M1-M4 params at M4). */
	woz_uwb_stop();

	struct aliro_uwb_session *sess = aliro_uwb_session_create(
		s_adapter, ALIRO_UWB_SESSION_ID, uwb_ev_cb, uwb_tx_cb,
		(void *)(uintptr_t)conn_handle);

	if (sess == NULL) {
		ESP_LOGE(TAG, "[conn %u] session_create failed", conn_handle);
		return -1;
	}
	if (aliro_uwb_session_set_ursk(sess, ursk) != ALIRO_UWB_ERR_NONE) {
		ESP_LOGE(TAG, "[conn %u] set_ursk failed", conn_handle);
		aliro_uwb_session_destroy(sess);
		return -1;
	}
	aliro_uwb_session_set_protocol_version(sess, ALIRO_VERSION);

	s_sess = sess;
	s_sess_conn = conn_handle;
	s_sess_active = true;

	/* Emits M1 synchronously via uwb_tx_cb before returning. */
	if (aliro_uwb_session_init_setup(sess) != ALIRO_UWB_ERR_NONE) {
		ESP_LOGE(TAG, "[conn %u] init_setup (M1) failed", conn_handle);
		s_sess = NULL;
		s_sess_active = false;
		aliro_uwb_session_destroy(sess);
		return -1;
	}
	ESP_LOGI(TAG, "[conn %u] ranging setup started (M1 sent)", conn_handle);
	return 0;
}

int aliro_ranging_feed(uint16_t conn_handle, const uint8_t *data, size_t len)
{
	if (!s_sess_active || s_sess == NULL || s_sess_conn != conn_handle) {
		return -1;
	}
	if (len < 4u || len > ALIRO_RANGING_MSG_MAX) {
		ESP_LOGW(TAG, "[conn %u] ranging SDU size %u out of range", conn_handle,
			 (unsigned)len);
		return -1;
	}

	/* Stack-framed message (the engine copies/consumes it, does not retain). */
	uint8_t storage[sizeof(struct aliro_uwb_message) + ALIRO_RANGING_MSG_MAX]
		__attribute__((aligned(8)));
	struct aliro_uwb_message *msg = (struct aliro_uwb_message *)storage;

	msg->len = len;
	memcpy(msg->data, data, len);

	/* M4 makes the engine start the responder (cherry_ccc_shim ->
	 * woz_uwb_start_aliro) with the negotiated params. A hard error may DEINIT
	 * the session, which clears s_sess via uwb_ev_cb; don't touch it after. */
	enum aliro_uwb_err e = aliro_uwb_session_message_handle(s_sess, msg);

	if (e != ALIRO_UWB_ERR_NONE) {
		ESP_LOGW(TAG, "[conn %u] message_handle err %d", conn_handle, (int)e);
		return -1;
	}
	return 0;
}

void aliro_ranging_stop(uint16_t conn_handle)
{
	if (!s_sess_active || s_sess == NULL || s_sess_conn != conn_handle) {
		return;
	}
	struct aliro_uwb_session *sess = s_sess;

	/* Clear first so the DEINIT event this destroy emits is a no-op, and a
	 * direct free (no CCC session yet) leaves no dangling pointer. */
	s_sess = NULL;
	s_sess_active = false;
	ESP_LOGI(TAG, "[conn %u] tearing down UWB ranging session", conn_handle);
	aliro_uwb_session_destroy(sess);
}
