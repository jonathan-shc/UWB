/* standalone_main.c — portable driver for the fuzz targets when no libFuzzer
 * runtime is available (Apple clang ships none). Replays each file named on the
 * command line through LLVMFuzzerTestOneInput; built with ASan/UBSan so the
 * checked-in corpus works as an everywhere-runnable regression gate. In CI on
 * Linux the same target files link against -fsanitize=fuzzer instead and this
 * file is left out (libFuzzer provides main). */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

/* Allocate exactly `size` bytes so the ASan redzone sits immediately after the
 * input; that is what turns a 1-byte tail over-read into a hard failure. */
static void run_file(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		perror(path);
		return;
	}
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return;
	}
	long n = ftell(f);
	if (n < 0 || fseek(f, 0, SEEK_SET) != 0) {
		fclose(f);
		return;
	}
	size_t size = (size_t)n;
	uint8_t *buf = (uint8_t *)malloc(size != 0 ? size : 1);
	if (buf == NULL) {
		fclose(f);
		return;
	}
	size_t got = fread(buf, 1, size, f);
	fclose(f);
	LLVMFuzzerTestOneInput(buf, got);
	free(buf);
}

int main(int argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		run_file(argv[i]);
	}
	return 0;
}
