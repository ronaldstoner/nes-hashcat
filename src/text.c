#include "text.h"
#include <string.h>
#include <neslib.h>

// --- Direct writes (rendering OFF). ---

void text_n(uint8_t x, uint8_t y, const char *s, uint8_t n) {
  vram_adr(NTADR_A(x, y));
  vram_write(s, n);
}

void text_at(uint8_t x, uint8_t y, const char *s) {
  text_n(x, y, s, (uint8_t)strlen(s));
}

void text_clear(void) {
  vram_adr(NAMETABLE_A);
  vram_fill(' ', 32 * 30);
  vram_adr(NAMETABLE_A + 0x3c0);  // attribute table
  vram_fill(0, 64);
}

void text_hline(uint8_t x, uint8_t y, uint8_t w, uint8_t tile) {
  vram_adr(NTADR_A(x, y));
  vram_fill(tile, w);
}

void attr_fill(uint8_t pal) {
  uint8_t b = (pal & 3) | ((pal & 3) << 2) | ((pal & 3) << 4) | ((pal & 3) << 6);
  vram_adr(NAMETABLE_A + 0x3c0);
  vram_fill(b, 64);
}

// Assign a palette to each of the 8 vertical 32px bands (attribute rows).
void attr_bands(const uint8_t bands[8]) {
  vram_adr(NAMETABLE_A + 0x3c0);
  for (uint8_t b = 0; b < 8; ++b) {
    uint8_t p = bands[b] & 3;
    uint8_t v = p | (p << 2) | (p << 4) | (p << 6);
    for (uint8_t c = 0; c < 8; ++c) vram_put(v);
  }
}

// --- Deferred update buffer (flushed during NMI) ---
// Format: [hi|NT_UPD_HORZ, lo, len, bytes...] per run, then NT_UPD_EOF.
static uint8_t vbuf[192];
static uint8_t vlen;

void vb_reset(void) { vlen = 0; }

void vb_strn(uint8_t x, uint8_t y, const char *s, uint8_t n) {
  unsigned adr = NTADR_A(x, y);
  if (vlen + n + 4 > sizeof(vbuf)) return;  // never overrun the vblank budget
  vbuf[vlen++] = (uint8_t)(adr >> 8) | NT_UPD_HORZ;
  vbuf[vlen++] = (uint8_t)(adr & 0xff);
  vbuf[vlen++] = n;
  for (uint8_t i = 0; i < n; ++i) vbuf[vlen++] = (uint8_t)s[i];
}

void vb_str(uint8_t x, uint8_t y, const char *s) {
  vb_strn(x, y, s, (uint8_t)strlen(s));
}

void vb_commit(void) {
  vbuf[vlen] = NT_UPD_EOF;
  set_vram_update(vbuf);
}

// --- Formatting. ---

uint8_t u32toa(uint32_t v, char *buf) {
  char tmp[10];
  uint8_t n = 0;
  do {
    tmp[n++] = '0' + (uint8_t)(v % 10);
    v /= 10;
  } while (v);
  for (uint8_t i = 0; i < n; ++i) buf[i] = tmp[n - 1 - i];
  return n;
}

uint8_t u16toa(uint16_t v, char *buf) { return u32toa(v, buf); }

void hex_bytes(const uint8_t *bytes, uint8_t n, char *buf) {
  static const char hx[] = "0123456789abcdef";
  for (uint8_t i = 0; i < n; ++i) {
    buf[i * 2] = hx[bytes[i] >> 4];
    buf[i * 2 + 1] = hx[bytes[i] & 15];
  }
}

// Largest unit + next, zero-padded: "5m 03s", "2h 14m", "9d 05h", "31y 200d".
static uint8_t two(char *b, uint32_t v) {  // ' ' + zero-padded 2-digit lower unit
  b[0] = ' ';
  if (v < 10) { b[1] = '0'; b[2] = '0' + (char)v; return 3; }
  return 1 + u32toa(v, b + 1);
}

uint8_t fmt_dur(uint32_t s, char *b) {
  uint8_t n = 0;
  if (s < 60) {
    n = u32toa(s, b); b[n++] = 's';
  } else if (s < 3600UL) {
    n = u32toa(s / 60, b); b[n++] = 'm'; n += two(b + n, s % 60); b[n++] = 's';
  } else if (s < 86400UL) {
    n = u32toa(s / 3600, b); b[n++] = 'h'; n += two(b + n, (s % 3600) / 60); b[n++] = 'm';
  } else if (s < 31556952UL) {
    n = u32toa(s / 86400, b); b[n++] = 'd'; n += two(b + n, (s % 86400) / 3600); b[n++] = 'h';
  } else {
    n = u32toa(s / 31556952UL, b); b[n++] = 'y';
    b[n++] = ' '; n += u32toa((s % 31556952UL) / 86400, b + n); b[n++] = 'd';
  }
  return n;
}
