/* Hash subroutine.
   Copyright (C) 2021 bellrise */

#include <stdint.h>
#include "asm.h"

/*
 * Thanks to Austin Appleby for creating the 32 & 64 bit Murmurhash2.
 * Modified `seed` to be taken from random(), not a parameter.
 *
 * Source:
 * <https://github.com/aappleby/smhasher/blob/master/src/MurmurHash2.cpp>
 */

#if __INTPTR_MAX__ == __INT64_MAX__

size_t hash(char *str)
{
    const uint64_t m = 0xc6a4a7935bd1e995LL;
    uint64_t len, seed;
    const int r = 47;

    seed = random();
    len  = strlen(str);

    uint64_t h = seed ^ (len * m);

    const uint64_t * data = (const uint64_t *) str;
    const uint64_t * end = data + (len/8);

    while (data != end)
    {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char * data2 = (const unsigned char*)data;

    switch (len & 7)
    {
        case 7: h ^= (uint64_t)(data2[6]) << 48;
        case 6: h ^= (uint64_t)(data2[5]) << 40;
        case 5: h ^= (uint64_t)(data2[4]) << 32;
        case 4: h ^= (uint64_t)(data2[3]) << 24;
        case 3: h ^= (uint64_t)(data2[2]) << 16;
        case 2: h ^= (uint64_t)(data2[1]) << 8;
        case 1: h ^= (uint64_t)(data2[0]);
        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

#elif __INTPTR_MAX__ == __INT32_MAX__
#error "murmur hash for 32 bits has not been stolen yet"
#endif
