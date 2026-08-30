#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * PicoKV File format
 * ------------------
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

uint32_t crc32_start() { return 0xFFFFFFFF; }

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
uint32_t crc32_start(uint32_t crc) { return crc ^= 0xFFFFFFFF; }

void picokv_write_header(FILE *fp) {
  // Write 12B of header
  uint32_t magic = PICOKV_MAGIC;     // 4B
  uint16_t version = PICOKV_VERSION; // 2B
  uint8_t reserved[6] = {0};         // 6B

  fwrite(&magic, sizeof(magic), 1, fp);
  fwrite(&version, sizeof(version), 1, fp);
  fwrite(&reserved, sizeof(reserved), 1, fp);
}

void picokv_write_rec_header(RecHeader h, FILE *fp) {
  fwrite(&h.op, sizeof(h.op), 1, fp);
  fwrite(&h.crc, sizeof(h.crc), 1, fp);
  fwrite(&h.k_sz, sizeof(h.k_sz), 1, fp);
  fwrite(&h.v_sz, sizeof(h.v_sz), 1, fp);
}

void picokv_write_record(uint8_t op, char *k, char *v, FILE *fp) {
  RecHeader h = {
      .op = op,
      .k_sz = strlen(k),
      .v_sz = strlen(v),
  };

  picokv_write_rec_header(h, fp);
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
  return 0;

  const uint8_t test[] = "123456789";
  uint32 crc = crc32_start();
  uint32_t res = crc32_update(crc, test, sizeof(test) - 1);

  printf("dat: %s\n", test);
  printf("crc: %08X\n", res);

  return 0;
}
