/* Irid assembler.
   Copyright (C) 2021-2022 bellrise */

#include "asm.h"

struct runtime *GLOB_RUNTIME;


int main(int argc, char **argv)
{
	struct runtime rt = {0};
	GLOB_RUNTIME = &rt;

	atexit(free_resources);

	if (argparse(&rt, --argc, ++argv) || !rt.nsources)
		die("no input files");

	irid_assemble(rt.sources[0], "out.bin", 0);

	exit(0);
}
