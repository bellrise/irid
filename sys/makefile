all: setup build/sys

setup:
	@mkdir -p build

build/%.o: %.i
	irid-as -o $@ $<

entry.i: entry.lf
	irid-lc -fcomment-asm -g 0 entry.lf -o entry.i

test.i: ../lc/test.lf
	irid-lc -fcomment-asm ../lc/test.lf -o test.i

build/sys: build/test.o build/io.o build/sh.o build/string.o build/mem.o build/entry.o build/cpucall.o
	irid-ld -o build/sys $(shell find build/ -name '*.o')

run: all
	@irid-emul build/sys

clean:
	rm -f test.i
	rm -rf build
