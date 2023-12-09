/* buffer
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void buffer_mmap(struct buffer *self, const char *path)
{
    struct stat file_stat;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd == -1)
        return;
    if (fstat(fd, &file_stat) == -1)
        return;

    self->size = file_stat.st_size;
    self->mem = mmap(NULL, self->size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (self->mem == MAP_FAILED) {
        self->mem = NULL;
        self->size = 0;
        return;
    }

    self->type = BUFFER_TYPE_MMAP;
    close(fd);
}

void buffer_free_contents(struct buffer *self)
{
    if (self->type == BUFFER_TYPE_MMAP)
        munmap(self->mem, self->size);
    if (self->type == BUFFER_TYPE_REGION)
        free(self->mem);
}
