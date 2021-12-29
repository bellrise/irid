/* Irid emulator.
   Copyright (C) 2021 bellrise */

#include <irid/arch.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "emul.h"

#define DEFAULT_WIDTH       640
#define DEFAULT_HEIGHT      400


static struct runtime rt = {0};
void free_resources();


int main(int argc, char **argv)
{
    struct runtime rt = {0};
    int err;

    atexit(free_resources);

    if (argparse(&rt, --argc, ++argv))
        die("could not parse arguments");

    if (!rt.is_headless) {
        rt.win = eg_create_window(DEFAULT_WIDTH, DEFAULT_HEIGHT);
        if (!rt.win)
            die("could not create window: %s", strerror(errno));
    }

    if ((err = irid_emulate(rt.binfile, 0, 0, rt.win)))
        die("cpufault 0x%02hhx", err);
}

void free_resources()
{
    info("freeing resources");

    if (!rt.is_headless)
        eg_close_window(rt.win);
}
