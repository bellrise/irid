/* Irid assemble routine.
   Copyright (C) 2021 bellrise */

#include "asm.h"


int irid_assemble(char *path, char *dest, int _Unused opts)
{
    FILE *in, *out;

    info("assembling %s -> %s", path, dest);

    /* Open both files. */

    if (!(in = fopen(path, "r")))
        die("failed to open %s", path);
    if (!(out = fopen(dest, "wb"))) {
        fclose(in);
        die("failed to open %s", dest);
    }

    // TODO: assemble

    fclose(in);
    fclose(out);
    return 1;
}
