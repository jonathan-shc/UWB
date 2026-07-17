/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 *
 * aliro_ble Phase 2.1: NimBLE bring-up, Aliro GATT service (0xFFF2) with the
 * SPSM/protocol-version READ and the device-version WRITE, and advertising.
 * The L2CAP CoC server behind the published SPSM lands in 2.2; aliro_ble_send()
 * and the on_data/on_connected callbacks are wired then. See SPEC.md.
 */
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "aliro_ble.h"

static const char *TAG = "aliro_ble";

/* L2CAP SPSM published to peers. Dynamic-PSM range is 0x0080..0x00FF; the value
 * is our choice (the peer learns it from the READ char, it is not well-known). */
#define ALIRO_L2CAP_SPSM 0x0080u

/* Aliro service, 16-bit 0xFFF2. */
static const ble_uuid16_t k_svc_uuid = BLE_UUID16_INIT(0xFFF2u);

/* Reader SPSM + BLE-UWB protocol version, D3B5A130-9E23-4B3A-8BE4-6B1EE5F980A3
 * (NimBLE stores 128-bit UUIDs little-endian, so bytes are reversed). */
static const ble_uuid128_t k_chr_reader_spsm_uuid = BLE_UUID128_INIT(
	0xa3, 0x80, 0xf9, 0xe5, 0x1e, 0x6b, 0xe4, 0x8b,
	0x3a, 0x4b, 0x23, 0x9e, 0x30, 0xa1, 0xb5, 0xd3);

/* User-device selected BLE-UWB protocol version, BD4B9502-3F54-11EC-B919-0242AC120005. */
static const ble_uuid128_t k_chr_device_ver_uuid = BLE_UUID128_INIT(
	0x05, 0x00, 0x12, 0xac, 0x42, 0x02, 0x19, 0xb9,
	0xec, 0x11, 0x54, 0x3f, 0x02, 0x95, 0x4b, 0xbd);

/* Advertised supported versions (host order) captured from config. */
#define ALIRO_MAX_VERSIONS 8u
static uint16_t s_versions[ALIRO_MAX_VERSIONS];
static size_t s_versions_count;
static struct aliro_ble_callbacks s_cb;

/* Prebuilt READ payload: [SPSM be16][verLen u8][versions be16*N][featLen u8][features u8]. */
static uint8_t s_read_payload[2u + 1u + (2u * ALIRO_MAX_VERSIONS) + 1u + 1u];
static uint16_t s_read_payload_len;

static uint8_t s_own_addr_type;

static void aliro_advertise(void);

static uint8_t encode_features(const struct aliro_ble_features *f)
{
	uint8_t b = 0u;
	if (f->timesync_procedure_0) b |= (uint8_t)(1u << 0);
	if (f->timesync_procedure_1) b |= (uint8_t)(1u << 1);
	if (f->le_coded_phy)         b |= (uint8_t)(1u << 2);
	return b;
}

static void build_read_payload(const struct aliro_ble_config *cfg)
{
	uint8_t *p = s_read_payload;

	*p++ = (uint8_t)(ALIRO_L2CAP_SPSM >> 8);
	*p++ = (uint8_t)(ALIRO_L2CAP_SPSM & 0xffu);

	*p++ = (uint8_t)(s_versions_count * 2u); /* protocol versions length */
	for (size_t i = 0; i < s_versions_count; i++) {
		*p++ = (uint8_t)(s_versions[i] >> 8);
		*p++ = (uint8_t)(s_versions[i] & 0xffu);
	}

	*p++ = 1u; /* features length: SupportedFeatures is one packed byte */
	*p++ = encode_features(&cfg->features);

	s_read_payload_len = (uint16_t)(p - s_read_payload);
}

/* READ: hand back the prebuilt SPSM/versions/features buffer. */
static int reader_spsm_access(uint16_t conn_handle, uint16_t attr_handle,
			      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	(void)conn_handle; (void)attr_handle; (void)arg;
	int rc = os_mbuf_append(ctxt->om, s_read_payload, s_read_payload_len);
	return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

/* WRITE: [version be16][featLen u8][features]. Validate + log the negotiated version. */
static int device_ver_access(uint16_t conn_handle, uint16_t attr_handle,
			     struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	(void)attr_handle; (void)arg;
	uint8_t buf[32];
	uint16_t len = 0;

	int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &len);
	if (rc != 0) {
		return BLE_ATT_ERR_UNLIKELY;
	}
	if (len < 3u) {
		return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
	}

	uint16_t version = (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
	uint8_t feat_len = buf[2];
	if (len != (uint16_t)(3u + feat_len)) {
		return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
	}

	bool supported = false;
	for (size_t i = 0; i < s_versions_count; i++) {
		if (s_versions[i] == version) {
			supported = true;
			break;
		}
	}
	ESP_LOGI(TAG, "conn %u selected BLE-UWB protocol version 0x%04x (%s)",
		 conn_handle, version, supported ? "supported" : "unsupported");
	return 0;
}

static const struct ble_gatt_svc_def k_gatt_svcs[] = {
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = &k_svc_uuid.u,
		.characteristics = (struct ble_gatt_chr_def[]){
			{
				.uuid = &k_chr_reader_spsm_uuid.u,
				.access_cb = reader_spsm_access,
				.flags = BLE_GATT_CHR_F_READ,
			},
			{
				.uuid = &k_chr_device_ver_uuid.u,
				.access_cb = device_ver_access,
				.flags = BLE_GATT_CHR_F_WRITE,
			},
			{ 0 },
		},
	},
	{ 0 },
};

static int gap_event(struct ble_gap_event *event, void *arg)
{
	(void)arg;
	switch (event->type) {
	case BLE_GAP_EVENT_CONNECT:
		ESP_LOGI(TAG, "GAP connect (conn %u) status=%d",
			 event->connect.conn_handle, event->connect.status);
		if (event->connect.status != 0) {
			aliro_advertise(); /* failed; keep advertising */
		}
		return 0;
	case BLE_GAP_EVENT_DISCONNECT:
		ESP_LOGI(TAG, "GAP disconnect reason=%d", event->disconnect.reason);
		aliro_advertise();
		return 0;
	case BLE_GAP_EVENT_ADV_COMPLETE:
		aliro_advertise();
		return 0;
	default:
		return 0;
	}
}

static void aliro_advertise(void)
{
	struct ble_hs_adv_fields fields = { 0 };
	const char *name = ble_svc_gap_device_name();

	fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
	fields.uuids16 = (ble_uuid16_t[]){ BLE_UUID16_INIT(0xFFF2u) };
	fields.num_uuids16 = 1;
	fields.uuids16_is_complete = 1;
	fields.name = (uint8_t *)name;
	fields.name_len = (uint8_t)strlen(name);
	fields.name_is_complete = 1;

	int rc = ble_gap_adv_set_fields(&fields);
	if (rc != 0) {
		ESP_LOGE(TAG, "adv_set_fields rc=%d", rc);
		return;
	}

	struct ble_gap_adv_params adv_params = { 0 };
	adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
	adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

	rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER,
			       &adv_params, gap_event, NULL);
	if (rc != 0) {
		ESP_LOGE(TAG, "adv_start rc=%d", rc);
	}
}

static void on_sync(void)
{
	int rc = ble_hs_util_ensure_addr(0);
	if (rc != 0) {
		ESP_LOGE(TAG, "ensure_addr rc=%d", rc);
		return;
	}
	rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
	if (rc != 0) {
		ESP_LOGE(TAG, "infer_auto rc=%d", rc);
		return;
	}
	ESP_LOGI(TAG, "NimBLE synced; advertising as Aliro reader (SPSM 0x%04x)",
		 (unsigned)ALIRO_L2CAP_SPSM);
	aliro_advertise();
}

static void on_reset(int reason)
{
	ESP_LOGW(TAG, "NimBLE reset; reason=%d", reason);
}

static void host_task(void *param)
{
	(void)param;
	nimble_port_run(); /* returns only on nimble_port_stop() */
	nimble_port_freertos_deinit();
}

int aliro_ble_start(const struct aliro_ble_config *cfg)
{
	if (cfg == NULL || cfg->proto_versions == NULL ||
	    cfg->proto_versions_count == 0 ||
	    cfg->proto_versions_count > ALIRO_MAX_VERSIONS) {
		return -1;
	}

	s_versions_count = cfg->proto_versions_count;
	for (size_t i = 0; i < s_versions_count; i++) {
		s_versions[i] = cfg->proto_versions[i];
	}
	s_cb = cfg->cb;
	build_read_payload(cfg);

	esp_err_t err = nvs_flash_init(); /* NimBLE stores bonding/keys in NVS */
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	err = nimble_port_init();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "nimble_port_init rc=%d", err);
		return -1;
	}

	ble_hs_cfg.sync_cb = on_sync;
	ble_hs_cfg.reset_cb = on_reset;

	ble_svc_gap_init();
	ble_svc_gatt_init();

	int rc = ble_gatts_count_cfg(k_gatt_svcs);
	if (rc != 0) {
		ESP_LOGE(TAG, "gatts_count_cfg rc=%d", rc);
		return -1;
	}
	rc = ble_gatts_add_svcs(k_gatt_svcs);
	if (rc != 0) {
		ESP_LOGE(TAG, "gatts_add_svcs rc=%d", rc);
		return -1;
	}

	rc = ble_svc_gap_device_name_set("Aliro Reader");
	if (rc != 0) {
		ESP_LOGW(TAG, "device_name_set rc=%d", rc);
	}

	nimble_port_freertos_init(host_task);
	return 0;
}

uint16_t aliro_ble_spsm(void)
{
	return ALIRO_L2CAP_SPSM;
}

int aliro_ble_send(uint16_t conn_handle, const uint8_t *data, size_t len)
{
	(void)conn_handle; (void)data; (void)len;
	ESP_LOGW(TAG, "aliro_ble_send: L2CAP not up yet (Phase 2.2)");
	return -1; /* L2CAP CoC arrives in 2.2 */
}
