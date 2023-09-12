/* Irid assembler
   Copyright (c) 2023 bellrise */

#include "as.h"

#include <getopt.h>

static void short_usage();
static void usage();

void opt_set_defaults(options& opts)
{
    opts.output = "out.bin";
    opts.input = "-";
}

void opt_parse(options& opts, int argc, char **argv)
{
    int opt_index;
    int c;

    static struct option long_opts[] = {{"help", no_argument, 0, 'h'},
                                        {"output", required_argument, 0, 'o'},
                                        {"version", no_argument, 0, 'v'},
                                        {0, 0, 0, 0}};

    opt_index = 0;

    while (1) {
        c = getopt_long(argc, argv, "ho:v", long_opts, &opt_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage();
            exit(0);
        case 'o':
            opts.output = optarg;
            break;
        case 'v':
            printf("irid-as %d.%d\n", AS_VER_MAJOR, AS_VER_MINOR);
            exit(0);
        }
    }

    if (optind < argc)
        opts.input = argv[optind];
}

static void short_usage()
{
    puts("usage: irid-as [-h] [-o OUTPUT] [INPUT]");
}

static void usage()
{
    short_usage();
    puts("\nAssemble Irid native assembly code into a binary format.");
    puts("Reads from stdin by default.\n");
    printf("Options:\n"
           "  -h, --help              show this usage page\n"
           "  -o, --output OUTPUT     output to a file (default out.bin)\n"
           "  -v, --version           show the version and exit\n");
}
