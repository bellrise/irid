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
#define IOF_FORMAT 2

#define IOFS_PIC 0x1

enum iof_header_endianness_type
{
    IOF_ENDIAN_LITTLE = 0,
};

struct iof_header
{
    u8 h_magic[4];
    u8 h_format;
    u8 h_addrwidth;
    u16 h_section_count;
    u16 h_section_addr; /* iof_pointer[] */
    u8 h_endianness;
    u8 h_0[5];
};

struct iof_pointer
{
    u16 p_addr;
};

enum iof_section_flag
{
    IOF_SFLAG_STATIC_ORIGIN = 1,
};

struct iof_section
{
    u16 s_flag;
    u16 s_origin;
    u16 s_code_size;
    u16 s_code_addr;
    u16 s_links_count;
    u16 s_links_addr; /* iof_link[] */
    u16 s_symbols_count;
    u16 s_symbols_addr; /* iof_symbol[] */
    u16 s_exports_count;
    u16 s_exports_addr; /* iof_export[] */
    u16 s_strings_count;
    u16 s_strings_addr; /* iof_string[] */
    u16 s_sname_size;
    u16 s_sname_addr; /* char[] */
};

/* symbol. */
struct iof_symbol
{
    u16 l_strid; /* string ID */
    u16 l_addr;  /* address in section */
};

/* String. */
struct iof_string
{
    u16 s_id;   /* string ID */
    u16 s_addr; /* char * address in section */
};

/* Link point in binary code. */
struct iof_link
{
    u16 l_strid; /* string ID */
    u16 l_addr;  /* u16 address in section */
};

/* Exported symbol. */
struct iof_export
{
    u16 e_strid;  /* string ID */
    u16 e_offset; /* offset in section */
};

#endif /* IRID_IOF_H */
