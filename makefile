all:
	@ make -j8 -C emul -s
	@ make -j8 -C as -s

clean:
	@ make -C emul -s clean
	@ make -C as -s clean
