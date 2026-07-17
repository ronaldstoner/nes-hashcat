#include "sound.h"

// Direct APU access. neslib doesn't touch these unless famitone2 is used, and
// it isn't here, so the two pulse channels, triangle and noise are ours.
#define APU(reg) (*(volatile uint8_t *)(0x4000 + (reg)))

// Melody format: triples {period_lo, period_hi, duration_frames}, 0 duration
// ends. Periods are NES timer values for the target pitch.

// Pulse period lows/highs for the notes we use.
#define G4 0x1c, 0x01
#define C5 0xd4, 0x00
#define E5 0xa9, 0x00
#define G5 0x8e, 0x00
#define A5 0x7e, 0x00
#define C6 0x6a, 0x00
#define E6 0x54, 0x00
#define G6 0x46, 0x00

static const uint8_t mel_move[] = {C5, 3, 0, 0, 0};
static const uint8_t mel_select[] = {C5, 4, G5, 6, 0, 0, 0};
// Ascending arpeggio finishing on a held high tonic -- the "trumpet" fanfare.
static const uint8_t mel_victory[] = {
    G4, 6, C5, 6, E5, 6, G5, 6, C6, 8, E6, 8, G6, 10, C6, 6, E6, 6, G6, 30, 0, 0, 0};
// Triangle bass line under the fanfare.
static const uint8_t mel_bass[] = {0xab, 0x01, 24, 0xd5, 0x00, 24, 0x6a, 0x00, 48, 0, 0, 0};

typedef struct {
  const uint8_t *ptr;
  uint8_t timer;
  uint8_t base;   // first APU register of the channel
  uint8_t trig;   // value written to the volume/control register on note-on
  uint8_t off;    // value written to silence the channel
} voice_t;

static voice_t p1, p2, tri;

static void voice_start(voice_t *v, const uint8_t *mel) {
  v->ptr = mel;
  v->timer = 0;
}

static void voice_step(voice_t *v) {
  if (!v->ptr) return;
  if (v->timer) { --v->timer; return; }
  uint8_t lo = v->ptr[0], hi = v->ptr[1], dur = v->ptr[2];
  if (dur == 0) {          // end of melody
    APU(v->base) = v->off;
    v->ptr = 0;
    return;
  }
  v->ptr += 3;
  v->timer = dur - 1;
  APU(v->base) = v->trig;         // duty / volume (or triangle control)
  APU(v->base + 2) = lo;          // period low
  APU(v->base + 3) = hi;          // period high (retriggers the channel)
}

void snd_init(void) {
  APU(0x15) = 0x0f;               // enable pulse1, pulse2, triangle, noise
  APU(0x01) = 0x08;               // pulse1 sweep off
  APU(0x05) = 0x08;               // pulse2 sweep off
  p1.base = 0x00; p1.trig = 0xbf; p1.off = 0x30;   // pulse1, duty 50%, vol 15
  p2.base = 0x04; p2.trig = 0xb8; p2.off = 0x30;   // pulse2, softer
  tri.base = 0x08; tri.trig = 0xff; tri.off = 0x00; // triangle control
  p1.ptr = p2.ptr = tri.ptr = 0;
}

void snd_update(void) {
  voice_step(&p1);
  voice_step(&p2);
  voice_step(&tri);
}

void snd_move(void) { voice_start(&p2, mel_move); }
void snd_select(void) { voice_start(&p2, mel_select); }

void snd_victory(void) {
  voice_start(&p1, mel_victory);
  voice_start(&tri, mel_bass);
}

// Faint noise "crunch" so the cracking screen sounds like it's chewing.
void snd_tick(void) {
  APU(0x0c) = 0x31;   // noise, short constant volume 1
  APU(0x0e) = 0x0c;   // high frequency period
  APU(0x0f) = 0x08;   // trigger
}

// Release every voice and hard-mute all four channels. Called on every scene
// transition so a half-finished jingle can never bleed into the next screen.
void snd_silence(void) {
  p1.ptr = p2.ptr = tri.ptr = 0;
  APU(0x00) = 0x30;   // pulse1 vol 0
  APU(0x04) = 0x30;   // pulse2 vol 0
  APU(0x08) = 0x00;   // triangle halt
  APU(0x0c) = 0x30;   // noise vol 0
}
