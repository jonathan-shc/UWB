<!-- generated documentation — edit the source, not this file -->
# `modules/woz_uwb/src/facade/woz_logfmt.c`

@file woz_logfmt.c — PRETTY-gated high-res timestamp + compact colored log line.

**discussed in** [`docs/porting-esp32.md`](../../porting-esp32.md), [`docs/porting.md`](../../porting.md)

## API

### `static log_timestamp_t woz_timestamp_get(void)`
`modules/woz_uwb/src/facade/woz_logfmt.c:47`

@brief Advance + read the 64-bit cycle accumulator (irq-safe; any context).

**called by** `woz_wrap_sample`

### `static void woz_wrap_sample(struct k_timer *t)`
`modules/woz_uwb/src/facade/woz_logfmt.c:61`

@brief Periodic sampler so a CYCCNT wrap is never missed during console idle.

**calls** `woz_timestamp_get`

### `struct woz_sink`
`modules/woz_uwb/src/facade/woz_logfmt.c:70`

@brief Small bounded sink so cbpprintf can render into a stack buffer.

### `static int woz_sink_out(int c, void *ctx)`
`modules/woz_uwb/src/facade/woz_logfmt.c:81`

@brief Sink callback for formatted output: append one character to the buffer if space remains.
@param c Character to append.
@param ctx Pointer to woz_sink context.
@return Character appended.

### `static void woz_sink_str(struct woz_sink *s, const char *str)`
`modules/woz_uwb/src/facade/woz_logfmt.c:98`

@brief Append a null-terminated string to the sink buffer one character at a time, stopping at
end of string or exhausted buffer space.
@param s Sink context.
@param str String to append (null-terminated).

**called by** `woz_msg_format`

### `static const char *module_color(const char *name)`
`modules/woz_uwb/src/facade/woz_logfmt.c:107`

@brief Stable per-module colour from a name hash (djb2).

**called by** `woz_msg_format`

### `static void woz_msg_format(const struct log_output *output, struct log_msg *msg, uint32_t flags)`
`modules/woz_uwb/src/facade/woz_logfmt.c:121`

@brief Render one message as `SEC.NS module message`, or delegate hexdumps.

**calls** `module_color`, `woz_sink_str`

### `static int woz_logfmt_init(void)`
`modules/woz_uwb/src/facade/woz_logfmt.c:231`

@brief Initialize the DWT cycle counter and log formatting on startup.
@return 0 on success.
