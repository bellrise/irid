/* Irid assembler internal header.
   Copyright (C) 2021-2022 bellrise */

#include <irid/common.h>
#include <irid/asm.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define _Unused     __attribute__((unused))

#ifdef DEBUG
# define info(...)      _info_impl("irid-asm", __func__, __VA_ARGS__)
#else
# define info(...)
#endif

#define die(...)        _die_impl("irid-asm", __VA_ARGS__)
#define warn(...)       _warn_impl("irid-asm", __VA_ARGS__)

/* Initial size of the hashmap. */
#define HM_INIT_SIZE        128

struct asm_ctx;
struct hashmap;

/*
 * Runtime information. Only a single global instace is created (GLOB_RUNTIME),
 * storing all data required for the program to work. This is an alternative to
 * global variables, so only one is accessible.
 */
struct runtime
{
    int             nsources;
    char            **sources;
    struct asm_ctx  **contexts;
};

/*
 * Assembler context. Each translation unit gets its own assembler context
 * struct allocated.
 */
struct asm_ctx
{
    FILE    *in;                /* input */
    FILE    *out;               /* output */
    char    *path;              /* path to file */
    int     pos[2];             /* line, pos */
};

/* Hash map element. */
struct hm_elem
{
    char *key;                  /* hashmap key */

    union
    {
        void    *vptr;          /* any pointer */
        char    *vstr;          /* pointer to string */
        size_t  vnum;           /* any number */
    };

    struct hm_elem *next;       /* pointer to next element */
};

/*
 * A hashmap stores a key-value pair in slots. To get the index of a element,
 * the string key is hashed. The free_call is a function which is called before
 * an element is free'd from memory, either with hm_remove or hm_free.
 *
 * Remember that iterating over the hashmap needs to be done using hm_next,
 * not using a simple C for loop with an index.
 */
struct hashmap
{
    size_t          size;       /* amount of filled elements */
    size_t          space;      /* space in the hashmap */
    struct hm_elem  **slots;    /* array of slots */

    size_t iter_pos[2];
    int (*free_call) (struct hm_elem *item);
};

/* Global runtime instance. */
extern struct runtime *GLOB_RUNTIME;

/*
 * Parse command-line arguments and put them in the runtime struct.
 *
 * @param   rt          assembler runtime info
 * @param   argc        argc
 * @param   argv        argv
 */
int argparse(struct runtime *rt, int argc, char **argv);

/*
 * Hash a null-terminated string and return an arch-bit unsigned integer.
 *
 * @param   str         string to hash
 */
size_t hash(char *str);

/*
 * Initalize the hash map. This only allocates space for some elements, and any
 * access should be done using the hm_* methods. The initial size of the array
 * is HM_INIT_SIZE.
 *
 * @param   hm          uninitialized hashmap
 */
void hm_init(struct hashmap *hm);

/*
 * Free the whole hashmap.
 *
 * @param   hm          hashmap to free
 */
void hm_free(struct hashmap *hm);

/*
 * Acquire a pointer to a single element in the hashmap. If no slots are left,
 * the hash map will resize itself. Note that when this happens, a relatively
 * large amount of memory will get copied over to a new hash map, because if
 * the size changes the keys will also change.
 *
 * @param   hm          hashmap to acquire a slot in
 * @param   key         string key of the element
 */
struct hm_elem *hm_acquire(struct hashmap *hm, char *key);

/*
 * Get a pointer to the element in the hashmap. May return NULL if the element
 * goes not exist.
 *
 * @param   hm          hashmap to find the element in
 * @param   key         string key of the element
 */
struct hm_elem *hm_get(struct hashmap *hm, char *key);

/*
 * For building iterators, returns the next element from the iterator. In order
 * to reset the iterator, call hm_reset_iter() on the hashmap. When the iterator
 * reaches the end of the hashmap, this will return NULL.
 *
 * Simple example for building an iterator:
 *
 *  struct hm_elem *elem;
 *
 *  hm_reset_iter(&hm);
 *  while ((elem = hm_next(&hm))) {
 *      do_with(elem);
 *  }
 *
 * @param   hm          hashmap to iterate over
 */
struct hm_elem *hm_next(struct hashmap *hm);

/*
 * Reset the iterator. See hm_next().
 *
 * @param   hm          hashmap to reset
 */
inline void hm_reset_iter(struct hashmap *hm)
{
    hm->iter_pos[0] = 0;
    hm->iter_pos[1] = 0;
}

/*
 * Remove an element from the hashmap with the given key. Returns 0 if the
 * operation succeeds, and 1 if the element is not found.
 *
 * @param   hm          hashmap to remove from
 * @param   key         string key of the element
 */
int hm_remove(struct hashmap *hm, char *key);

/*
 * This subroutine is called after exit() is called. All allocated global
 * resources are free'd using this function. Passed to atexit() in main().
 */
void free_resources();
