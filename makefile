# Copyright (C) 2021 bellrise

# A single makefile to build the whole project, for convenience. Some targets
# may contain their custom makefile configurations. Call `make` to build the
# project, generating binaries in the `build` directory.

CC 	   := clang
CFLAGS := -Wall -Wextra -Iinc -fsanitize=address -DDEBUG


all: _setup emul ld asm

emul: _setup
	$(CC) -o build/irid-emul $(CFLAGS) $(shell find emul -name '*.c')

ld: _setup

asm: _setup

clean:
	rm -rf build


# Private targets

_setup:
	mkdir -p build


.PHONY  : all emul ld asm
.SILENT : emul ld asm _setup
