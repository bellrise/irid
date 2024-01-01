/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <stdarg.h>
#include <stdlib.h>

static const char *find_start_of_line(struct tokens *tokens,
                                      const char *from_here)
{
    const char *p;

    p = from_here;
    while (p > tokens->source) {
        if (p[-1] == '\n')
            return p;
        p--;
    }

    return p;
}

static int line_len(const char *from_here)
{
    const char *p;

    p = from_here;
    while (*p != '\n' && *p != 0)
        p++;

    return (int) ((size_t) p - (size_t) from_here);
}

static int count_line_of_token(struct tokens *tokens, const char *here)
{
    const char *p;
    int line;

    line = 1;
    p = tokens->source;

    while (p != here) {
        if (*p == '\n')
            line++;
        p++;
    }

    return line;
}

void source_error(struct tokens *tokens, const char *here, int here_len,
                  int offset, const char *fix_str, const char *help_str,
                  const char *title, const char *color, const char *fmt,
                  va_list args)
{
    const char *line_start;
    int line_off;
    int line_num;
    int len;

    line_start = find_start_of_line(tokens, here);
    line_num = count_line_of_token(tokens, here);

    if (title) {
        printf("irid-lc: \033[1;98m%s at %s:%d\033[0m\n\n", title,
               tokens->source_name, line_num);
    }

    if (fix_str) {
        printf("    | ");
        line_off = offset + (int) ((size_t) here - (size_t) line_start);
        for (int i = 0; i < line_off; i++)
            fputc(' ', stdout);
        for (int i = 0; i < offset; i++)
            fputc(' ', stdout);
        printf("\033[3;32m%s\033[0m\n    | \033[3;32m", fix_str);

        for (int i = 0; i < line_off; i++)
            fputc(' ', stdout);
        len = here_len - offset;
        for (int i = 0; i < len; i++)
            fputc('v', stdout);

        printf("\033[0m\n");
    }

    else {
        printf("    |\n");
    }

    len = line_len(line_start);

    printf("% 3d | ", line_num);

    len = here - line_start;
    printf("%.*s", len, line_start);
    len = here_len - offset;
    printf("%s%.*s\033[0m", color, len, here + offset);
    len = line_len(here + here_len);
    printf("%.*s", len, here + here_len);

    printf("\n    | %s", color);

    line_off = offset + (int) ((size_t) here - (size_t) line_start);
    for (int i = 0; i < line_off; i++)
        fputc(' ', stdout);
    fputc('^', stdout);

    len = here_len - offset - 1;
    for (int i = 0; i < len; i++)
        fputc('~', stdout);

    fputc(' ', stdout);
    if (args)
        vprintf(fmt, args);
    else
        printf("%s", fmt);
    printf("\033[0m\n");

    if (help_str)
        printf("    |\n    | \033[36m%s\033[0m\n", help_str);

    fputc('\n', stdout);
}

void source_error_nv(struct tokens *tokens, const char *here, int here_len,
                     int offset, const char *fix_str, const char *help_str,
                     const char *title, const char *color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    source_error(tokens, here, here_len, offset, fix_str, help_str, title,
                 color, fmt, args);
    va_end(args);
}
