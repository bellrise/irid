# irid-as build rules
# Copyright (c) 2023 bellrise

CXX ?= clang++

CFLAGS    += -Wall -Wextra -std=c++2a -I../include -I../libiridtools/include
LDFLAGS   +=
MAKEFLAGS += -j$(nproc)

PREFIX := /usr/local

SRC := $(wildcard src/*.cc)
DEP := $(wildcard src/*.h)
OBJ := $(patsubst src/%.cc,build/%.o,$(SRC))
LIB := ../libiridtools/build/libiridtools.a
BIN := irid-as
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

install:
	echo "  CP $(OUT) $(PREFIX)/bin/$(BIN)"
	cp $(OUT) $(PREFIX)/bin/$(BIN)

test:
	./runtests.sh

compile_flags.txt:
	echo $(CFLAGS) -xc++ | tr ' ' '\n' > compile_flags.txt

$(OUT): $(OBJ) $(LIB)
	@echo "  LD $@"
	@$(CXX) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIB)

build/%.o: src/%.cc $(DEP)
	@echo "  CXX $<"
	@$(CXX) -c -o $@ $(CFLAGS) $<

../libiridtools/build/libiridtools.a:
	@make -C ../libiridtools

.PHONY: compile_flags.txt
.SILENT: build clean install test
