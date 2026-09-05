// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026-present Ritam Das

#include "include/picokv.h"
#include "include/picokv_version.h"
#include "include/pkverr.h"
#include "src/repl.h"
#include <stdio.h>
#include <string.h>

#define PICOKV_DEFAULT_FILE ("data.picokv")

int main(int argc, char **argv) {
  const char *filename = PICOKV_DEFAULT_FILE;

  if (argc > 2) {
    fprintf(stderr, "usage: %s [filename]\n", argv[0]);
    return 1;
  }

  if (argc == 2) {
    filename = argv[1];
  }

  PicoKV *pkv = picokv_new();
  if (pkv == NULL) {
    printf("Error: %s\n", picokv_strerror(PICOKV_ERR_NOMEM));
    return 1;
  }

  int rc = picokv_open(pkv, filename);
  if (rc != 0) {
    fprintf(stderr, "Error opening file: %s\n", picokv_strerror(rc));
    picokv_close(pkv);
    return 1;
  }

  printf("PicoKV %s\n", PICOKV_SRC_VERSION);
  printf("File: %s %s\n", filename,
         (strcmp(filename, PICOKV_DEFAULT_FILE) == 0 ? "[default file]" : ""));
  printf("Type 'help' for help.\n\n");

  int i = repl(pkv);
  picokv_close(pkv);

  return i;
}
