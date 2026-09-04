// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026-present Ritam Das

#include <stdint.h>
#include <stdlib.h>

uint32_t crc32_start(void) { return 0xFFFFFFFF; }

uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    uint8_t byte = data[i];
    crc ^= byte;

    for (int j = 0; j < 8; ++j) {
      uint8_t lowest = crc & 1;

      if (lowest == 1) {
        crc = (crc >> 1) ^ 0xEDB88320; // CRC-32/ISO-HDLC polynomial
      } else {
        crc = crc >> 1;
      }
    }
  }
  return crc;
}

uint32_t crc32_finish(uint32_t crc) { return crc ^ 0xFFFFFFFF; }
