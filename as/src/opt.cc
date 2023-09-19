/* Irid assembler
   Copyright (c) 2023 bellrise */

#include "as.h"

#include <getopt.h>

static void short_usage();
static void usage();
static void parse_warn_flag(options&, std::string option_name);

void opt_set_defaults(options& opts)
{
    opts.output = "out.iof";
    opts.input = "-";

    opts.warn_origin_overlap = true;
    opts.raw_binary = false;
}

void opt_set_warnings_for_as(assembler& as, options& opts)
{
    as.set_warning(warning_type::OVERLAPING_ORG, opts.warn_origin_overlap);
}

void opt_parse(options& opts, int argc, char **argv)
{
    int opt_index;
    int c;

    static struct option long_opts[] = {{"help", no_argument, 0, 'h'},
                                        {"output", required_argument, 0, 'o'},
                                        {"raw", no_argument, 0, 'r'},
                                        {"version", no_argument, 0, 'v'},
                                        {0, 0, 0, 0}};

    opt_index = 0;

    while (1) {
        c = getopt_long(argc, argv, "ho:rvW:", long_opts, &opt_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage();
            exit(0);
        case 'o':
            opts.output = optarg;
            break;
        case 'r':
            opts.raw_binary = true;
            break;
        case 'v':
            printf("irid-as %d.%d\n", AS_VER_MAJOR, AS_VER_MINOR);
            exit(0);
        case 'W':
            parse_warn_flag(opts, optarg);
            break;
        }
    }

    if (optind < argc)
        opts.input = argv[optind];
}

void short_usage()
{
    puts("usage: irid-as [-h] [-o OUTPUT] [INPUT]");
}

void usage()
{
    short_usage();
    puts("\nAssemble Irid native assembly code into a binary format.");
    puts("Reads from stdin by default.\n");
    printf("Options:\n"
           "  -h, --help              show this usage page\n"
           "  -o, --output OUTPUT     output to a file (default out.bin)\n"
           "  -v, --version           show the version and exit\n"
           "  -Worigin-overlap        .org may cause code overlap\n");
}

void parse_warn_flag(options& opts, std::string option_name)
{
    bool enable_warning = true;

    if (option_name.starts_with("no-")) {
        enable_warning = false;
        option_name = option_name.substr(3);
    }

    if (option_name == "origin-overlap")
        opts.warn_origin_overlap = enable_warning;
}
