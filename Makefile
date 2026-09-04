CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -Iinclude -Isrc

SRC = main.c src/crc.c src/format.c src/repl.c src/picokv.c src/inmemmap.c src/pkverr.c
TARGET = dist/picokv
OBJ = $(SRC:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p dist
	$(CC) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
