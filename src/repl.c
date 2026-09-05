// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026-present Ritam Das

#include "../include/picokv.h"
#include "../include/pkverr.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PICOKV_PROMPT ("picokv> ")
#define ARG_DELIM (" \t\r\n\a")
#define PICOKV_BUFSZ (64)

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

int scan_line(char **dest) {
  size_t bufsz = PICOKV_BUFSZ;
  size_t pos = 0;

  char *buffer = malloc(sizeof(char) * bufsz);
  if (buffer == NULL)
    return PICOKV_ERR_NOMEM;

  while (true) {
    int c = getchar();

    if (c == EOF || c == '\n')
      break;

    if (pos + 1 >= bufsz) {
      bufsz += PICOKV_BUFSZ;

      char *new_buffer = realloc(buffer, bufsz);
      if (new_buffer == NULL) {
        free(buffer);
        return PICOKV_ERR_NOMEM;
      }
      buffer = new_buffer;
    }
    buffer[pos++] = (char)c;
  }

  buffer[pos] = '\0';
  *dest = buffer;

  return 0;
}

int lex_line(char ***out, char *line) {
  size_t cap = PICOKV_BUFSZ;
  size_t argc = 0;

  char **argv = malloc(cap * sizeof *argv);
  if (argv == NULL)
    return PICOKV_ERR_NOMEM;

  char *read = line;
  char *write = line;

  while (*read != '\0') {
    while (isspace((unsigned char)*read))
      read++;

    if (*read == '\0')
      break;

    if (argc + 1 >= cap) {
      cap += PICOKV_BUFSZ;

      char **new_argv = realloc(argv, cap * sizeof *argv);
      if (new_argv == NULL) {
        free(argv);
        return PICOKV_ERR_NOMEM;
      }

      argv = new_argv;
    }

    // '=' is a diff indipendent token
    if (*read == '=') {
      argv[argc++] = write;
      *write++ = *read++;
      *write++ = '\0';
      continue;
    }

    argv[argc++] = write;

    bool quoted = false;
    bool escaped = false;

    while (*read != '\0') {
      char c = *read++;

      if (escaped) {
        *write++ = c;
        escaped = false;
        continue;
      }

      if (c == '\\') {
        escaped = true;
        continue;
      }

      if (c == '"') {
        quoted = !quoted;
        continue;
      }

      if (!quoted && (isspace((unsigned char)c) || c == '=')) {
        break;
      }

      *write++ = c;
    }

    *write++ = '\0';

    if (quoted || escaped) {
      free(argv);
      return PICOKV_ERR_SYNTAX;
    }

    // if the delimiter we just consumed was '=',
    // put it back so the next iteration lexes it
    if (read > line && read[-1] == '=') {
      read--;
    }
  }

  argv[argc] = NULL;
  *out = argv;

  return 0;
}

int repl(PicoKV *pkv) {
  printf(PICOKV_PROMPT);
  while (true) {
    char *line;
    char **argv;

    int rc = scan_line(&line);
    if (rc != 0)
      return rc;

    rc = lex_line(&argv, line);
    if (rc != 0) {
      free(line);
      return rc;
    }

    int argc = 0;
    while (argv[argc] != NULL)
      argc++;
    if (argc == 0) {
      free(argv);
      free(line);
      continue;
    }

    enum Command command = parse_command(argv[0]);

    rc = 0;
    switch (command) {
    case CMD_QUIT:
      free(argv);
      free(line);
      return 0;

    case CMD_HELP: {
      printf("Commands: help, quit, set, get, del\n");
      break;
    }

    case CMD_SET: {
      if (argc != 4 || strcmp(argv[2], "=") != 0) {
        printf("usage: set <key> = <value>\n");
        break;
      }

      rc = picokv_set(pkv, argv[1], argv[3]);
      if (rc != 0) {
        printf("Error: %s\n", picokv_strerror(rc));
        break;
      }

      printf("OK SET %s %s %s\n", argv[1], argv[2], argv[3]);
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
