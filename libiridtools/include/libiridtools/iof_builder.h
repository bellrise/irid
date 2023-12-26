/* libiridtools - library for the Irid toolchain
   Copyright (c) 2023 bellrise */

#pragma once

#include <irid/iof.h>
#include <libiridtools/bytebuffer.h>
#include <stddef.h>
#include <string>
#include <vector>

class iof_builder;
class iof_section_builder;

class iof_builder
{
  public:
    /* Returns the built IOF file. */
    bytebuffer build();

    void add_section(const iof_section_builder& section);

  private:
    struct section
    {
        bytebuffer source;
        size_t file_offset;
    };

    std::vector<section> m_sections;
};

class iof_section_builder
{
  public:
    iof_section_builder();

    /* Returns the built section. */
    bytebuffer build() const;

    void set_code(bytebuffer buf);
    void set_origin(u16 origin);
    void set_flag(u16 flags);
    void set_name(const std::string& name);

    void add_symbol(const std::string& name, u16 offset);
    void add_link(const std::string& to, u16 addr);
    void add_export(const std::string& label, u16 offset);

  private:
    struct string
    {
        std::string val;
        u16 id;
        u16 addr;

        string(const std::string& val, u16 id)
            : val(val)
            , id(id)
        { }
    };

    bool m_origin_set;
    iof_section m_header;
    bytebuffer m_code;
    std::string m_name;
    std::vector<string> m_strings;
    std::vector<iof_symbol> m_symbols;
    std::vector<iof_link> m_links;
    std::vector<iof_export> m_exports;

    u16 string_id(const std::string& str);
};
