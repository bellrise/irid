/* Common toolchain utilities.
   Copyright (C) 2021-2022 bellrise */

#ifndef IRID_COMMON_H
#define IRID_COMMON_H

#include <stddef.h>

/*
 * These common function and type declarations are used all across the Irid
 * toolchain, so to keep the code "DRY", all of them are then implemented
 * in the common/ subdirectory of the project.
 *
 * The _xxx_impl functions are designed to be used with internal macros,
 * which automatically should pass the program name or current function
 * name.
 */

/*
 * Print n bytes in hex format, 16 byter per line.
 *
 * @param   addr        address to start from
 * @param   amount      amount of bytes to print
 */
void dbytes(void *addr, size_t amount);

/*
 * Print an error in a bright red color, similar to GCC & Clang errors.
 * This will then exit. In order to properly free any allocated/open
 * resources, the atexit() function should be used.
 *
 * @param   prog        program name
 * @param   fmt         printf-style format string
 * @param   ...         format string arguments
 */
int _die_impl(const char *prog, const char *fmt, ...);

/*
 * Print a warning with a purple color, similar to GCC & Clang warnings.
 *
 * @param   prog        program name
 * @param   fmt         printf-style format string
 * @param   ...         format string arguments
 */
int _warn_impl(const char *prog, const char *fmt, ...);

/*
 * Print an informational message in a calming, blue tint.
 *
 * @param   prog        program name
 * @param   func        function name
 * @param   fmt         printf-style format string
 * @param   ...         format string arguments
 */
int _info_impl(const char *prog, const char *func, const char *fmt, ...);


#endif /* IRID_COMMON_H */
