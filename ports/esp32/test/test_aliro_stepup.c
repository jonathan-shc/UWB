/*
 * Copyright (c) 2026 asxeem
 * SPDX-License-Identifier: ISC
 *
 * Host known-answer test for the Aliro step-up (Access Document) codec + §7.4
 * verifier. Pure host build (no ESP-IDF, no hardware), compiled from the same
 * aliro_stepup/aliro_stepup_parse/aliro_crypto/aliro_hash sources as the target,
 * with aliro_prim_host.c as the AES-GCM double.
 *
 * Two anchor sets:
 *   §14.6  the spec worked example — StepUpSK -> SKReader/SKDevice, the
 *          SessionData request/response decrypt, the DeviceRequest bytes, the
 *          DeviceResponse parse, and the step-3 digest recompute. Byte-exact.
 *   synthetic  a deterministic self-minted issuer (stepup_vectors.h) whose ES256
 *          signatures really verify (checked at generation with the cryptography
 *          library). Drives the full verifier + every reject branch. The ES256
 *          primitive is a stub that, in GOLDEN mode, asserts the module handed it
 *          exactly the Sig_structure/signature/pubkey the real signer used.
 */
#include <stdio.h>
#include <string.h>

#include "aliro_crypto.h"
#include "aliro_hash.h"
#include "aliro_prim.h"
#include "aliro_stepup.h"

#include "stepup_vectors.h"

static int fails;

static size_t uh(uint8_t *d, const char *h)
{
	size_t n = strlen(h) / 2;

	for (size_t i = 0; i < n; i++) {
		unsigned v;

		sscanf(h + 2 * i, "%2x", &v);
		d[i] = (uint8_t)v;
	}
	return n;
}

static void chk_hex(const char *name, const uint8_t *got, size_t n, const char *want)
{
	char g[1200];

	for (size_t i = 0; i < n; i++) {
		sprintf(g + 2 * i, "%02x", got[i]);
	}
	if (strcmp(g, want) != 0) {
		printf("  FAIL %-28s\n    got  %s\n    want %s\n", name, g, want);
		fails++;
	} else {
		printf("  ok   %-28s\n", name);
	}
}

static void chk(const char *name, int cond)
{
	if (!cond) {
		printf("  FAIL %-28s\n", name);
		fails++;
	} else {
		printf("  ok   %-28s\n", name);
	}
}

static void chk_eq(const char *name, long got, long want)
{
	if (got != want) {
		printf("  FAIL %-28s got %ld want %ld\n", name, got, want);
		fails++;
	} else {
		printf("  ok   %-28s\n", name);
	}
}

/* ---- §14.6 spec vectors -------------------------------------------------- */

static const char *K_STEPUP_SK =
	"616f6575616f6575616f6575616f6575616f6575616f6575616f6575616f6575";
static const char *K_SKREADER =
	"d8dcf4959bf4ae5f05318bbd47d793f00bcb1dfaa82efbb32e10933c86148478";
static const char *K_SKDEVICE =
	"6a57227cc56f84760f03cd6c55c4da55d4a85cdc4ef39bb69a4fdf466606b270";
static const char *K_DEVREQ =
	"a2613163312e30613281a16131d818582ba26131a167616c69726f2d61a268656c656d656e7432f568656c65"
	"6d656e7434f5613567616c69726f2d61";
static const char *K_SD_REQ =
	"a16464617461584c8c8fba6253f17dd60f3f30fcf035195ecca10e706320c72ed41920e59ad72d0002305f08d"
	"fd03f785d91eb22cfe475760b72cba8c427a15c4520de417fc1a8b13c80f4a66ddc19df0e5b5d17";
static const char *K_SD_RESP =
	"a164646174615901fc8068130e2312181a60e00fbb74c6f71431b8dd77ec38d0e622568ec417f74575e2dcce0"
	"623bcff99c3fc3bf8a90bfdad1f1263ff016dc43f44c518c472de38ba2f05c2dff4493dd713d0babde2c009c58"
	"42d5cb7d348f1d93c1ca3224581a0ffc320631a86e93be1b2d572846ceb46746e2438fd21179c1587d9f48e94"
	"1b569682649ceac641c9a7a4dcdcd4f17de66b5c7660341e47d41785d578244f80d1ce00edf7a9b4b2ef2b2a58"
	"50c5f365f6a85d9ebf1fd0a8b9523d2d467ed1fd4dba8a95ebd8e46c1969085150738603bcb645dde6f5ebced5"
	"59d87582601c2b7db09d7a056059796084843b928559f81af2761d7c32dc101723f15f50c543212631590cc371"
	"e0010eb48011e370de65e7330b6ea4eb0bbe666e63f01451681b8e1261c12bca61524ad2daf081c9d21d06f302"
	"31eb926dce6e1c52f92c1068cea7a6ca8fe11b5dfd1d6fe48e8c336c2ef11f863f98dc877625ba8710cb7c860e"
	"e7f7202662409254e9c2da0cc2bf00ad585189e301a013342123782cdacec9eb1b17ddbc99eff438c501ddfb60"
	"58ef36db28a44b32b64470f49dbade000fe91d5925a129f89af53a61527702bc60c4667bc6bcbd9294b6bcaed1"
	"91dd76bf2873260dbe6da3d29fbeb883335bf7a1892421207968e1b87987b8419ffa629b8669582ff1cca8f211"
	"078f9b87db132ffdacc6068335d72546a679816d49d8a4";
static const char *K_DEVRESP =
	"a3613163312e30613281a26131a26131a167616c69726f2d6181d8185838a4613101613258200aa260c85ca2f"
	"6eca90016720a1d7c7c160baf9cfa1a5aa4156331b71863b426613368656c656d656e74326134a10001613284"
	"43a10126a104478ea23b8fe54e51590133d81859012ea7613163312e306132675348412d3235366133a167616"
	"c69726f2d61a3005820b193e9b1fd40d43aee51f794fb2754f537a12104b743f53ede26d4a74ef60466015820"
	"2f6f396adb893a91242c60f3b3a32237c90f543cbbed2bf10398ac228955b7e902582095feb0333d71a311b949"
	"21230db1bcd094629c01d0fe5e1f2ab6d888b8997ca36134a16134a40102200121582096313d6c63e24e337274"
	"2bfdb1a33ba2c897dcd68ab8c753e4fbd48dca6b7f9a2258201fb3269edd418857de1b39a4e4a44b92fa484caa"
	"722c228288f01d0c03a2c3d6613567616c69726f2d616136a36131c074323032342d30362d30315431333a3330"
	"3a30325a6132c074323032342d30362d30315431333a33303a30325a6133c074323032352d30362d3031543133"
	"3a33303a30325a6137f5584007df311fce5e28c83b5b88e6402fae24250c778eec0c58e06283a7d6ab7037e791"
	"307aadb8571b1229e18c49932de464a4dc4f639ad186eb8742099b56a15d17613567616c69726f2d61613300";
static const char *K_VDIGEST1 =
	"2f6f396adb893a91242c60f3b3a32237c90f543cbbed2bf10398ac228955b7e9";

/* ---- injected ES256 (see file header) ------------------------------------ */

enum { MODE_GOLDEN, MODE_ACCEPT, MODE_REJECT };
static int g_mode;
static int g_golden_ss_ok;
static int g_calls;

static int stub_verify(const uint8_t pub[65], const uint8_t *msg, size_t len, const uint8_t sig[64])
{
	g_calls++;
	if (g_mode == MODE_REJECT) {
		return -1;
	}
	if (g_mode == MODE_GOLDEN) {
		g_golden_ss_ok = len == SV_GOLDEN_SIGSTRUCT_len &&
				 memcmp(msg, SV_GOLDEN_SIGSTRUCT, len) == 0 &&
				 memcmp(sig, SV_GOLDEN_SIG, 64) == 0 &&
				 memcmp(pub, SV_ISSUER_PUB, 65) == 0;
		return g_golden_ss_ok ? 0 : -1;
	}
	return 0; /* MODE_ACCEPT */
}

static struct aliro_stepup_verify_ctx base_ctx(const uint8_t *kid, size_t kid_len,
					       const uint8_t *pub)
{
	static struct aliro_stepup_issuer iss;

	iss.kid = kid;
	iss.kid_len = kid_len;
	if (pub) {
		memcpy(iss.pub, pub, 65);
	}
	struct aliro_stepup_verify_ctx ctx = {0};

	ctx.issuers = &iss;
	ctx.n_issuers = 1;
	ctx.time_valid = 1;
	ctx.now_epoch = SV_EPOCH_NOW;
	ctx.access_iteration = 0;
	ctx.expected_doctype = ALIRO_STEPUP_DOCTYPE_ACCESS;
	ctx.ecdsa_verify = stub_verify;
	return ctx;
}

/* ---- suites -------------------------------------------------------------- */

static void t_spec_keys_and_codec(void)
{
	printf("-- §14.6 keys + SessionData codec --\n");
	uint8_t block[ALIRO_KEY_BLOCK_LEN] = {0};
	uint8_t stepup[32];

	uh(stepup, K_STEPUP_SK);
	memcpy(block + ALIRO_STEPUP_SK_OFFSET, stepup, 32);

	uint8_t skr[32], skd[32];

	chk("derive_keys rc", aliro_stepup_derive_keys(block, skr, skd) == 0);
	chk_hex("SKReader", skr, 32, K_SKREADER);
	chk_hex("SKDevice", skd, 32, K_SKDEVICE);

	/* DeviceRequest bytes (Table 8-21). */
	uint8_t dr[128];
	size_t drn;

	chk("build_device_request rc", aliro_stepup_build_device_request(NULL, 0, dr, sizeof(dr),
									 &drn) == 0);
	chk_hex("DeviceRequest", dr, drn, K_DEVREQ);

	/* Seal the DeviceRequest -> §14.6 SessionData request (SKReader, ctr 1). */
	struct aliro_secchan sc;

	aliro_stepup_channel_init(&sc, skr, skd);

	uint8_t sd[256];
	size_t sdn;

	chk("seal_sessiondata rc",
	    aliro_stepup_seal_sessiondata(&sc, dr, drn, sd, sizeof(sd), &sdn) == 0);
	chk_hex("SessionData request", sd, sdn, K_SD_REQ);

	/* Open the §14.6 SessionData response -> DeviceResponse (SKDevice, ctr 1). */
	uint8_t sdresp[600];
	size_t sdrespn = uh(sdresp, K_SD_RESP);
	uint8_t devresp[600];
	size_t devn;

	chk("open_sessiondata rc", aliro_stepup_open_sessiondata(&sc, sdresp, sdrespn, devresp,
								 sizeof(devresp), &devn) == 0);
	chk_hex("DeviceResponse (decrypted)", devresp, devn, K_DEVRESP);
}

static void t_spec_parse_and_digest(void)
{
	printf("-- §14.6 DeviceResponse parse + digest recompute --\n");
	uint8_t buf[600];
	size_t n = uh(buf, K_DEVRESP);
	struct aliro_stepup_doc doc;

	chk("parse rc", aliro_stepup_parse_response(buf, n, &doc) == 0);
	chk("have_document", doc.have_document == 1);
	chk("status 0", doc.status == 0);
	chk("doc_type aliro-a", strcmp(doc.doc_type, "aliro-a") == 0);
	chk("mso_doc_type aliro-a", strcmp(doc.mso_doc_type, "aliro-a") == 0);
	chk("digest_alg SHA-256", strcmp(doc.digest_alg, "SHA-256") == 0);
	chk_eq("n_digests", (long)doc.n_digests, 3);
	chk_eq("n_items", (long)doc.n_items, 1);
	chk("item digest_id 1", doc.items[0].digest_id == 1);
	chk("item elem element2", strcmp(doc.items[0].elem_id, "element2") == 0);
	chk("timeVerificationRequired", doc.time_verification_required == 1);
	chk("have validFrom/Until", doc.have_valid_from && doc.have_valid_until);
	chk_eq("validFrom epoch", (long)doc.valid_from_epoch, SV_EPOCH_VALID_FROM);
	chk_eq("validUntil epoch", (long)doc.valid_until_epoch, SV_EPOCH_VALID_UNTIL);

	/* Step-3 digest recompute against valueDigests[id=1]. */
	uint8_t want[32];

	uh(want, K_VDIGEST1);
	uint8_t got[32];

	aliro_sha256(doc.items[0].tagged, doc.items[0].tagged_len, got);
	chk("SHA-256(item)==valueDigests[1]", memcmp(got, want, 32) == 0);

	/* Full verify with the digest/validity/doctype/iteration logic exercised on
	 * the real spec document; ES256 is ACCEPTED (no issuer key is in §14.6). */
	uint8_t kid[8];
	size_t kidn = uh(kid, "8ea23b8fe54e51");
	struct aliro_stepup_verify_ctx ctx = base_ctx(kid, kidn, NULL);
	struct aliro_stepup_verdict v;

	g_mode = MODE_ACCEPT;
	ctx.now_epoch = SV_EPOCH_VALID_FROM; /* inside the window */
	int rc = aliro_stepup_verify(&doc, &ctx, &v);

	chk("§14.6 verify valid", rc == 0 && v.valid);
	chk("§14.6 valid_elements 1", v.valid_elements == 1);
	chk("§14.6 digests_ok", v.digests_ok);
	chk("§14.6 doctype_ok", v.doctype_ok);
	chk("§14.6 time_ok", v.time_ok);
}

static int verify_bytes(const uint8_t *buf, size_t n, struct aliro_stepup_verify_ctx *ctx,
			struct aliro_stepup_verdict *v)
{
	struct aliro_stepup_doc doc;

	if (aliro_stepup_parse_response(buf, n, &doc) != 0) {
		return -2;
	}
	return aliro_stepup_verify(&doc, ctx, v);
}

static void t_synth_good_and_rejects(void)
{
	printf("-- synthetic verifier (real ES256 vectors) --\n");
	struct aliro_stepup_verify_ctx ctx = base_ctx(SV_KID, SV_KID_len, SV_ISSUER_PUB);
	struct aliro_stepup_verdict v;

	/* Good: GOLDEN mode also asserts the exact Sig_structure/sig/pubkey. */
	g_mode = MODE_GOLDEN;
	g_golden_ss_ok = 0;
	int rc = verify_bytes(SV_GOOD, SV_GOOD_len, &ctx, &v);

	chk("good valid", rc == 0 && v.valid);
	chk("good Sig_structure==golden", g_golden_ss_ok == 1);
	chk("good sig_ok", v.sig_ok);
	chk("good chain_validated (kid)", v.issuer_chain_validated == 1);
	chk("good valid_elements 1", v.valid_elements == 1);

	/* Bad signature -> reject at step 2. */
	g_mode = MODE_REJECT;
	verify_bytes(SV_GOOD, SV_GOOD_len, &ctx, &v);
	chk("bad-sig reject step 2", v.reject_step == 2 && !v.valid);

	/* From here ES256 is ACCEPTED so later steps are the ones under test. */
	g_mode = MODE_ACCEPT;

	/* Tampered item -> digest mismatch -> step 3. */
	verify_bytes(SV_TAMPERED, SV_TAMPERED_len, &ctx, &v);
	chk("tampered reject step 3", v.reject_step == 3 && v.valid_elements == 0);

	/* DocType mismatch (MSO aliro-r vs doc aliro-a) -> step 4. */
	verify_bytes(SV_DOCTYPE_MISMATCH, SV_DOCTYPE_MISMATCH_len, &ctx, &v);
	chk("doctype reject step 4", v.reject_step == 4 && !v.doctype_ok);

	/* Time window: now after validUntil -> step 5. */
	ctx.now_epoch = SV_EPOCH_VALID_UNTIL + 1;
	verify_bytes(SV_GOOD, SV_GOOD_len, &ctx, &v);
	chk("expired reject step 5", v.reject_step == 5);

	/* now before validFrom -> step 5. */
	ctx.now_epoch = SV_EPOCH_VALID_FROM - 1;
	verify_bytes(SV_GOOD, SV_GOOD_len, &ctx, &v);
	chk("not-yet-valid reject step 5", v.reject_step == 5);

	/* No trusted clock + TimeVerificationRequired=true -> step 5. */
	ctx.now_epoch = SV_EPOCH_NOW;
	ctx.time_valid = 0;
	verify_bytes(SV_GOOD, SV_GOOD_len, &ctx, &v);
	chk("no-clock+required reject step 5", v.reject_step == 5 && !v.time_ok);

	/* No trusted clock + TimeVerificationRequired=false -> accepted (MAY). */
	verify_bytes(SV_TIMEVER_FALSE, SV_TIMEVER_FALSE_len, &ctx, &v);
	chk("no-clock+not-required valid", v.valid && v.time_ok);
	ctx.time_valid = 1;

	/* ValidityIteration: VI=1 < AccessIteration=20, diff 19>=8 -> step 6. */
	ctx.access_iteration = 20;
	verify_bytes(SV_WITH_VI, SV_WITH_VI_len, &ctx, &v);
	chk("VI reject step 6", v.reject_step == 6 && !v.iteration_ok);

	/* VI=1 < AccessIteration=5, diff 4<8 -> valid. */
	ctx.access_iteration = 5;
	verify_bytes(SV_WITH_VI, SV_WITH_VI_len, &ctx, &v);
	chk("VI small-diff valid", v.valid);
	ctx.access_iteration = 0;

	/* Wrong kid -> issuer not found -> step 1. */
	uint8_t wrong_kid[4] = {0xaa, 0xbb, 0xcc, 0xdd};
	struct aliro_stepup_verify_ctx wctx = base_ctx(wrong_kid, 4, SV_ISSUER_PUB);

	verify_bytes(SV_GOOD, SV_GOOD_len, &wctx, &v);
	chk("wrong-kid reject step 1", v.reject_step == 1 && !v.issuer_key_found);

	/* x5chain: EE key extracted, chain NOT validated, otherwise valid. */
	g_mode = MODE_GOLDEN;
	g_golden_ss_ok = 0;
	struct aliro_stepup_verify_ctx xctx = base_ctx(NULL, 0, NULL);

	xctx.n_issuers = 0; /* force x5chain path */
	rc = verify_bytes(SV_X5CHAIN, SV_X5CHAIN_len, &xctx, &v);
	chk("x5chain valid", rc == 0 && v.valid);
	chk("x5chain key found", v.issuer_key_found);
	chk("x5chain chain NOT validated", v.issuer_chain_validated == 0);
	chk("x5chain Sig_structure==golden", g_golden_ss_ok == 1);
	g_mode = MODE_ACCEPT;

	/* No documents returned -> reject (no data elements). */
	rc = verify_bytes(SV_NO_DOC, SV_NO_DOC_len, &ctx, &v);
	chk("no-document reject", rc < 0 && !v.valid);
}

static void t_run_convenience(void)
{
	printf("-- aliro_stepup_run (§14.6 SessionData response) --\n");
	uint8_t block[ALIRO_KEY_BLOCK_LEN] = {0}, stepup[32], skr[32], skd[32];

	uh(stepup, K_STEPUP_SK);
	memcpy(block + ALIRO_STEPUP_SK_OFFSET, stepup, 32);
	aliro_stepup_derive_keys(block, skr, skd);

	struct aliro_secchan sc;

	aliro_stepup_channel_init(&sc, skr, skd);

	uint8_t sdresp[600];
	size_t sdn = uh(sdresp, K_SD_RESP);
	uint8_t scratch[700];
	uint8_t kid[8];
	size_t kidn = uh(kid, "8ea23b8fe54e51");
	struct aliro_stepup_verify_ctx ctx = base_ctx(kid, kidn, NULL);

	ctx.now_epoch = SV_EPOCH_VALID_FROM;
	g_mode = MODE_ACCEPT;

	struct aliro_stepup_doc doc;
	struct aliro_stepup_verdict v;
	int rc = aliro_stepup_run(&sc, sdresp, sdn, &ctx, scratch, sizeof(scratch), &doc, &v);

	chk("run valid", rc == 0 && v.valid);
	chk("run doc_type aliro-a", strcmp(doc.doc_type, "aliro-a") == 0);
	chk("run valid_elements 1", v.valid_elements == 1);
}

static void t_apdu_builders(void)
{
	printf("-- ENVELOPE / GET RESPONSE APDUs --\n");
	uint8_t data[4] = {0xa1, 0x64, 0x64, 0x61};
	uint8_t out[32];
	size_t n;

	chk("envelope rc", aliro_stepup_build_envelope(data, 4, 0, out, sizeof(out), &n) == 0);
	chk_hex("envelope", out, n, "00c3000004a164646100");
	chk("envelope chaining CLA 0x10",
	    aliro_stepup_build_envelope(data, 4, 1, out, sizeof(out), &n) == 0 && out[0] == 0x10);
	chk("get_response rc", aliro_stepup_build_get_response(0x20, out, sizeof(out), &n) == 0);
	chk_hex("get_response", out, n, "00c0000020");
}

int main(void)
{
	aliro_prim_init();
	aliro_crypto_init();

	t_spec_keys_and_codec();
	t_spec_parse_and_digest();
	t_synth_good_and_rejects();
	t_run_convenience();
	t_apdu_builders();

	printf("\n%s (%d failure%s)\n", fails ? "FAIL" : "PASS", fails, fails == 1 ? "" : "s");
	return fails ? 1 : 0;
}
