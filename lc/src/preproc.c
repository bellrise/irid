/* Leaf compiler
   Copyright (c) 2023 bellrise */

#include "lc.h"

#include <stdlib.h>
#include <string.h>

char *preproc_remove_comments(char *source)
{
    char *p;
    char *q;

    p = source;
    while (*p) {
        if (p[0] == '/' && p[1] == '/') {
            q = p;
            while (*q != '\n')
                q++;
            memset(p, ' ', (size_t) q - (size_t) p);
        }

        p++;
    }

    return source;
}

char *preproc_replace_tabs(char *source)
{
    char *new_buffer;
    size_t tab_count;
    size_t source_len;
    size_t buffer_size;
    size_t write_index;

    tab_count = 0;
    source_len = 0;

    for (size_t i = 0; source[i]; i++) {
        if (source[i] == '\t')
            tab_count += 1;
        source_len += 1;
    }

    /* Re-allocate with more space for spaces, and copy it over. */

    buffer_size = source_len + tab_count * 3;
    new_buffer = calloc(1, buffer_size + 16);
    write_index = 0;

    for (size_t i = 0; i < source_len; i++) {
        if (source[i] == '\t') {
            new_buffer[write_index++] = ' ';
            new_buffer[write_index++] = ' ';
            new_buffer[write_index++] = ' ';
            new_buffer[write_index++] = ' ';
        } else {
            new_buffer[write_index++] = source[i];
        }
    }

    free(source);

    return new_buffer;
}
