/* libiridtools
   Copyright (c) 2023 bellrise */

#include <libiridtools/iof_builder.h>
#include <stdexcept>

iof_section_builder::iof_section_builder() { }

iof_section_builder& iof_section_builder::set_name(const std::string& name)
{
    m_name = name;
    return *this;
}

iof_section_builder& iof_section_builder::set_origin(size_t origin_addr)
{
    m_origin = origin_addr;
    return *this;
}

iof_section_builder& iof_section_builder::set_code(bytebuffer code)
{
    m_code = code;
    return *this;
}

iof_section_builder& iof_section_builder::add_link(size_t offset,
                                                   const std::string& symbol)
{
    m_links.push_back({offset, symbol});
    return *this;
}

iof_section_builder& iof_section_builder::add_export(size_t offset,
                                                     const std::string& symbol)
{
    m_exports.push_back({offset, symbol});
    return *this;
}

std::string iof_section_builder::name() const
{
    return m_name;
}

size_t iof_section_builder::origin() const
{
    return m_origin;
}

std::vector<std::pair<size_t, std::string>> iof_section_builder::exports() const
{
    return m_exports;
}

std::vector<std::pair<size_t, std::string>> iof_section_builder::links() const
{
    return m_links;
}

bytebuffer iof_section_builder::code() const
{
    return m_code;
}
