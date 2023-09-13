# irid-emul build rules
# Copyright (c) 2023 mini-rose

CXX ?= clang++

CFLAGS    += -Wall -Wextra
LDFLAGS   += -lxcb
MAKEFLAGS += -j$(nproc)

PREFIX := /usr/local

SRC := $(wildcard src/*.cc)
OBJ := $(patsubst src/%.cc,build/%.o,$(SRC))
BIN := irid-emul
OUT := build/$(BIN)
BT  ?= debug

ifeq ($(BT), debug)
	CFLAGS += -O0 -ggdb -DDEBUG=1 -fsanitize=address
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

install:
	echo "  CP $(OUT) $(PREFIX)/bin/$(BIN)"
	cp $(OUT) $(PREFIX)/bin/$(BIN)

compile_flags.txt:
	echo $(CFLAGS) -xc++ | tr ' ' '\n' > compile_flags.txt

$(OUT): $(OBJ)
	@echo "  LD $@"
	@$(CXX) -o $@ $(CFLAGS) $(LDFLAGS) $^

build/%.o: src/%.cc
	@echo "  CXX $<"
	@$(CXX) -c -o $@ $(CFLAGS) $<


.PHONY: compile_flags.txt
.SILENT: build clean install
