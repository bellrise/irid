/* irid-ld - irid linker
   Copyright (c) 2023 bellrise */

#include "ld.h"

#include <cstring>
#include <irid/iof.h>
#include <map>

linker::linker() { }

void linker::add_object(const bytebuffer& object)
{
    bytebuffer buf = object;
    const byte *base_ptr;
    const iof_header *header;
    const iof_pointer *section_pointers;
    const iof_section *section_header;
    size_t ptr;

    buf.freeze();
    base_ptr = buf.get_range(0, buf.len()).ptr;

    header = (const iof_header *) base_ptr;
    debug("base_ptr=%p", base_ptr);
    debug("section_count=%zu section_addr=0x%zx", header->h_section_count,
          header->h_section_addr);

    section_pointers =
        reinterpret_cast<const iof_pointer *>(base_ptr + sizeof(iof_header));

    for (int ind = 0; ind < header->h_section_count; ind++) {
        ptr = section_pointers[ind].p_addr;
        section_header = reinterpret_cast<const iof_section *>(base_ptr + ptr);

        const char *sname =
            (const char *) (section_header->s_sname_addr + base_ptr);

        debug("--- Section %d [%.*s] ---", ind, section_header->s_sname_size,
              sname);
        debug("s_code_addr=0x%zx", section_header->s_code_addr);
        debug("s_code_size=0x%zx", section_header->s_code_size);
        debug("s_links_addr=0x%zx", section_header->s_links_addr);
        debug("s_links_count=0x%zx", section_header->s_links_count);
        debug("s_exports_addr=0x%zx", section_header->s_exports_addr);
        debug("s_exports_count=0x%zx", section_header->s_exports_count);
        debug("s_strings_addr=0x%zx", section_header->s_strings_addr);
        debug("s_strings_count=0x%zx", section_header->s_strings_count);
        debug("s_sname_addr=0x%zx", section_header->s_sname_addr);
        debug("s_sname_size=0x%zx", section_header->s_sname_size);

        std::map<int, std::string> string_map;

        for (int i = 0; i < section_header->s_strings_count; i++) {
            const iof_string *iofs =
                (const iof_string *) (section_header->s_strings_addr
                                      + base_ptr);
            debug("str %d:0x%zx '%s'", iofs[i].s_id, iofs[i].s_addr + ptr,
                  iofs[i].s_addr + ptr + base_ptr);
            string_map[iofs[i].s_id] =
                (char *) (iofs[i].s_addr + ptr + base_ptr);
        }

        for (int i = 0; i < section_header->s_links_count; i++) {
            const iof_link *iofl =
                (const iof_link *) (section_header->s_links_addr + base_ptr);
            debug("link 0x%04x :: %s", iofl[i].l_addr,
                  string_map[iofl[i].l_strid].c_str());
        }

        for (int i = 0; i < section_header->s_exports_count; i++) {
            const iof_export *iofe =
                (const iof_export *) (section_header->s_exports_addr
                                      + base_ptr);
            debug("export 0x%04x :: %s", iofe[i].e_offset,
                  string_map[iofe[i].e_strid].c_str());
        }
    }
}

bytebuffer linker::link()
{
    bytebuffer res;

    // TODO

    return res;
}
