/* Irid Object Format declaration header.
   Copyright (C) 2023 bellrise */

#ifndef IRID_IOF_H
#define IRID_IOF_H

/*
 * This file format is used for transfering compiled code for the Irid
 * architecture along with useful metadata which allow for linking multiple
 * compiled objects together using a linker.
 */

#ifndef IRID_DEFINED_UX
# define IRID_DEFINED_UX 1
typedef unsigned short u16;
typedef unsigned char u8;
#endif

#define IOF_MAGIC  "IOF\x7f"
#define IOF_FORMAT 1

struct iof_header
{
    u8 h_magic[4];
    u8 h_format;
    u8 h_0;
    u16 h_symbols_count;
    u16 h_symbols_addr;
    u16 h_links_count;
    u16 h_links_addr;
    u16 h_bin_size;
    u16 h_bin_addr;
};

struct iof_symbol
{
    u16 s_id;
    u16 s_nameaddr;
};

struct iof_link
{
    u16 l_id;
    u16 l_addr;
};

#endif /* IRID_IOF_H */
