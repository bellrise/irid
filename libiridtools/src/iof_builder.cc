/* libiridtools
   Copyright (c) 2023 bellrise */

#include <cstring>
#include <irid/iof.h>
#include <libiridtools/iof_builder.h>
#include <stdexcept>

bytebuffer iof_builder::build()
{
    bytebuffer res;
    iof_header header;
    iof_section section_header;
    iof_pointer ptr;

    memset(&header, 0, sizeof(header));
    memcpy(header.h_magic, IOF_MAGIC, 4);

    header.h_format = IOF_FORMAT;
    header.h_section_count = m_sections.size();
    header.h_section_addr = sizeof(header);

    res.append_range(&header, sizeof(header));
    res.insert_fill(0, sizeof(header), sizeof(iof_pointer) * m_sections.size());

    for (size_t i = 0; i < m_sections.size(); i++) {
        section& sec = m_sections[i];
        size_t section_offset;

        section_offset = res.len();

        /* Read the section header from the byte buffer, update its offsets and
           write it back to the buffer, writing it to the file. */
        range header = sec.source.get_range(0, sizeof(iof_section));
        memcpy(&section_header, header.ptr, header.len);

        section_header.s_code_addr += section_offset;
        section_header.s_links_addr += section_offset;
        section_header.s_exports_addr += section_offset;
        section_header.s_strings_addr += section_offset;
        section_header.s_sname_addr += section_offset;

        ptr.p_addr = section_offset;

        /* Copy the header back to the section buffer. */
        sec.source.insert_range(
            range<byte>(reinterpret_cast<byte *>(&section_header),
                        sizeof(iof_section)),
            0);

        /* Insert pointer to section. */
        res.insert_range(range<byte>(&ptr, sizeof(ptr)),
                         sizeof(header) + sizeof(iof_pointer) * i);

        /* Append section. */
        res.append_buffer(sec.source);
    }

    return res;
}

void iof_builder::add_section(const iof_section_builder& part)
{
    section res;

    res.source = part.build();
    res.file_offset = 0;

    m_sections.push_back(res);
}
