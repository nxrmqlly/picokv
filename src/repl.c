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

int next_token(char **out, const char **read) {
  const char *p = *read;

  while (isspace((unsigned char)*p))
    p++;

  if (*p == '\0') {
    *out = NULL;
    *read = p;
    return 0; // no more tokens
  }

  // = is a special token!!
  if (*p == '=') {
    char *tok = malloc(2 * sizeof(char));
    if (tok == NULL)
      return PICOKV_ERR_NOMEM;

    tok[0] = '=';
    tok[1] = '\0';
    *out = tok;
    *read = p + 1;
    return 0;
  }

  size_t cap = PICOKV_BUFSZ;
  size_t len = 0;
  char *buf = malloc(cap);

  if (buf == NULL)
    return PICOKV_ERR_NOMEM;

  bool quoted = false;
  bool escaped = false;

  while (*p != '\0') {
    char c = *p;

    if (escaped) {
      p++;
      escaped = false;

    } else if (c == '\\') {
      p++;
      escaped = true;
      continue;

    } else if (c == '"') {
      p++;
      quoted = !quoted;
      continue;

    } else if (!quoted && (isspace((unsigned char)c) || c == '=')) {
      break;

    } else {
      p++;
    }

    if (len + 1 >= cap) {
      cap += PICOKV_BUFSZ;
      char *new_buf = realloc(buf, cap);

      if (new_buf == NULL) {
        free(buf);
        return PICOKV_ERR_NOMEM;
      }
      buf = new_buf;
    }
    buf[len++] = c;
  }

  if (quoted || escaped) {
    free(buf);
    return PICOKV_ERR_SYNTAX;
  }

  buf[len] = '\0';
  *out = buf;
  *read = p;
  return 0;
}

int lex_line(char ***out, char *line) {
  size_t cap = PICOKV_BUFSZ;
  size_t argc = 0;

  char **argv = malloc(cap * sizeof *argv);
  if (argv == NULL)
    return PICOKV_ERR_NOMEM;

  const char *read = line;

  while (true) {
    char *tok;
    int rc = next_token(&tok, &read);
    if (rc != 0) {
      for (size_t i = 0; i < argc; i++)
        free(argv[i]);
      free(argv);

      return rc;
    }

    if (tok == NULL)
      break;

    if (argc + 1 >= cap) {
      cap += PICOKV_BUFSZ;
      char **new_argv = realloc(argv, cap * sizeof *argv);

      if (new_argv == NULL) {
        free(tok);

        for (size_t i = 0; i < argc; i++)
          free(argv[i]);
        free(argv);

        return PICOKV_ERR_NOMEM;
      }
      argv = new_argv;
    }
    argv[argc++] = tok;
  }
  argv[argc] = NULL;
  *out = argv;

  return 0;
}

void free_argv(char **argv) {
  if (argv == NULL)
    return;

  for (size_t i = 0; argv[i] != NULL; i++)
    free(argv[i]);
  free(argv);
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
    free(line); // no longer needed after parse
    if (rc != 0) {
      return rc;
    }

    int argc = 0;
    while (argv[argc] != NULL)
      argc++;

    if (argc == 0) {
      free_argv(argv);
      continue;
    }

    enum Command command = parse_command(argv[0]);

    rc = 0;
    switch (command) {
    case CMD_QUIT:
      free_argv(argv);
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

    free_argv(argv);
    // free(line); // idk if we should be freeing line here again
    printf(PICOKV_PROMPT);
  }
  return 0;
}
