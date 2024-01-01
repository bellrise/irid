/* Leaf compiler
   Copyright (c) 2023 bellrise */

#include "lc.h"

#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void short_usage();
static void usage();

void opt_set_defaults(struct options *opts)
{
    opts->output = NULL;
    opts->input = NULL;
    opts->set_origin = false;
}

void opt_parse(struct options *opts, int argc, char **argv)
{
    int opt_index;
    int c;

    static struct option long_opts[] = {{"help", no_argument, 0, 'h'},
                                        {"output", required_argument, 0, 'o'},
                                        {"origin", required_argument, 0, 'g'},
                                        {"version", no_argument, 0, 'v'},
                                        {0, 0, 0, 0}};

    opt_index = 0;

    while (1) {
        c = getopt_long(argc, argv, "hg:o:v", long_opts, &opt_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage();
            exit(0);
        case 'g':
            opts->origin = strtol(optarg, NULL, 0);
            opts->set_origin = true;
            break;
        case 'o':
            opts->output = optarg;
            break;
        case 'v':
            printf("irid-lc %d.%d\n", LC_VER_MAJ, LC_VER_MIN);
            exit(0);
        }
    }

    if (optind < argc)
        opts->input = argv[optind];
}

void short_usage()
{
    puts("usage: irid-lc [-h] [-o OUTPUT] INPUT");
}

void usage()
{
    short_usage();
    puts("Compiles Leaf code into Irid assembly.\n");
    printf("Options:\n"
           "  -h, --help            show this usage page\n"
           "  -g, --origin ORIGIN   set the origin of the code section\n"
           "  -o, --output OUTPUT   select output file\n"
           "  -v, --version         show the version and exit\n");
}

void opt_add_missing_opts(struct options *opts)
{
    static char output_filename[PATH_MAX];
    char buf[NAME_MAX];
    char *p;

    if (!opts->input)
        die("no input file");

    if (opts->output)
        return;

    memset(buf, 0, NAME_MAX);
    p = basename(opts->input);
    strcpy(buf, p);

    if ((p = strchr(buf, '.')))
        *p = 0;

    snprintf(output_filename, PATH_MAX, "%s/%s.i", dirname(opts->input), buf);
    if (!access(output_filename, F_OK))
        die("output file %s already exists", output_filename);

    opts->output = output_filename;
}
