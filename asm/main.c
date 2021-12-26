/* Irid assembler.
   Copyright (C) 2021 bellrise */

#include "asm.h"


int main(int argc, char **argv)
{
    struct runtime rt;

    if (argparse(&rt, --argc, ++argv) || !rt.nsources)
        die("no input files");

    irid_assemble(rt.sources[0], "out.bin", 0);

    free(rt.sources);
}
