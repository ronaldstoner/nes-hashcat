// Text helpers: direct nametable writes (rendering off) and a neslib-format
// VRAM update buffer for the animated cracking screen (rendering on).
#ifndef HASHCAT_NES_TEXT_H
#define HASHCAT_NES_TEXT_H

#include <stdint.h>

// --- direct writes; only valid while rendering is OFF ---
void text_at(uint8_t x, uint8_t y, const char *s);
void text_n(uint8_t x, uint8_t y, const char *s, uint8_t n);
void text_clear(void);                 // fill nametable A with blanks
void text_hline(uint8_t x, uint8_t y, uint8_t w, uint8_t tile);
void attr_fill(uint8_t pal);           // set every attribute byte to one palette
void attr_bands(const uint8_t bands[8]);  // palette per 32px-tall horizontal band

// --- deferred VRAM update buffer (flushed by neslib during NMI) ---
void vb_reset(void);
void vb_str(uint8_t x, uint8_t y, const char *s);
void vb_strn(uint8_t x, uint8_t y, const char *s, uint8_t n);
void vb_commit(void);                  // terminate + hand buffer to neslib

// --- number formatting; returns digit count, writes into buf (no NUL) ---
uint8_t u32toa(uint32_t v, char *buf);
uint8_t u16toa(uint16_t v, char *buf);

// lowercase hex of n bytes -> 2n chars into buf (no NUL)
void hex_bytes(const uint8_t *bytes, uint8_t n, char *buf);

// Human duration ("42s", "5m 03s", "2h 14m", "9d 05h", "31y 200d") -> buf.
uint8_t fmt_dur(uint32_t seconds, char *buf);

#endif
