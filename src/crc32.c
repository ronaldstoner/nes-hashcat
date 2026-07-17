// CRC-32 (IEEE / zlib, reflected poly 0xEDB88320). Table-driven: one lookup +
// shift per byte instead of 8 bit-iterations (table in PRG-ROM).
#include "crc32.h"
#include "crc32_table.h"

uint32_t crc32(const uint8_t *data, uint16_t len) {
  uint32_t crc = 0xffffffffUL;
  uint16_t i;
  for (i = 0; i < len; ++i)
    crc = (crc >> 8) ^ CRC32_TAB[(uint8_t)(crc ^ data[i])];
  return crc ^ 0xffffffffUL;
}
