/* Emulator API declarations.
   Copyright (C) 2021 bellrise */
#ifndef IRID_EMUL_H
#define IRID_EMUL_H

#include <stddef.h>

/* The emulator version ID is a unique ID that gets incremented on each update,
   so you can either check for a feature using the preprocessor, or a script by
   using the `irid-emul -v` command. See doc/emul-version for more. */
#define IRID_EMUL_VERID         1
#define IRID_EMUL_VERSION       "0.1"


/* Emulate the Irid CPU architecture. `binfile` can be a path to a file which to
   load at the virtual address 0x0000. If `load_offt` is not 0, the file will be
   loaded at that position. If the program exits normally, 0 will be returned.
   Otherwise, if the error is not handled and the program cannot continue, a CPU
   fault ID will be returned (one of CPUFAULT_*, found in <irid/arch.h>).

   Any failure caused by the operating system or invalid arguments will return
   a negative value. -1 is a OS error, -2 means the binfile was not found, and
   -3 any other. */
int irid_emulate(const char *binfile, size_t load_offt, int opts);


#endif /* IRID_EMUL_H */
