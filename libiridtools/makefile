# libiridtools build rules
# Copyright (c) 2023-2024 bellrise

CXX ?= clang++

CFLAGS    += -Wall -Wextra -std=c++2a -I./include -I../include
LDFLAGS   +=
MAKEFLAGS += -j$(nproc)

PREFIX := /usr/local

SRC := $(wildcard src/*.cc)
DEP := $(wildcard src/*.h)
OBJ := $(patsubst src/%.cc,build/%.o,$(SRC))
BIN := libiridtools.a
OUT := build/$(BIN)
BT  ?= debug

ifeq ($(BT), debug)
	CFLAGS += -O0 -ggdb -DDEBUG=1
	LDFLAGS +=
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

test:
	./runtests.sh

compile_flags.txt:
	echo $(CFLAGS) -xc++ | tr ' ' '\n' > compile_flags.txt

$(OUT): $(OBJ)
	@echo "  AR $@"
	@ar r $(OUT) $(OBJ) 2>/dev/null

build/%.o: src/%.cc $(DEP)
	@echo "  CXX $<"
	@$(CXX) -c -o $@ $(CFLAGS) $(LDFLAGS) $<


.PHONY: compile_flags.txt
.SILENT: build clean install test
