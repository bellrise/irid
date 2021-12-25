/* Argument parser.
   Copyright (C) 2021 bellrise */

#include <string.h>
#include "emul.h"

static void parse_opt(struct runtime *rt, int *i, int argc, char **argv);
static void usage();
static void version();


int argparse(struct runtime *rt, int argc, char **argv)
{
    rt->argc = argc;
    rt->argv = argv;

    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-')
            parse_opt(rt, &i, argc, argv);
        else if (i < argc)
            rt->binfile = argv[i];
    }

    if (!rt->binfile)
        die("no binary file");

    return 0;
}

static void parse_opt(struct runtime *_Unused rt, int *i, int _Unused argc,
        char **argv)
{
    char *long_arg;
    size_t arglen;
    char arg;

    if (!argv[*i][1])
        return;

    if (argv[*i][1] == '-')
        goto long_opt;

    /* If only one hyphen is present, each letter represents a unique flag. */

    arglen = strlen(argv[*i]);
    for (size_t j = 1; j < arglen; j++) {
        arg = argv[*i][j];
        switch (arg) {
            case '?':
            case 'h':
                usage();
                break;
            case 'v':
                printf("%d\n", IRID_EMUL_VERID);
                exit(1);
            default:
                warn("unknown flag '%s'", argv[*i]);
        }
    }
    return;

long_opt:
    long_arg = argv[*i] + 2;
    if (!strcmp(long_arg, "version"))
        version();
    else if (!strcmp(long_arg, "help"))
        usage();
    else
        warn("unknown option '%s'", long_arg);
}

static void usage()
{
    printf(
        "usage: irid-emul [-hv] [opt]... <file>\n\n"
        "Emulate the Irid architecture.\n\n"
        "  file         the binary file to load at address 0x0000\n"
        "  -h, --help   show this and exit\n"
        "  -v           show the version ID\n"
        "  --version    show the full version and exit\n"
    );

    exit(0);
}

static void version()
{
    printf(
        "irid-emul %s\n"
        "Copyright (C) 2021 bellrise\n"
        "source: https://github.com/bellrise/irid\n",
        IRID_EMUL_VERSION
    );
    exit(0);
}
