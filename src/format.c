// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026-present Ritam Das

#include "format.h"
#include "crc.h"
#include "pkverr.h"
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
 * - crc    : 4B (CRC32(k || v))
 * - k_sz   : 4B
 * - v_sz   : 4B (0 IF op = DEL)
 * Record (<k_sz + v_sz>B)
 * - k      : <k_sz>B
 * - v      : <v_sz>B
 * Op:
 * - SET: 0x01
 * - DEL: 0x02
 */

static void safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  if (size == 0 || nmemb == 0)
    return;

  size_t ir = fread(ptr, size, nmemb, stream);

  if (ir != nmemb) {
    if (feof(stream)) {
      fprintf(stderr, "Fatal: Unexpected EOF\n");
    } else if (ferror(stream)) {
      perror("Fatal: Failed to read from file");
    } else {
      fprintf(stderr, "Fatal: Partial read\n");
    }
    exit(1);
  }
}

static void safe_fwrite(const void *ptr, size_t size, size_t nmemb,
                        FILE *stream) {
  if (size == 0 || nmemb == 0)
    return;

  size_t iw = fwrite(ptr, size, nmemb, stream);
  if (iw != nmemb) {
    perror("Fatal: Failed to write to file");
    exit(1);
  }
}

// Writes strictly in little endian. Crashes on IO error
static void write_le(uint64_t x, size_t n, FILE *fp) {
  uint8_t buf[8];
  for (size_t i = 0; i < n; i++) {
    buf[i] = (uint8_t)(x >> (i * 8));
  }
  safe_fwrite(buf, 1, n, fp);
}

// Reads little endian. Crashes on IO error
static uint64_t read_le(size_t n, FILE *fp) {
  uint8_t buf[8] = {0};
  safe_fread(buf, 1, n, fp);

  uint64_t x = 0;
  for (size_t i = 0; i < n; i++) {
    x |= (uint64_t)buf[i] << (i * 8);
  }
  return x;
}

void picokv_write_header(FILE *fp) {
  // Write 16B of header
  uint8_t reserved[10] = {0}; // 10B
  safe_fwrite(PICOKV_MAGIC, 4, 1, fp);
  write_le(PICOKV_VERSION, 2, fp);
  safe_fwrite(reserved, sizeof reserved, 1, fp);
}

static void picokv_write_rec_header(const Record *r, FILE *fp) {
  const char padding[3] = {0};
  safe_fwrite(&r->op, sizeof r->op, 1, fp);
  safe_fwrite(padding, sizeof padding, 1, fp);
  write_le(r->crc, sizeof r->crc, fp);
  write_le(r->k_sz, sizeof r->k_sz, fp);
  write_le(r->v_sz, sizeof r->v_sz, fp);
}

int picokv_write_record(uint8_t op, const char *k, const char *v, FILE *fp) {
  if (op != PICOKV_OP_SET && op != PICOKV_OP_DEL) {
    return PICOKV_ERR_BADOP;
  }

  uint32_t k_sz = strlen(k);
  uint32_t v_sz = op == PICOKV_OP_DEL ? 0 : strlen(v);
  if (k_sz > MAX_K_SZ || v_sz > MAX_V_SZ) {
    return PICOKV_ERR_SIZE;
  }
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

  picokv_write_rec_header(&h, fp);
  safe_fwrite((const uint8_t *)k, k_sz, 1, fp);
  safe_fwrite((const uint8_t *)v, v_sz, 1, fp);

  return 0;
}

int picokv_read_header(Header *out, FILE *fp) {
  safe_fread(out->magic, sizeof out->magic, 1, fp);
  out->version = read_le(sizeof out->version, fp);
  safe_fread(out->reserved, sizeof out->reserved, 1, fp);

  if (memcmp(out->magic, PICOKV_MAGIC, 4) != 0) {
    return PICOKV_ERR_MAGIC;
  }
  if (out->version != PICOKV_VERSION) {
    return PICOKV_ERR_BADVER;
  }

  return 0;
}

void picokv_free_record(Record *r) {
  free(r->k);
  free(r->v);
}

static int picokv_read_rec_header(Record *out, FILE *fp) {
  out->op = read_le(1, fp);
  safe_fread(out->padding, 3, 1, fp);
  out->crc = read_le(sizeof out->crc, fp);
  out->k_sz = read_le(sizeof out->k_sz, fp);
  out->v_sz = read_le(sizeof out->v_sz, fp);

  if (out->op != PICOKV_OP_SET && out->op != PICOKV_OP_DEL) {
    return PICOKV_ERR_BADOP;
  }
  if (out->k_sz > MAX_K_SZ || out->v_sz > MAX_V_SZ) {
    return PICOKV_ERR_SIZE;
  }
  if (out->op == PICOKV_OP_DEL && out->v_sz != 0) {
    return PICOKV_ERR_SIZE;
  }

  return 0;
}

int picokv_read_record(Record *out, FILE *fp) {
  int first = fgetc(fp);

  if (first == EOF) {
    if (feof(fp))
      return PICOKV_ERR_EOF;

    return PICOKV_ERR_IO;
  }

  if (ungetc(first, fp) == EOF)
    return PICOKV_ERR_IO;

  int rch = picokv_read_rec_header(out, fp);
  if (rch != 0)
    return rch;

  out->k = malloc(out->k_sz + 1); // +1 for '\0'
  out->v = malloc(out->v_sz + 1);

  if (out->k == NULL || out->v == NULL) {
    free(out->k);
    free(out->v);
    out->k = NULL;
    out->v = NULL;
    return PICOKV_ERR_NOMEM;
  }

  safe_fread(out->k, out->k_sz, 1, fp);
  safe_fread(out->v, out->v_sz, 1, fp);
  out->k[out->k_sz] = '\0'; // convert to c-string
  out->v[out->v_sz] = '\0';

  uint32_t crc = crc32_start();
  crc = crc32_update(crc, (const uint8_t *)out->k, out->k_sz);
  crc = crc32_update(crc, (const uint8_t *)out->v, out->v_sz);
  crc = crc32_finish(crc);

  if (out->crc != crc) {
    picokv_free_record(out);
    return PICOKV_ERR_BADCRC;
  }

  return 0;
}
