#include "../include/picokv.h"
#include "./format.h"
#include "inmemmap.h"
#include <stdio.h>

struct PicoKV {
  FILE *fp;
  Header h;
};

int picokv_open(PicoKV *pkv, const char *path) {
  pkv->fp = fopen(path, "a+b");

  if (pkv->fp == NULL) {
    perror("Failed to open file");
    return PICOKV_ERR_IO;
  }

  if (fseek(pkv->fp, 0, SEEK_END) != 0) {
    return PICOKV_ERR_IO;
  }

  long size = ftell(pkv->fp);

  if (size < 0) {
    picokv_close(pkv);
    return PICOKV_ERR_IO;
  }

  if (size == 0) {
    picokv_write_header(pkv->fp);
  } else {
    if (fseek(pkv->fp, 0, SEEK_SET) != 0)
      return PICOKV_ERR_IO;
    int rh = picokv_read_header(&pkv->h, pkv->fp);

    if (rh != 0) {
      picokv_close(pkv);
      return rh;
    }
  }

  while (1) {
    Record r;
    int rc = picokv_read_record(&r, pkv->fp);

    if (rc == PICOKV_EOF)
      break;

    if (rc != 0)
      return rc;

    picokv_free_record(&r);
  }
  return 0;
}

void picokv_close(PicoKV *pkv) {
  fclose(pkv->fp);
  pkv->fp = NULL;
}

int picokv_set(PicoKV *pkv, const char *key, const char *val) {}
int picokv_get(PicoKV *pkv, const char *key, char *val, size_t sz) {}
int picokv_del(PicoKV *pkv, const char *key) {}
