/* libiridtools
   Copyright (c) 2023-2024 bellrise */

#include <libiridtools/iof_builder.h>
#include <stdexcept>

iof_section_builder::iof_section_builder()
    : m_origin_set(false)
{ }

bytebuffer iof_section_builder::build() const
{
    iof_section header;
    bytebuffer res;
    size_t code_len;

    header = m_header;
    res.append_range(&header, sizeof(header));

    /* Remove unnecessary padding before the first origin, if any. */

    if (m_origin_set) {
        code_len = m_code.len() - header.s_origin;
        header.s_code_addr = res.len();
        header.s_code_size = code_len;
        res.append_range(m_code.get_range(header.s_origin, code_len));
    }

    else {
        header.s_code_addr = res.len();
        header.s_code_size = m_code.len();
        res.append_buffer(m_code);
    }

    /* Insert symbols. */
    header.s_symbols_addr = res.len();
    header.s_symbols_count = m_symbols.size();
    for (const iof_symbol& sym : m_symbols)
        res.append_range(&sym, sizeof(sym));

    /* Insert link points. */
    header.s_links_addr = res.len();
    header.s_links_count = m_links.size();
    for (const iof_link& link : m_links)
        res.append_range(&link, sizeof(link));

    /* Insert export points. */
    header.s_exports_addr = res.len();
    header.s_exports_count = m_exports.size();
    for (const iof_export& export_ : m_exports)
        res.append_range(&export_, sizeof(export_));

    /* Insert name. */
    header.s_sname_addr = res.len();
    header.s_sname_size = m_name.size();
    res.append_range(m_name.c_str(), m_name.size());
    res.append(0);

    /* Insert strings. */
    header.s_strings_addr = res.len();
    header.s_strings_count = m_strings.size();

    res.insert_fill(0, header.s_strings_addr,
                    sizeof(iof_string) * m_strings.size());

    for (size_t i = 0; i < m_strings.size(); i++) {
        iof_string str_header;
        str_header.s_id = m_strings[i].id;
        str_header.s_addr = res.len();

        res.insert_range(range<byte>(&str_header, sizeof(str_header)),
                         header.s_strings_addr + sizeof(str_header) * i);

        res.append_range(m_strings[i].val.c_str(), m_strings[i].val.size());
        res.append(0);
    }

    /* Write the header back. */
    res.insert_range(range<byte>(&header, sizeof(header)), 0);

    return res;
}

void iof_section_builder::set_code(bytebuffer buf)
{
    m_code = buf;
    m_header.s_code_size = buf.len();
}

void iof_section_builder::set_origin(u16 origin)
{
    m_header.s_flag |= IOF_SFLAG_STATIC_ORIGIN;
    m_header.s_origin = origin;
    m_origin_set = true;
}

void iof_section_builder::set_flag(u16 flags)
{
    m_header.s_flag = flags;
}

void iof_section_builder::set_name(const std::string& name)
{
    m_name = name;
}

void iof_section_builder::add_symbol(const std::string& name, u16 offset)
{
    m_symbols.push_back(iof_symbol{string_id(name), offset});
}

void iof_section_builder::add_link(const std::string& to, u16 addr)
{
    m_links.push_back(iof_link{string_id(to), addr});
}

void iof_section_builder::add_export(const std::string& label, u16 offset)
{
    m_exports.push_back(iof_export{string_id(label), offset});
}

u16 iof_section_builder::string_id(const std::string& str)
{
    u16 id;

    for (const string& string : m_strings) {
        if (string.val == str)
            return string.id;
    }

    id = m_strings.size();
    m_strings.push_back(string(str, id));
    return id;
}
