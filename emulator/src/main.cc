/* irid-emul
   Copyright (c) 2023 bellrise */

#include "emul.h"

#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

struct image_argument
{
    char path[256];
    int offset;
};

struct settings
{
    std::vector<image_argument> images;
    bool show_perf_results;
    int target_ips;
};

static void add_keyboard_device(cpu&);
static void parse_args(struct settings& settings, int argc, char **argv);
static void load_images(const std::vector<image_argument>& images,
                        memory& memory);

int main(int argc, char **argv)
{
    struct settings settings;

    settings.target_ips = 10000;

    parse_args(settings, argc, argv);

    memory ram(0x10000, IRID_PAGE_SIZE);
    cpu cpu(ram);

    cpu.set_target_ips(settings.target_ips);

    load_images(settings.images, ram);
    add_keyboard_device(cpu);

    /* Run the CPU. */
    cpu.start();

    if (settings.show_perf_results)
        cpu.print_perf();
}

void add_keyboard_device(cpu& cpu)
{
    device console = {0x1000, "console"};

    console.read = []() {
        return fgetc(stdin);
    };

    console.write = [](u8 byte) {
        fputc(byte, stdout);
        fflush(stdout);
    };

    console.poll = []() {
        struct pollfd poll_rq;
        poll_rq.fd = STDIN_FILENO;
        poll_rq.events = POLLIN;

        poll(&poll_rq, 1, 0);
        return poll_rq.revents & POLLIN;
    };

    cpu.add_device(console);
}

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
         "  -h, --help      show the help page\n"
         "  -i, --ips [Hz]  target instructions per second (e.g. 1k)\n"
         "  -p, --perf      show performace results on exit (e.g. ips)\n"
         "  -v, --version   show the emulator version\n");
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

void parse_args(struct settings& settings, int argc, char **argv)
{
    int opt_index;
    int c;

    static struct option long_opts[] = {{"help", no_argument, 0, 'h'},
                                        {"ips", required_argument, 0, 'i'},
                                        {"perf", no_argument, 0, 'p'},
                                        {"version", no_argument, 0, 'v'},
                                        {0, 0, 0, 0}};

    opt_index = 0;

    if (argc == 1) {
        short_usage();
        exit(0);
    }

    while (1) {
        c = getopt_long(argc, argv, "hi:pv", long_opts, &opt_index);
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
        case 'i':
            settings.target_ips = parse_int(optarg);
            break;
        }
    }

    while (optind < argc)
        settings.images.push_back(parse_image_argument(argv[optind++]));
}

static void load_images(const std::vector<image_argument>& images,
                        memory& memory)
{
    for (const image_argument& image : images) {
        /* Load image file into memory. */
        if (access(image.path, R_OK))
            die("cannot access %s", image.path);

        struct stat fileinfo;
        char *buf;
        int fd;

        fd = open(image.path, O_RDONLY);
        fstat(fd, &fileinfo);
        buf = (char *) malloc(fileinfo.st_size);
        read(fd, buf, fileinfo.st_size);

        /* Copy the buffer into the CPU memory. */
        memory.write_range(image.offset, buf, fileinfo.st_size);
        free(buf);
    }
}
