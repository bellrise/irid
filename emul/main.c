/* Irid emulator.
   Copyright (C) 2021 bellrise */

#include <irid/arch.h>
#include <stdio.h>
#include "emul.h"


#define DEFAULT_WIDTH       640
#define DEFAULT_HEIGHT      400


int main(int argc, char **argv)
{
    struct runtime rt = {0};

    if (argparse(&rt, --argc, ++argv))
        die("could not parse arguments");

    rt.win = eg_create_window(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    if (!rt.win)
        die("could not create window: %s", eg_strerror(eg_get_last_error()));

    irid_emulate(rt.binfile, 0, 0);

    eg_close_window(rt.win);
}
