/* Irid assemble routine.
   Copyright (C) 2021 bellrise */

#include "asm.h"


int irid_assemble(char *path, char *dest, int _Unused opts)
{
    FILE *in, *out;

    in  = fopen(path, "r");
    out = fopen(dest, "wb");

    if (!in || !out)
        die("failed to open file %s", !in ? path : dest);

    fclose(in);
    fclose(out);
    return 1;
}
