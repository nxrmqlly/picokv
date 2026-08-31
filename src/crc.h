#pragma once
#include <stdint.h>
#include <stdlib.h>

uint32_t crc32_start(void);
uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t length);
uint32_t crc32_finish(uint32_t crc);
