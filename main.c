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
 * Header (16B):
 * - magic   : 4B ("p1c0")
 * - version : 2B
 * - reserved: 10B
 *
 * Record Header (16B):
 * - op     : 1B
 * - padding: 3B (reserved, 00)
 * - crc    : 4B
 * - k_sz   : 4B (CRC32(k || v))
 * - v_sz   : 4B (0 IF op = DEL)
 * Record (<k_sz + v_sz>B)
 * - k      : <k_sz>B
 * - v      : <v_sz>B
 * Op:
 * - SET: 0x01
 * - DEL: 0x02
 */

static const char PICOKV_MAGIC[4] = "p1c0";
#define PICOKV_VERSION 0x0001

#define PICOKV_OP_SET 0x01
#define PICOKV_OP_DEL 0x02

typedef struct {
  char magic[4];
  uint16_t version;
  uint8_t reserved[10];
} Header;

typedef struct {
  uint8_t op;
  uint8_t padding[3];
  uint32_t crc;
  uint32_t k_sz;
  uint32_t v_sz;

  uint8_t *k;
  uint8_t *v;
} Record;

void write_le(uint64_t x, size_t n, FILE *fp) {
  for (size_t i = 0; i < n; i++) {
    uint8_t byte = (uint8_t)(x >> (i * 8));
    fwrite(&byte, 1, 1, fp);
  }
}

uint64_t read_le(size_t n, FILE *fp) {
  uint64_t x = 0;
  for (size_t i = 0; i < n; i++) {
    uint8_t byte;
    fread(&byte, 1, 1, fp);
    x |= (uint64_t)byte << (i * 8);
  }
  return x;
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
  // Write 16B of header
  uint8_t reserved[10] = {0}; // 10B
  fwrite(PICOKV_MAGIC, 4, 1, fp);
  write_le(PICOKV_VERSION, 2, fp);
  fwrite(reserved, sizeof reserved, 1, fp);
}

Header picokv_read_header(FILE *fp) {
  Header h;
  fread(h.magic, sizeof h.magic, 1, fp);
  h.version = read_le(sizeof h.version, fp);
  fread(h.reserved, sizeof h.reserved, 1, fp);

  return h;
}

void _picokv_write_rec_header(const Record *r, FILE *fp) {
  const char padding[3] = {0};
  fwrite(&r->op, sizeof r->op, 1, fp);
  fwrite(padding, sizeof padding, 1, fp);
  write_le(r->crc, sizeof r->crc, fp);
  write_le(r->k_sz, sizeof r->k_sz, fp);
  write_le(r->v_sz, sizeof r->v_sz, fp);
}

Record _picokv_read_rec_header(FILE *fp) {
  Record r;
  r.op = read_le(1, fp);
  fread(r.padding, 3, 1, fp);
  r.crc = read_le(sizeof r.crc, fp);
  r.k_sz = read_le(sizeof r.k_sz, fp);
  r.v_sz = read_le(sizeof r.v_sz, fp);
  return r;
}

void picokv_write_record(uint8_t op, char *k, char *v, FILE *fp) {
  uint32_t k_sz = strlen(k);
  uint32_t v_sz = op == PICOKV_OP_DEL ? 0 : strlen(v);
  uint32_t crc = crc32_start();
  crc = crc32_update(crc, (const uint8_t *)k, k_sz);
  crc = crc32_update(crc, (const uint8_t *)v, v_sz);
  crc = crc32_finish(crc);

  Record h = {
      .op = op,
      .crc = crc,
      .k_sz = k_sz,
      .v_sz = v_sz,
  };

  _picokv_write_rec_header(&h, fp);
  fwrite((const uint8_t *)k, k_sz, 1, fp);
  fwrite((const uint8_t *)v, v_sz, 1, fp);
}

Record picokv_read_record(FILE *fp) {
  Record r = _picokv_read_rec_header(fp);

  r.k = malloc(r.k_sz + 1); // +1 for \0
  r.v = malloc(r.v_sz + 1);
  fread(r.k, r.k_sz, 1, fp);
  fread(r.v, r.v_sz, 1, fp);
  r.k[r.k_sz] = '\0'; // convert to c-string
  r.v[r.v_sz] = '\0';

  return r;
}

void picokv_free_record(Record *r) {
  free(r->k);
  free(r->v);
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
}
