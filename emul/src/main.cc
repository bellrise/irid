/* irid-emul
   Copyright (c) 2023-2024 bellrise */

#include "emul.h"

#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

void parse_args(struct settings& settings, int argc, char **argv);
static void load_images(const std::vector<image_argument>& images,
                        memory& memory);

int main(int argc, char **argv)
{
    struct settings settings = {};
    u16 serial_addr;

    settings.target_ips = 10000;
    settings.show_perf_results = false;

    parse_args(settings, argc, argv);

    memory ram(IRID_MAX_ADDR + 1, IRID_PAGE_SIZE);
    cpu cpu(ram);

    cpu.set_target_ips(settings.target_ips);

    load_images(settings.images, ram);
    cpu.add_device(console_create(STDIN_FILENO, STDOUT_FILENO));

    serial_addr = 0x100;
    for (const serial_argument& arg : settings.serials)
        cpu.add_device(serial_create(serial_addr++, arg.name, arg.file));

    /* Run the CPU. */
    cpu.start();

    if (settings.show_perf_results)
        cpu.print_perf();

    cpu.remove_devices();
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
