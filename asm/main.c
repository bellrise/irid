/* Irid assembler.
   Copyright (C) 2021 bellrise */

#include "asm.h"

static struct runtime rt = {0};
void free_resources();


int main(int argc, char **argv)
{
    atexit(free_resources);

    if (argparse(&rt, --argc, ++argv) || !rt.nsources)
        die("no input files");

    irid_assemble(rt.sources[0], "out.bin", 0);
}

void free_resources()
{
    info("freeing resources");
    free(rt.sources);
}
