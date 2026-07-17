#ifndef HASHCAT_NES_CRC32_H
#define HASHCAT_NES_CRC32_H
#include <stdint.h>
uint32_t crc32(const uint8_t *data, uint16_t len);
#endif
