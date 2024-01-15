/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *file_read(const char *path)
{
    char *contents;
    long read_siz;
    long siz;
    FILE *f;

    if (!(f = fopen(path, "r")))
        die("failed to open file %s", path);

    fseek(f, 0, SEEK_END);
    siz = ftell(f);
    rewind(f);

    /* malloc 16 bytes more, for some buffer-overflow protection */
    contents = malloc(siz + 16);
    read_siz = fread(contents, 1, siz, f);

    if (read_siz != siz)
        die("failed to read file %s\n", path);

    contents[siz] = 0;

    fclose(f);
    return contents;
}
