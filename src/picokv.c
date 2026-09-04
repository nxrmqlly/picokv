#include "../include/picokv.h"
#include "./format.h"
#include "inmemmap.h"
#include "pkverr.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PicoKV {
  FILE *fp;
  Header h;
  HashMap *map;
};

int picokv_open(PicoKV *pkv, const char *path) {
  pkv->fp = fopen(path, "a+b");
  if (pkv->fp == NULL) {
    perror("Failed to open file");
    return PICOKV_ERR_IO;
  }

  pkv->map = create_map();

  if (pkv->map == NULL) {
    picokv_close(pkv);
    return PICOKV_ERR_NOMEM;
  }

  if (fseek(pkv->fp, 0, SEEK_END) != 0) {
    picokv_close(pkv);
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
    if (fseek(pkv->fp, 0, SEEK_SET) != 0) {
      picokv_close(pkv);
      return PICOKV_ERR_IO;
    }

    int rh = picokv_read_header(&pkv->h, pkv->fp);

    if (rh != 0) {
      picokv_close(pkv);
      return rh;
    }
  }

  while (true) {
    Record r;
    int rc = picokv_read_record(&r, pkv->fp);

    if (rc == PICOKV_ERR_EOF)
      break;
    if (rc != 0)
      return rc;

    int mr = 0;
    switch (r.op) {
    case PICOKV_OP_SET:
      mr = map_put(pkv->map, r.k, r.v);
      break;

    case PICOKV_OP_DEL:
      mr = map_del(pkv->map, r.k);
      break;

    default:
      // just in case; as this should be automatically handled by
      // picokv_read_record
      mr = PICOKV_ERR_BADOP;
    }

    // NOTFOUND from map_del is probably fiiiine, doesn't really break the rest
    // of the data during the replay.
    // FIXME: Perhaps we can cleanup these somewhere?
    if (mr != PICOKV_ERR_NOTFOUND && mr != 0) {
      picokv_free_record(&r);
      picokv_close(pkv);
      return mr;
    }
    picokv_free_record(&r);
  }
  return 0;
}

void picokv_close(PicoKV *pkv) {
  // NULL checking makes calling picokv_close during error path safe
  if (pkv->fp != NULL) {
    fclose(pkv->fp);
    pkv->fp = NULL;
  }

  if (pkv->map != NULL) {
    free_map(pkv->map);
    pkv->map = NULL;
  }
}

int picokv_set(PicoKV *pkv, const char *key, const char *val) {
  int rc = picokv_write_record(PICOKV_OP_SET, key, val, pkv->fp);
  if (rc != 0)
    return rc;

  return map_put(pkv->map, key, val);
}

static const char *picokv_find(PicoKV *pkv, const char *key) {
  return map_get(pkv->map, key);
}

int picokv_get_size(PicoKV *pkv, size_t *out_sz, const char *key) {
  const char *found = map_get(pkv->map, key);
  if (found == NULL) {
    return PICOKV_ERR_NOTFOUND;
  }
  *out_sz = strlen(found);
  return 0;
}

int picokv_get(PicoKV *pkv, char *out, const char *key, size_t sz) {
  const char *found = map_get(pkv->map, key);
  if (found == NULL) {
    return PICOKV_ERR_NOTFOUND;
  }

  size_t len = strlen(found);
  if (len + 1 > sz) { // +1 for '\0'
    return PICOKV_ERR_SIZE;
  }

  memcpy(out, found, len + 1);
  return 0;
}

int picokv_del(PicoKV *pkv, const char *key) {
  if (map_get(pkv->map, key) == NULL)
    return PICOKV_ERR_NOTFOUND;

  if (fseek(pkv->fp, 0, SEEK_END) != 0)
    return PICOKV_ERR_IO;

  int rc = picokv_write_record(PICOKV_OP_DEL, key, NULL, pkv->fp);
  if (rc != 0)
    return rc;

  return map_del(pkv->map, key);
}
