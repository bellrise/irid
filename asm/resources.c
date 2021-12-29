/* Subroutines for managing system resources.
   Copyright (C) 2021 bellrise */

#include "asm.h"


void free_resources()
{
    info("freeing resources");
    free(GLOB_RUNTIME->sources);
}
