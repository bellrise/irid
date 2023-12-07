/* Irid linker
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <getopt.h>

static void short_usage();
static void usage();

void opt_set_defaults(options& opts)
{
    opts.output = "out.bin";
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
            printf("irid-ld %d.%d\n", LD_VER_MAJOR, LD_VER_MINOR);
            exit(0);
        }
    }

    while (optind < argc)
        opts.inputs.push_back(argv[optind++]);
}

void short_usage()
{
    puts("usage: irid-ld [-h] [-o OUTPUT] [INPUT]...");
}

void usage()
{
    short_usage();
    puts("Link Irid objects into a executable format.");
    printf("Options:\n"
           "  -h, --help              show this usage page\n"
           "  -o, --output OUTPUT     output to a file (default out.bin)\n"
           "  -v, --version           show the version and exit\n");
}
