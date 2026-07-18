idf_component_register(
	SRCS
		"src/aliro_hash.c"
		"src/aliro_crypto.c"
		"src/aliro_prim_psa.c"
	INCLUDE_DIRS "include"
	PRIV_INCLUDE_DIRS "src"
	REQUIRES mbedtls
)
