#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define PICOKV_PROMPT ("picokv> ")

enum Command {
  CMD_EMPTY,
  CMD_UNKNOWN,
  CMD_HELP,
  CMD_QUIT,
  CMD_SET,
  CMD_DEL,
  CMD_GET
};

int strieq(char *a, char *b) {
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
      return 0;
    }
    a++;
    b++;
  }
  return *a == *b;
}

int repl(void) {
  char input[256];
  printf(PICOKV_PROMPT);

  while (fgets(input, sizeof input, stdin)) {
    input[strcspn(input, "\n")] = '\0';
    enum Command command;

    if (input[0] == '\0') {
      command = CMD_EMPTY;
    } else if (strieq(input, "quit") || strieq(input, "exit")) {
      command = CMD_QUIT;
    } else if (strieq(input, "help")) {
      command = CMD_HELP;
    } else if (strieq(input, "set")) {
      command = CMD_SET;
    } else if (strieq(input, "get")) {
      command = CMD_GET;
    } else if (strieq(input, "del")) {
      command = CMD_DEL;
    } else {
      command = CMD_UNKNOWN;
    }

    switch (command) {
    case CMD_EMPTY:
      break;
    case CMD_UNKNOWN:;
      printf("Unknown Command: %s\n", input);
      break;
    case CMD_QUIT:
      return 0;
    case CMD_HELP:
      printf("Commands: help, quit\n");
      break;
    case CMD_SET:;
    case CMD_GET:;
    case CMD_DEL:;
    }

    printf(PICOKV_PROMPT);
  }
  return 0;
}
