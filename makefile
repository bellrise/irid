# Copyright (C) 2021 bellrise

# A single makefile to build the whole project, for convenience. Some targets
# may contain their custom makefile configurations. Call `make` to build the
# project, generating binaries in the `build` directory.

CC 	   := clang
CFLAGS := -Wall -Wextra -Iinc -lX11 -O0 -DDEBUG -fsanitize=address
COMMON := common/info.c


all: _setup emul ld asm

emul: _setup
	$(CC) -o build/irid-emul $(CFLAGS) $(shell find emul -name '*.c') $(COMMON)

ld: _setup

asm: _setup
	$(CC) -o build/irid-asm $(CFLAGS) $(shell find asm -name '*.c') $(COMMON)

clean:
	rm -rf build

install:
	sh ./scripts/install

# Private targets

_setup:
	mkdir -p build


.PHONY  : all emul ld asm
.SILENT : emul ld asm clean install install-global _setup
