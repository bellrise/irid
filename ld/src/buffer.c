/* buffer
   Copyright (c) 2023-2024 bellrise */

#include "ld.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct buffer *buffer_new()
{
    return calloc(1, sizeof(struct buffer));
}

void buffer_resize(struct buffer *self, size_t size)
{
    self->type = BUFFER_TYPE_MALLOC;
    self->mem = realloc(self->mem, size);
    self->size = size;
}

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

void buffer_write_file(struct buffer *self, const char *path)
{
    FILE *fp;

    fp = fopen(path, "w");
    if (!fp)
        die("failed to open file '%s'", path);

    if (fwrite(self->mem, 1, self->size, fp) != self->size)
        die("failed to write all bytes to file '%s'", path);

    fclose(fp);
}

void buffer_free_contents(struct buffer *self)
{
    if (self->type == BUFFER_TYPE_MMAP)
        munmap(self->mem, self->size);
    if (self->type == BUFFER_TYPE_MALLOC)
        free(self->mem);
}

void buffer_free(struct buffer *self)
{
    if (!self)
        return;

    buffer_free_contents(self);
    free(self);
}
