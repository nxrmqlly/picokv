// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026-present Ritam Das

#include "../include/picokv.h"
#include "../include/pkverr.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PICOKV_PROMPT ("picokv> ")
#define MAX_ARGS (3)
#define ARG_DELIM (" \t")

enum Command { CMD_UNKNOWN, CMD_HELP, CMD_QUIT, CMD_SET, CMD_DEL, CMD_GET };

int strieq(const char *a, const char *b) {
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
      return 0;
    }
    a++;
    b++;
  }
  return *a == *b;
}

enum Command parse_command(const char *cmd) {
  if (strieq(cmd, "quit") || strieq(cmd, "exit"))
    return CMD_QUIT;

  if (strieq(cmd, "help"))
    return CMD_HELP;

  if (strieq(cmd, "set"))
    return CMD_SET;

  if (strieq(cmd, "get"))
    return CMD_GET;

  if (strieq(cmd, "del"))
    return CMD_DEL;

  return CMD_UNKNOWN;
}

int repl(PicoKV *pkv) {
  char input[256];
  printf(PICOKV_PROMPT);

  while (fgets(input, sizeof input, stdin)) {
    input[strcspn(input, "\n")] = '\0';

    char *argv[MAX_ARGS];
    int argc = 0;

    char *token = strtok(input, ARG_DELIM);

    while (token != NULL && argc < MAX_ARGS) {
      argv[argc++] = token;
      token = strtok(NULL, ARG_DELIM);
    }

    if (argc == 0)
      continue;

    enum Command command = parse_command(argv[0]);

    int rc = 0;

    switch (command) {
    case CMD_QUIT:
      return 0;

    case CMD_HELP: {
      printf("Commands: help, quit, set, get, del\n");
      break;
    }

    case CMD_SET: {
      if (argc != 3) {
        printf("usage: set <key> <value>\n");
        break;
      }

      rc = picokv_set(pkv, argv[1], argv[2]);
      if (rc != 0) {
        printf("Error: %s\n", picokv_strerror(rc));
        break;
      }

      printf("OK SET %s = %s\n", argv[1], argv[2]);
      break;
    }

    case CMD_GET: {
      if (argc != 2) {
        printf("Usage: get <key>\n");
        break;
      }

      size_t sz = 0;
      rc = picokv_get_size(pkv, &sz, argv[1]);
      if (rc != 0) {
        printf("Error: %s\n", picokv_strerror(rc));
        break;
      }

      // +1 for '\0' terminated.
      char *out_str = malloc(sz + 1); // deliberately put it on the heap
      if (out_str == NULL) {
        printf("Error: %s\n", picokv_strerror(PICOKV_ERR_NOMEM));
        break;
      }

      rc = picokv_get(pkv, out_str, argv[1], sz + 1);
      if (rc != 0) {
        free(out_str);
        printf("Error: %s\n", picokv_strerror(rc));
        break;
      }

      printf("%s\n", out_str);
      free(out_str);
      break;
    }

    case CMD_DEL: {
      if (argc != 2) {
        printf("Usage: del <key>\n");
        break;
      }

      rc = picokv_del(pkv, argv[1]);
      if (rc != 0) {
        printf("Error: %s\n", picokv_strerror(rc));
        break;
      }

      printf("OK DEL %s\n", argv[1]);
      break;
    }

    case CMD_UNKNOWN:
      printf("Unknown Command: %s\n", argv[0]);
      break;
    }

    printf(PICOKV_PROMPT);
  }
  return 0;
}
