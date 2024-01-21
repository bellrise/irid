/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

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

void options_set_defaults(struct options *opts)
{
    opts->output = NULL;
    opts->input = NULL;
    opts->set_origin = false;

    opts->w_unused_var = true;
    opts->f_block_tree = false;
    opts->f_comment_asm = false;
    opts->f_cmp_literal = true;
    opts->f_node_tree = false;
    opts->f_fold_constants = true;
}

static void warn_opt(struct options *opts, const char *name)
{
    bool set_mode = true;

    if (!strncmp(name, "no-", 3)) {
        name += 3;
        set_mode = false;
    }

    if (!strcmp(name, "unused-var"))
        opts->w_unused_var = set_mode;
}

static void func_opt(struct options *opts, const char *name)
{
    bool set_mode = true;

    if (!strncmp(name, "no-", 3)) {
        name += 3;
        set_mode = false;
    }

    if (!strcmp(name, "block-tree"))
        opts->f_block_tree = set_mode;
    if (!strcmp(name, "comment-asm"))
        opts->f_comment_asm = set_mode;
    if (!strcmp(name, "cmp-literal"))
        opts->f_cmp_literal = set_mode;
    if (!strcmp(name, "node-tree"))
        opts->f_node_tree = set_mode;
    if (!strcmp(name, "fold-constants"))
        opts->f_fold_constants = set_mode;
}

void options_parse(struct options *opts, int argc, char **argv)
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
        c = getopt_long(argc, argv, "f:hg:o:vW:", long_opts, &opt_index);
        if (c == -1)
            break;

        switch (c) {
        case 'f':
            func_opt(opts, optarg);
            break;
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
        case 'W':
            warn_opt(opts, optarg);
            break;
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
           "  -v, --version         show the version and exit\n"
           "  -Wunused-var          unused variables\n"
           "  -fblock-tree          show the compiled block tree\n"
           "  -ffold-constants      pre-calculate constant operations\n"
           "  -fcomment-asm         add comments to the generated assembly\n"
           "  -fcmp-literal         optimize comparing with literals\n"
           "  -fnode-tree           show the parsed node tree\n");

    fputc('\n', stdout);
}

void options_add_missing_opts(struct options *opts)
{
    static char output_filename[PATH_MAX];
    char buf[NAME_MAX];
    char *input_copy;
    char *p;

    if (!opts->input)
        die("no input file");

    if (opts->output)
        return;

    memset(buf, 0, NAME_MAX);

    input_copy = string_copy_z(input_copy);
    p = basename(input_copy);

    strcpy(buf, p);

    if ((p = strchr(buf, '.')))
        *p = 0;

    snprintf(output_filename, PATH_MAX, "%s/%s.i", dirname(input_copy), buf);
    if (!access(output_filename, F_OK))
        die("output file %s already exists", output_filename);

    opts->output = output_filename;
}
