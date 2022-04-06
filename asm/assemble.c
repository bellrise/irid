/* Irid assemble routine.
   Copyright (C) 2021-2022 bellrise */

#include "asm.h"


int irid_assemble(char *path, char *dest, int _Unused opts)
{
	struct asm_ctx context = {0};

	context.in  = fopen(path, "r");
	context.out = fopen(dest, "wb");

	if (!context.in || !context.out)
		die("failed to open file %s", !context.in ? path : dest);

	fclose(context.in);
	fclose(context.out);
	return 1;
}
