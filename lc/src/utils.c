/* Leaf compiler
   Copyright (c) 2023 bellrise */

#include "lc.h"

#include <stdlib.h>
#include <string.h>

char *string_copy(const char *src, int len)
{
    char *str;

    str = ac_alloc(global_ac, len + 1);
    memcpy(str, src, len);
    str[len] = 0;

    return str;
}

char *string_copy_z(const char *src)
{
    return string_copy(src, strlen(src));
}
