/* libiridtools
   Copyright (c) 2023 bellrise */

#include <cstring>
#include <irid/iof.h>
#include <libiridtools/iof_builder.h>
#include <stdexcept>

iof_builder::iof_builder() { }

iof_builder& iof_builder::add_section(iof_section_builder& section)
{
    m_sections.push_back(section);
    return *this;
}

bytebuffer iof_builder::build()
{
    iof_header header;

    memcpy(header.h_magic, IOF_MAGIC, 4);
    header.h_format = IOF_FORMAT;
    header.h_0 = 0;

    header.h_section_count = m_sections.size();
    header.h_strings_count = m_strings.size();

    /* Start the buffer after the IOF header. */
    m_buffer.insert_fill(0, 0, sizeof(iof_header));

    for (iof_section_builder& section : m_sections) {
        add_strings_from_section(section);
    }

    return m_buffer;
}

u16 iof_builder::register_string(const std::string& val)
{
    string_resource res;

    for (const string_resource& res : m_strings) {
        if (res.value == val)
            return res.id;
    }

    /* Register a new string. */

    res.id = m_strings.size();
    res.value = val;
    m_strings.push_back(res);

    return res.id;
}

void iof_builder::add_strings_from_section(iof_section_builder& section)
{
    for (const auto& thing : section.m_exports)
        register_string(thing.second);
    for (const auto& thing : section.m_links)
        register_string(thing.second);
}
