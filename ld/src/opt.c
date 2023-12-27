/* Irid linker
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static void short_usage();
static void usage();

void opt_set_defaults(struct options *opts)
{
    opts->output = "out.bin";
    opts->inputs.strings = NULL;
    opts->inputs.size = 0;
    opts->dump_symbols = false;
    opts->only_exported = false;
    opts->dump_header = false;
    opts->portable = false;
    opts->verbose = false;
}

void opt_parse(struct options *opts, int argc, char **argv)
{
    int opt_index;
    int c;

    static struct option long_opts[] = {{"help", no_argument, 0, 'h'},
                                        {"output", required_argument, 0, 'o'},
                                        {"portability", no_argument, 0, 'P'},
                                        {"version", no_argument, 0, 'v'},
                                        {"verbose", no_argument, 0, 'V'},
                                        {"symbols", no_argument, 0, 't'},
                                        {"exported", no_argument, 0, 'T'},
                                        {"headers", no_argument, 0, 'H'},
                                        {0, 0, 0, 0}};

    opt_index = 0;

    while (1) {
        c = getopt_long(argc, argv, "hHo:PtTvV", long_opts, &opt_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage();
            exit(0);
        case 'H':
            opts->dump_header = true;
            break;
        case 'o':
            opts->output = optarg;
            break;
        case 'P':
            opts->portable = true;
            break;
        case 't':
            opts->dump_symbols = true;
            break;
        case 'T':
            opts->only_exported = true;
            opts->dump_symbols = true;
            break;
        case 'v':
            printf("irid-ld %d.%d\n", LD_VER_MAJOR, LD_VER_MINOR);
            exit(0);
        case 'V':
            opts->verbose = true;
            break;
        }
    }

    while (optind < argc)
        strlist_append(&opts->inputs, argv[optind++]);
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
           "  -h, --help            show this usage page\n"
           "  -H, --headers         display all section headers\n"
           "  -o, --output OUTPUT   output to a file (default out.bin)\n"
           "  -P, --portability     use portable output\n"
           "  -t, --symbols         display all symbols\n"
           "  -T, --exported        display only exported symbols\n"
           "  -v, --version         show the version and exit\n"
           "  -V, --verbose         be verbose\n");
}
