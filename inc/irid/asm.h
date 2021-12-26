/* Assembler API declarations.
   Copyright (C) 2021 bellrise */

#ifndef IRID_ASM_H
#define IRID_ASM_H

/*
 * The assembler version ID is a unique ID that gets incremented on each update,
 * so you can check it using a preprocessor.
 */

#define IRID_ASM_VERID          1
#define IRID_ASM_VERSION        "0.1"

/*
 * Assemble the file at the given path. Returns 0 if the compilation succeeds.
 *
 * @param   path        path to the source file
 * @param   dest        path to the destination file
 * @param   opts        assembly options
 */
int irid_assemble(char *path, char *dest, int opts);


#endif /* IRID_ASM_H */
