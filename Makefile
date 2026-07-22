CC := gcc

CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -Iinclude

TARGET := build/edgesentinel

SOURCES := $(wildcard src/*.c)

OBJECTS := $(patsubst src/%.c,build/%.o,$(SOURCES))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build
