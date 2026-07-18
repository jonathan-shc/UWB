idf_component_register(
	SRCS "aliro_reader.c" "aliro_apdu.c" "aliro_prov.c" "aliro_prov_nvs.c"
	INCLUDE_DIRS "include"
	PRIV_INCLUDE_DIRS "."
	REQUIRES aliro_ble woz_uwb aliro_crypto nvs_flash
)
