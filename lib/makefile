all: setup build/sys

setup:
	@mkdir -p build

build/%.o: %.i
	irid-as -o $@ $<

build/sys: build/entry.o build/io.o
	irid-ld -o build/sys $(shell find build/ -name '*.o')

clean:
	rm -rf build
