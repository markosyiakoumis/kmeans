# Define variables
CC = gcc
CFLAGS = -Wall -Werror -ljpeg -lm
LDFLAGS = -ljpeg -lm

ifeq ($(PG), 1)
	CFLAGS += -pg
	LDFLAGS += -pg
endif

ifeq ($(OPTIMIZE), O3)
    CFLAGS += -O3
else ifeq ($(OPTIMIZE), O2)
    CFLAGS += -O2
endif

TARGET = build/bin/kmeans
SRC = src/kmeans.c
OBJ = $(SRC:src/%.c=build/obj/%.o)

# Default target: build the program
all: $(TARGET)

# Ensure the necessary directories exist
build/obj/%.o: src/%.c | build/obj
	$(CC) $(CFLAGS) -c $< -o $@

# Build the executable
$(TARGET): $(OBJ) | build/bin
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Ensure that build/obj exists
build/obj:
	mkdir -p build/obj

# Ensure that build/bin exists
build/bin:
	mkdir -p build/bin

# Clean up the build
clean:
	rm -rf build

# Phony targets
.PHONY: all clean