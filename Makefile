CC ?= clang
CFLAGS = -Wall -Wextra -Wpedantic -Iinclude -Isrc -g
SAN_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer

SRC = main.c src/crc.c src/format.c src/repl.c src/picokv.c src/inmemmap.c src/pkverr.c
TARGET ?= dist/picokv
OBJ = $(SRC:.c=.o)

.PHONY: all clean run san

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p dist
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

san:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) $(SAN_FLAGS)"
