/* Emulator API declarations.
   Copyright (C) 2021-2022 bellrise */

#ifndef IRID_EMUL_H
#define IRID_EMUL_H

#include <stddef.h>

/*
 * The emulator version ID is a unique ID that gets incremented on each update,
 * so you can either check for a feature using the preprocessor, or a script by
 * using the `irid-emul -v` command. See doc/emul-version for more.
 */

#define IRID_EMUL_VERID         3
#define IRID_EMUL_VERSION       "0.3"

#define IRID_EMUL_QUIET         1

/*
 * Emulate the Irid CPU architecture. If the program exits normally, 0 will be
 * returned. Otherwise, if the error is not handled and the program cannot
 * continue, a CPU fault ID will be returned (one of CPUFAULT_*, found in
 * <irid/arch.h>).
 *
 * Any failure caused by the operating system or invalid arguments will return
 * a negative value. -1 is a OS error, -2 means the binfile was not found, and
 * -3 any other.
 *
 * The window may be NULL, or a pointer to the Emulator Graphics API window
 * struct. It is defined to take a void* in order to not dirty this namespace
 * by including emul_graphics.h. If that is defined, any change to video memory
 * in the emulator will also show up on the window. The window should already be
 * open & configured for a 80x25 character display. See doc/display for more.
 *
 * If IRID_EMUL_QUIET is passed as an option, no output will be shown. This
 * is overriden by compiling in DEBUG mode, which will force info() text to be
 * shown.
 *
 * @param   binfile     the file to load
 * @param   load_offt   offset in memory where the file will be loaded
 * @param   opts        options (unused)
 * @param   win         pointer to window from the emulator graphics API
 */
int irid_emulate(const char *binfile, size_t load_offt, int opts, void *win);


#endif /* IRID_EMUL_H */
