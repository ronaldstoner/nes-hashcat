// hashcat-nes: a joke a-z brute-force cracker for the NES over five hash modes
// (MD5, SHA-1, SHA-256, NTLM, CRC32). Picks a random secret, hashes it, and
// grinds candidates until it matches -- fast for tiny lengths, forever for big.
#include <ines.h>
#include <neslib.h>
#include "scenes.h"

MAPPER_PRG_ROM_KB(32);
MAPPER_CHR_ROM_KB(8);
MAPPER_USE_VERTICAL_MIRRORING;

// Generated CHR-ROM (font at tile==ASCII); referenced so LTO keeps it.
extern const unsigned char chr_rom[8192];

int main(void) {
  ppu_off();
  (void)chr_rom;
  game_run();
  return 0;
}
