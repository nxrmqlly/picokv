// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026-present Ritam Das

#include "include/picokv.h"
#include "include/pkverr.h"
#include "src/repl.h"
#include <stdio.h>

int main(void) {
  PicoKV *pkv = picokv_new();
  if (pkv == NULL) {

    printf("Error: %s\n", picokv_strerror(PICOKV_ERR_NOMEM));
    return 1;
  }

  int rc = picokv_open(pkv, "test.picokv");
  if (rc != 0) {
    fprintf(stderr, "Error opening file: %s\n", picokv_strerror(rc));
    picokv_close(pkv);
    return 1;
  }

  int i = repl(pkv);
  picokv_close(pkv);

  return i;
}
