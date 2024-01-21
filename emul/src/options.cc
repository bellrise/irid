/* irid-emul
   Copyright (c) 2023-2024 bellrise */

#include "emul.h"

#include <getopt.h>
#include <string.h>

static void short_usage()
{
    puts("usage: irid-emul [-h] <image[:addr]> ...");
}

static void usage()
{
    short_usage();
    puts("Emulate the Irid architecture. Loads the given images into memory\n"
         "and starts execution from 0x0000.\n"
         "\n"
         "  -h, --help          show the help page\n"
         "  -i, --ips SPEED     target instructions per second (e.g. 1k)\n"
         "  -p, --perf          show performace results on exit (e.g. ips)\n"
         "  -s, --serial name=NAME,socket=FILE\n"
         "                      create a serial device\n"
         "  -v, --version       show the emulator version\n");
}

static int parse_int(const char *num)
{
    size_t len;
    int base;
    int c;

    len = strlen(num);
    base = 1;

    if (isalpha(num[len - 1])) {
        c = tolower(num[len - 1]);
        if (c == 'm')
            base = 1000000;
        if (c == 'k')
            base = 1000;
    }

    return base * strtol(num, NULL, 10);
}

static image_argument parse_image_argument(const char *str)
{
    image_argument image;
    const char *middle;

    middle = strchr(str, ':');
    if (!middle) {
        image.offset = 0;
        strncpy(image.path, str, 256);
        return image;
    }

    /* Split the image name and offset. */

    image.path[middle - str] = 0;
    strncpy(image.path, str, middle - str);
    image.offset = strtol(middle + 1, NULL, 16);

    return image;
}

static serial_argument parse_serial_argument(char *str)
{
    serial_argument serial;
    char *p;
    char *q;

    while (1) {
        p = strchr(str, ',');
        if (!p)
            p = str + strlen(str);

        q = strchr(str, '=');
        if (!q)
            die("malformed parameter string: %s", str);

        q++;

        if (!strncmp("name", str, q - str - 1))
            serial.name = std::string(q).substr(0, p - q);
        if (!strncmp("file", str, q - str - 1))
            serial.file = std::string(q).substr(0, p - q);

        if (!p[0])
            break;
        str = p + 1;
    }

    return serial;
}

void parse_args(struct settings& settings, int argc, char **argv)
{
    int opt_index;
    int c;

    static struct option long_opts[] = {
        {"help", no_argument, 0, 'h'},    {"ips", required_argument, 0, 'i'},
        {"perf", no_argument, 0, 'p'},    {"serial", required_argument, 0, 's'},
        {"version", no_argument, 0, 'v'}, {0, 0, 0, 0}};

    opt_index = 0;

    if (argc == 1) {
        short_usage();
        exit(0);
    }

    while (1) {
        c = getopt_long(argc, argv, "hi:ps:v", long_opts, &opt_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage();
            exit(0);
        case 'v':
            printf("irid-emul %s\n", IRID_EMUL_VERSION);
            exit(0);
        case 'p':
            settings.show_perf_results = true;
            break;
        case 's':
            settings.serials.push_back(parse_serial_argument(optarg));
            break;
        case 'i':
            settings.target_ips = parse_int(optarg);
            break;
        }
    }

    while (optind < argc)
        settings.images.push_back(parse_image_argument(argv[optind++]));
}
