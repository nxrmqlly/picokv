#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * PicoKV File format
 * ------------------
 *
 * All multi byte numbers are little endian
 *
 * Header (12b):
 * - magic   : 4B ("p1c0")
 * - version : 2B
 * - reserved: 6B
 * Records:
 * - op  : 1B
 * - crc : 4B
 * - k_sz: 4B (CRC32(k+v))
 * - v_sz: 4B (0 IF op = DEL)
 * - k   : <k_sz>B
 * - v   : <v_sz>B
 * Op:
 * - SET: 0x01
 * - DEL: 0x02
 */

#define PICOKV_MAGIC 0x70316330
#define PICOKV_VERSION 0x0001

#define PICOKV_OP_SET 0x01
#define PICOKV_OP_DEL 0x02

typedef struct {
  uint32_t magic;
  uint16_t version;
  uint8_t reserved[6];
} Header;

typedef struct {
  uint8_t op;
  uint32_t crc;
  uint32_t k_sz;
  uint32_t v_sz;
} RecHeader;

void write_le(uint64_t x, size_t n, FILE *fp) {
  for (size_t i = 0; i < n; i++) {
    uint8_t byte = (uint8_t)(x >> (i * 8));
    fwrite(&byte, 1, 1, fp);
  }
}

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

uint32_t crc32_finish(uint32_t crc) { return crc ^= 0xFFFFFFFF; }

void picokv_write_header(FILE *fp) {
  // Write 12B of header
  uint32_t magic = PICOKV_MAGIC;     // 4B
  uint16_t version = PICOKV_VERSION; // 2B
  uint8_t reserved[6] = {0};         // 6B

  write_le(magic, 4, fp);
  write_le(version, 2, fp);
  fwrite(&reserved, sizeof(reserved), 1, fp);
}

void picokv_write_rec_header(RecHeader h, FILE *fp) {
  fwrite(&h.op, sizeof(h.op), 1, fp);
  fwrite(&h.crc, sizeof(h.crc), 1, fp);
  write_le(h.k_sz, 4, fp);
  write_le(h.v_sz, 4, fp);
}

void picokv_write_record(uint8_t op, char *k, char *v, FILE *fp) {
  uint32_t k_sz = strlen(k);
  uint32_t v_sz = strlen(v);
  uint32_t crc = crc32_start();
  crc = crc32_update(crc, (const uint8_t *)k, k_sz);
  crc = crc32_update(crc, (const uint8_t *)v, v_sz);
  crc = crc32_finish(crc);

  RecHeader h = {
      .op = op,
      .crc = crc,
      .k_sz = k_sz,
      .v_sz = v_sz,
  };

  picokv_write_rec_header(h, fp);
  fwrite((const uint8_t *)k, strlen(k), 1, fp);
  fwrite((const uint8_t *)v, strlen(v), 1, fp);
}

int main() {
  FILE *file_ptr;
  file_ptr = fopen("test.picokv", "wb");

  if (file_ptr == NULL) {
    printf("I/O error while opening file.\n");
    exit(1);
  }

  picokv_write_header(file_ptr);

  fclose(file_ptr);

  const uint8_t test[] = "123456789";
  uint32_t res =
      crc32_finish(crc32_update(crc32_start(), test, sizeof(test) - 1));

  printf("dat: %s\n", test);
  printf("crc: %08X\n", res);

  return 0;
}
