#include "scenes.h"
#include "text.h"
#include "sound.h"
#include "hash.h"
#include "brute.h"
#include "chr.h"
#include "gen_title.h"
#include <neslib.h>

// neslib bumps this every NMI (once per TV frame). Reading its delta lets the
// crack loop measure a real hashes/sec without ever blocking on the PPU.
extern volatile uint8_t FRAME_CNT1;

volatile uint8_t g_scene;
volatile uint8_t g_found;

// Menu configuration.
static const uint8_t LEN_OPTS[] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 20, 25, 50, 100};
#define LEN_COUNT (sizeof(LEN_OPTS))
static uint8_t cfg_algo = ALGO_MD5;
static uint8_t cfg_len_idx = 3;   // -> length 4

// Work buffers.
static uint8_t target[32];
static uint8_t digest[32];
static char line[40];   // never format more than 40 chars into this

// --- Palettes ---
static const uint8_t PAL[16] = {
    0x0f, 0x2a, 0x1a, 0x30,   // 0: green terminal
    0x0f, 0x30, 0x10, 0x00,   // 1: white
    0x0f, 0x16, 0x27, 0x30,   // 2: red / amber
    0x0f, 0x2c, 0x21, 0x30,   // 3: cyan (title)
};

static void wait_frame(void) { ppu_wait_nmi(); }

// Append "label" + decimal(v), padded with trailing spaces to erase leftovers.
static void vb_labeled(uint8_t x, uint8_t y, const char *label, uint32_t v) {
  uint8_t n = 0;
  while (label[n]) { line[n] = label[n]; n++; }
  n += u32toa(v, line + n);
  while (n < 22) line[n++] = ' ';
  vb_strn(x, y, line, n);
}

// Append "label" + text, padded to erase the previous (possibly longer) value.
static void vb_labeled_str(uint8_t x, uint8_t y, const char *label, const char *val) {
  uint8_t n = 0;
  while (label[n]) { line[n] = label[n]; ++n; }
  uint8_t j = 0;
  while (val[j]) line[n++] = val[j++];
  while (n < 24) line[n++] = ' ';
  vb_strn(x, y, line, n);
}

// Append "label" + a formatted duration, padded to erase the old value.
static void vb_labeled_dur(uint8_t x, uint8_t y, const char *label, uint32_t seconds) {
  uint8_t n = 0;
  while (label[n]) { line[n] = label[n]; ++n; }
  n += fmt_dur(seconds, line + n);
  while (n < 22) line[n++] = ' ';
  vb_strn(x, y, line, n);
}

// NES palette values cycled through the victory "MATCH FOUND" text.
static const uint8_t RAINBOW[8] = {0x16, 0x27, 0x28, 0x2a, 0x2b, 0x21, 0x24, 0x25};

// --- Title ---
#define LOGO_Y 3
static void draw_title(void) {
  ppu_off();
  text_clear();
  // Per-band palettes: logo white, caption red, press-start white, credits cyan.
  static const uint8_t bands[8] = {1, 1, 1, 2, 1, 0, 3, 3};
  attr_bands(bands);

#if LOGO_OK
  for (uint8_t ty = 0; ty < LOGO_H; ++ty)
    text_n(0, LOGO_Y + ty, (const char *)(logo_map + ty * LOGO_W), LOGO_W);
#else
  text_at(6, 5, "H A S H C A T");
#endif

  text_at(3, 13, "ADVANCED PASSWORD RECOVERY");   // band 3 -> red
  text_at(10, 17, "PRESS START");                 // band 4 -> white (blinks)
  text_at(4, 25, "RON STONER * STONER.COM");      // band 6 -> cyan
  text_at(6, 27, "SHOUTZ TO SOLST/ICE");          // band 6 -> cyan
  ppu_on_all();
}

static void scene_title(void) {
  g_scene = SC_TITLE;
  snd_silence();
  draw_title();
  uint8_t blink = 0;
  uint16_t entropy = 0;
  for (;;) {
    wait_frame();
    snd_update();
    ++entropy;
    if ((++blink & 31) == 0) {
      vb_reset();
      if (blink & 32) vb_str(10, 17, "PRESS START");
      else vb_str(10, 17, "           ");
      vb_commit();
    }
    if (pad_trigger(0) & PAD_START) {
      set_rand(entropy ? entropy : 1);
      snd_select();
      set_vram_update(0);
      return;
    }
  }
}

// --- Menu ---
static void draw_menu_static(void) {
  ppu_off();
  text_clear();
  // Attribute table (palette per 16x16 quadrant): cyan header/bar/labels, rest
  // white. Each byte is BR|BL|TR|TL x2 bits; 0x55=all white, 0xff=all cyan.
  {
    uint8_t a[64], i;
    for (i = 0; i < 64; ++i) a[i] = 0x55;   // white everywhere
    for (i = 0; i < 8; ++i) a[i] = 0xff;    // header band (y0-3) cyan
    for (i = 8; i < 16; ++i) a[i] = 0x5f;   // bar row (y4) top quadrants cyan
    a[17] = 0xf5;                           // "ALGO" label (y10, cols 4-7) cyan
    a[25] = 0x5f;                           // "LEN"  label (y13, cols 4-7) cyan
    vram_adr(NAMETABLE_A + 0x3c0);
    vram_write(a, 64);
  }
  text_at(8, 2, "-- CONFIGURE --");
  text_hline(2, 4, 28, TILE_SOLID);
  text_at(4, 10, "ALGO:");
  text_at(4, 13, "LEN :");
  text_at(4, 20, "START: begin cracking");
  text_at(4, 22, "L/R: change   U/D: move");
  text_at(4, 24, "B  : back to title");
  ppu_on_all();
}

static void menu_refresh(uint8_t cursor) {
  vb_reset();
  // cursor markers
  vb_str(2, 10, cursor == 0 ? ">" : " ");
  vb_str(2, 13, cursor == 1 ? ">" : " ");

  // ALGO line: name + hashcat mode number.
  uint8_t n = 0;
  const char *nm = algo_name(cfg_algo);
  while (nm[n]) { line[n] = nm[n]; ++n; }
  while (n < 8) line[n++] = ' ';
  line[n++] = '('; line[n++] = 'm'; line[n++] = ' ';
  n += u16toa(algo_mode(cfg_algo), line + n);
  line[n++] = ')';
  while (n < 20) line[n++] = ' ';
  vb_strn(10, 10, line, n);

  // LEN line + keyspace hint (26^len is astronomical; just show 26^n text).
  n = u16toa(LEN_OPTS[cfg_len_idx], line);
  line[n++] = ' '; line[n++] = 'c'; line[n++] = 'h'; line[n++] = 'a'; line[n++] = 'r';
  if (LEN_OPTS[cfg_len_idx] != 1) line[n++] = 's';
  while (n < 14) line[n++] = ' ';
  line[n++] = '2'; line[n++] = '6'; line[n++] = '^';
  n += u16toa(LEN_OPTS[cfg_len_idx], line + n);
  while (n < 22) line[n++] = ' ';
  vb_strn(10, 13, line, n);
  vb_commit();
}

// returns 1 -> go to crack, 0 -> back to title
static uint8_t scene_menu(void) {
  g_scene = SC_MENU;
  snd_silence();
  draw_menu_static();
  uint8_t cursor = 0;
  menu_refresh(cursor);
  for (;;) {
    wait_frame();
    snd_update();
    uint8_t t = pad_trigger(0);
    if (t & PAD_UP && cursor) { cursor = 0; snd_move(); }
    if (t & PAD_DOWN && !cursor) { cursor = 1; snd_move(); }
    if (t & (PAD_LEFT | PAD_RIGHT)) {
      if (cursor == 0) {
        if (t & PAD_RIGHT) cfg_algo = (cfg_algo + 1) % ALGO_COUNT;
        else cfg_algo = (cfg_algo + ALGO_COUNT - 1) % ALGO_COUNT;
      } else {
        if (t & PAD_RIGHT) cfg_len_idx = (cfg_len_idx + 1) % LEN_COUNT;
        else cfg_len_idx = (cfg_len_idx + LEN_COUNT - 1) % LEN_COUNT;
      }
      snd_move();
    }
    if (t & PAD_START) { snd_select(); set_vram_update(0); return 1; }
    if (t & PAD_B) { snd_move(); set_vram_update(0); return 0; }
    if (t) menu_refresh(cursor);
  }
}

// --- Crack ---
// Bands shared by crack + victory: header/timers cyan, target/stats white, status red.
static const uint8_t CRACK_BANDS[8] = {3, 1, 2, 1, 3, 1, 0, 0};

// Draw the header + algo + target-hash block (identical on crack & victory).
static void draw_crack_header(uint8_t len) {
  uint8_t n = 0;
  const char *h = "HASHCAT-NES  MODE ";
  while (h[n]) { line[n] = h[n]; ++n; }
  n += u16toa(algo_mode(cfg_algo), line + n);
  text_n(2, 2, line, n);   // header down 1 row to clear TV overscan

  n = 0;
  const char *nm = algo_name(cfg_algo);
  while (nm[n]) { line[n] = nm[n]; ++n; }
  text_n(2, 3, line, n);
  text_at(14, 3, "LEN:");
  n = u16toa(len, line);
  text_n(19, 3, line, n);

  // Print the target hex 14 bytes (28 chars) per line, so SHA-256's 64-char
  // digest never overflows the 40-byte `line` buffer.
  uint8_t dl = hash_digest_len(cfg_algo);      // 4..32 bytes
  text_at(2, 4, "TARGET:");
  for (uint8_t boff = 0, y = 5; boff < dl; boff += 14, ++y) {
    uint8_t chunk = (dl - boff < 14) ? dl - boff : 14;
    hex_bytes(target + boff, chunk, line);
    text_n(2, y, line, chunk * 2);
  }
}

static void draw_crack_static(uint8_t len) {
  ppu_off();
  text_clear();
  attr_bands(CRACK_BANDS);
  draw_crack_header(len);
  text_at(2, 9, "STATUS: BRUTE FORCING a-z");
  text_at(2, 12, "TRY:");
  text_at(2, 21, "B / START: abort");
  ppu_on_all();
}

// Show the candidate at "TRY: ", truncated with ".." to fit the screen width.
static void show_guess(uint8_t y) {
  uint8_t len = brute_len, n = 0;
  if (len > 22) {
    for (; n < 22; ++n) line[n] = brute_cand[n];
    line[n++] = '.'; line[n++] = '.';
  } else {
    for (; n < len; ++n) line[n] = brute_cand[n];
    while (n < 24) line[n++] = ' ';  // erase the previous (longer) guess
  }
  vb_strn(7, y, line, n);
}

// 26^len as a 64-bit keyspace; sets *of if it overflows (len >= 14).
static uint64_t keyspace(uint8_t len, uint8_t *of) {
  uint64_t ks = 1;
  *of = 0;
  for (uint8_t i = 0; i < len; ++i) {
    if (ks > 0xFFFFFFFFFFFFFFFFULL / 26) { *of = 1; return 0; }
    ks *= 26;
  }
  return ks;
}

static void scene_crack(void) {
  g_scene = SC_CRACK;
  g_found = 0;
  snd_silence();
  uint8_t len = LEN_OPTS[cfg_len_idx];

  brute_reset(len);
  {
    uint8_t secret[BRUTE_MAX];               // only needed to build the target
    brute_secret(secret);
    hash_compute(cfg_algo, secret, len, target);
  }
  uint8_t dl = hash_begin(cfg_algo, len);   // arm the prepared-block fast path

  draw_crack_static(len);

  uint8_t ks_of;
  uint64_t ks = keyspace(len, &ks_of);       // full a-z keyspace, for the ETA

  uint32_t tries = 0, win_hashes = 0, run_frames = 0;
  uint16_t rate = 0;
  uint8_t last_fc = FRAME_CNT1;
  uint8_t found = 0, spin = 0, abort_armed = 0;

  for (;;) {
    // Hash flat-out for ~4 frames between refreshes: neslib only flushes VRAM
    // in ppu_wait_nmi, so amortizing that idle frame keeps ~80% hashing duty.
    while (!found && (uint8_t)(FRAME_CNT1 - last_fc) < 4) {
      uint8_t k;
      for (k = 0; k < 4; ++k) {
        hash_next(brute_cand, digest);
        ++tries;
        ++win_hashes;
        if (digest_equal_n(digest, target, dl)) { found = 1; break; }
        brute_next();   // secret is always in range; this never wraps first
      }
    }
    if (found) { g_found = 1; break; }

    // Abort on level (held), not edge: slow modes poll only every ~20 frames,
    // so a quick tap was missed. Debounce so a held launch-Start doesn't abort.
    uint8_t held = pad_poll(0) & (PAD_B | PAD_START);
    if (!held) abort_armed = 1;
    else if (abort_armed) { snd_silence(); set_vram_update(0); return; }

    {
      uint8_t now = FRAME_CNT1;
      uint8_t df = now - last_fc;
      rate = (uint16_t)((win_hashes * 60UL) / df);
      run_frames += df;
      win_hashes = 0;
      last_fc = now;
      ++spin;

      // Small VRAM updates to fit vblank: odometer + spinner every frame, the
      // four stat lines round-robin one per frame (a full rewrite overruns it).
      set_vram_update(0);
      vb_reset();
      line[0] = "|/-\\"[spin & 3];
      vb_strn(28, 9, line, 1);                     // spinner by STATUS
      show_guess(12);
      switch (spin & 3) {
        case 0:
          vb_labeled(2, 14, "HASHES: ", tries);
          break;
        case 1:
          vb_labeled(2, 15, "RATE  : ", rate);
          vb_str(18, 15, "H/s");
          break;
        case 2:
          if (rate == 0)
            vb_labeled_str(2, 16, "EST   : ", "computing...");
          else if (ks_of)                          // keyspace too vast to even count
            vb_labeled_str(2, 16, "EST   : ", "ETERNITY");
          else {
            uint64_t est = ks / rate;
            if (est > 3155695200ULL) vb_labeled_str(2, 16, "EST   : ", "> 100 YEARS");
            else vb_labeled_dur(2, 16, "EST   : ", (uint32_t)est);
          }
          break;
        case 3:
          vb_labeled_dur(2, 17, "RUN   : ", run_frames / 60);
          break;
      }
      vb_commit();

      if ((spin & 3) == 0) snd_tick();
      snd_update();
      ppu_wait_nmi();  // one flush per ~4 frames of hashing
    }
  }

  // --- victory ---------------------------------------------------------------
  set_vram_update(0);
  ppu_off();
  text_clear();
  attr_bands(CRACK_BANDS);
  draw_crack_header(len);
  text_at(2, 9, "STATUS: ** MATCH FOUND **");
  text_at(2, 12, "PLAINTEXT RECOVERED:");
  {
    uint8_t show = (len < 26) ? len : 26, i = 0;
    for (; i < show; ++i) line[i] = brute_cand[i];
    text_n(2, 13, line, show);
  }
  { uint8_t n = 0; const char *l = "CRACKED IN: ";
    while (l[n]) { line[n] = l[n]; ++n; }
    n += fmt_dur(run_frames / 60, line + n); text_n(2, 15, line, n); }
  { uint8_t n = 0; const char *l = "HASHES TRIED: ";
    while (l[n]) { line[n] = l[n]; ++n; }
    n += u32toa(tries, line + n); text_n(2, 17, line, n); }
  text_at(2, 21, "START: back to menu");
  ppu_on_all();
  snd_victory();

  // Victory animation: one-shot white flash-in (`flash`), then a forever
  // rainbow-cycled MATCH banner (`hue`).
  pal_bright(8);
  uint8_t flash = 6, hue = 0;
  for (;;) {
    wait_frame();
    snd_update();
    if (flash) { pal_bright(4 + flash / 2); --flash; }   // 7,6,6,5,5,4 -> normal, once
    pal_col(2 * 4 + 1, RAINBOW[(hue >> 2) & 7]);          // BG palette 2, color 1
    ++hue;
    if (pad_trigger(0) & (PAD_START | PAD_A)) {
      snd_silence();
      pal_bright(4);
      pal_col(2 * 4 + 1, 0x16);                            // restore red
      return;
    }
  }
}

void game_run(void) {
  pal_bg(PAL);
  pal_spr(PAL);
  oam_clear();
  snd_init();
  for (;;) {
    scene_title();
    // Stay in the menu/crack cycle until the player backs out with B.
    while (scene_menu())
      scene_crack();
  }
}
