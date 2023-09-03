/* irid-emul
   Copyright (c) 2023 bellrise */

#include "emul.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 2)
        die("missing image file", argv[0]);

    memory ram(0x10000, IRID_PAGE_SIZE);
    cpu cpu(ram);

    /* Load image file into memory. */
    if (access(argv[1], R_OK))
        die("cannot access %s", argv[1]);

    struct stat fileinfo;
    char *buf;
    int fd;

    fd = open(argv[1], O_RDONLY);
    fstat(fd, &fileinfo);
    buf = (char *) malloc(fileinfo.st_size);
    read(fd, buf, fileinfo.st_size);

    /* Copy the buffer into the CPU memory. */
    ram.write_range(0, buf, fileinfo.st_size);
    free(buf);

    /* Add a basic console device. */

    device console = {0x1000, "console"};

    console.read = []() {
        return fgetc(stdin);
    };

    console.write = [](u8 byte) {
        fputc(byte, stdout);
    };

    cpu.add_device(console);

    /* Run the CPU. */

    cpu.start();
}
