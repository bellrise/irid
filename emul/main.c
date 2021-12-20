/* Irid emulator.
   Copyright (C) 2021 bellrise */

#include <irid/arch.h>
#include <stdio.h>
#include "emul.h"


int main(int argc, char **argv)
{
    struct runtime rt = {0};

    if (argparse(&rt, --argc, ++argv))
        die("could not parse arguments");

    irid_emulate(rt.binfile, 0, 0);
}
