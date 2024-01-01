/* strlist
   Copyright (c) 2023-2024 bellrise */

#include "ld.h"

#include <stdlib.h>
#include <string.h>

void strlist_append(struct strlist *list, const char *string)
{
    list->strings = realloc(list->strings, sizeof(char *) * (list->size + 1));
    list->strings[list->size++] = strdup(string);
}

void strlist_free(struct strlist *list)
{
    for (size_t i = 0; i < list->size; i++)
        free(list->strings[i]);
    free(list->strings);

    list->strings = NULL;
    list->size = 0;
}
