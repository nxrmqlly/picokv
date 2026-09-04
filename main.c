#include "src/repl.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  FILE *file_ptr;
  file_ptr = fopen("test.picokv", "wb");

  if (file_ptr == NULL) {
    perror("Failed to open test.picokv");
    exit(1);
  }

  fclose(file_ptr);

  int i = repl();
  return i;
}
