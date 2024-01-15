# irid-ld build rules
# Copyright (c) 2023-2024 bellrise

CC ?= clang

CFLAGS    += -Wall -Wextra -I../include
LDFLAGS   +=
MAKEFLAGS += -j$(nproc)

PREFIX := /usr/local

SRC := $(wildcard src/*.c)
DEP := $(wildcard src/*.h)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))
BIN := irid-ld
LIB :=
OUT := build/$(BIN)
BT  ?= debug

ifeq ($(BT), debug)
	CFLAGS += -O0 -ggdb -DDEBUG=1
	LDFLAGS += -fsanitize=address
else ifeq ($(BT), release)
	CFLAGS += -O3
else
	$(error unknown build type: $(BT))
endif


all: build $(OUT)


build:
	mkdir -p build

clean:
	echo "  RM build"
	rm -rf build

compile_flags.txt:
	echo $(CFLAGS) -xc | tr ' ' '\n' > compile_flags.txt

$(OUT): $(OBJ)
	@echo "  LD $@"
	@$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIB)

build/%.o: src/%.c $(DEP)
	@echo "  CC $<"
	@$(CC) -c -o $@ $(CFLAGS) $(LDFLAGS) $<


.PHONY: compile_flags.txt
.SILENT: build clean install
