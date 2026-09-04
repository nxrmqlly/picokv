#pragma once

#include <stdint.h>
#include <stdio.h>

static const char PICOKV_MAGIC[4] = {'p', '1', 'c', 'o'};
#define PICOKV_VERSION 0x0001

#define PICOKV_OP_SET 0x01
#define PICOKV_OP_DEL 0x02

// 4 KiB
#define MAX_K_SZ (4 * 1024)
// 1 MiB
#define MAX_V_SZ (1 * 1024 * 1024)

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
  char *k;
  char *v;
} Record;

int picokv_read_header(Header *out, FILE *fp);
void picokv_write_header(FILE *fp);

int picokv_write_record(uint8_t op, const char *k, const char *v, FILE *fp);
int picokv_read_record(Record *out, FILE *fp);

void picokv_free_record(Record *r);
