all:
	@ make -j8 -C libiridtools -s
	@ make -j8 -C emul -s
	@ make -j8 -C as -s
	@ make -j8 -C ld -s

clean:
	@ make -C libiridtools -s clean
	@ make -C emul -s clean
	@ make -C as -s clean
	@ make -C ld -s clean
