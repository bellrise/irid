/* libiridtools - library for the Irid toolchain
   Copyright (c) 2023 bellrise */

#pragma once

#include <irid/iof.h>
#include <libiridtools/bytebuffer.h>
#include <stddef.h>
#include <string>
#include <vector>

class iof_section_builder;

class iof_builder
{
  public:
    iof_builder();

    iof_builder& add_section(iof_section_builder&);
    bytebuffer build();

  private:
    struct string_resource
    {
        u16 id;
        std::string value;
    };

    std::vector<iof_section_builder> m_sections;
    std::vector<string_resource> m_strings;
    bytebuffer m_buffer;

    /* Utils */
    u16 register_string(const std::string& val);

    void add_strings_from_section(iof_section_builder&);
};

class iof_section_builder
{
    friend class iof_builder;

  public:
    iof_section_builder();

    iof_section_builder& set_name(const std::string& name);
    iof_section_builder& set_origin(size_t origin_addr);
    iof_section_builder& set_code(bytebuffer code);
    iof_section_builder& add_link(size_t offset, const std::string& symbol);
    iof_section_builder& add_export(size_t offset, const std::string& symbol);

    std::string name() const;
    size_t origin() const;

  private:
    std::vector<std::pair<size_t, std::string>> m_exports;
    std::vector<std::pair<size_t, std::string>> m_links;
    std::string m_name;
    bytebuffer m_code;
    size_t m_origin;

    std::vector<std::pair<size_t, std::string>> exports() const;
    std::vector<std::pair<size_t, std::string>> links() const;
    bytebuffer code() const;
};
