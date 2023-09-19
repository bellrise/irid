/* Irid assembler
   Copyright (c) 2023 bellrise */

#include "as.h"

#include <cstring>

iof_builder::iof_builder(const bytebuffer& source_code)
    : m_code(source_code)
{ }

iof_builder::iof_builder(bytebuffer&& source_code)
    : m_code(std::move(source_code))
{ }

void iof_builder::add_link(const std::string& link_destination, size_t offset)
{
    m_links.push_back({symbol_id(link_destination), static_cast<u16>(offset)});
}

void iof_builder::add_export(const std::string& exported_name, size_t offset)
{
    m_exports.push_back({symbol_id(exported_name), static_cast<u16>(offset)});
}

bytebuffer iof_builder::build()
{
    std::vector<named_addr> placement_addrs;
    bytebuffer buf;
    iof_header header;

    buf.append_range<byte>((const byte *) &header, sizeof(header));

    std::memcpy(header.h_magic, IOF_MAGIC, 4);
    header.h_format = IOF_FORMAT;

    header.h_symbols_count = m_symbols.size();
    header.h_symbols_addr = buf.len();
    placement_addrs = build_symbol_table(buf);

    header.h_links_count = m_links.size();
    header.h_links_addr = buf.len();
    build_link_table(buf);

    header.h_exports_count = m_exports.size();
    header.h_exports_addr = buf.len();
    build_export_table(buf);

    header.h_bin_size = m_code.len();
    header.h_bin_addr = buf.len();
    build_code(buf);

    build_symbol_strings(buf, placement_addrs);

    buf.insert_range(range<byte>((const byte *) &header, sizeof(header)), 0);

    return buf;
}

u16 iof_builder::symbol_id(const std::string& symbol)
{
    for (size_t i = 0; i < m_symbols.size(); i++) {
        if (symbol == m_symbols[i].first)
            return m_symbols[i].second;
    }

    m_symbols.push_back({symbol, m_symbols.size()});
    return m_symbols.size() - 1;
}

std::vector<iof_builder::named_addr>
iof_builder::build_symbol_table(bytebuffer& buf)
{
    std::vector<named_addr> placement_addrs;
    iof_symbol sym;

    for (const auto& symbol : m_symbols) {
        sym.s_id = symbol.second;
        sym.s_nameaddr = 0;
        placement_addrs.push_back({symbol.first, buf.len() + 2});
        buf.append_range((const byte *) &sym, sizeof(sym));
    }

    return placement_addrs;
}

void iof_builder::build_symbol_strings(
    bytebuffer& buf, const std::vector<named_addr>& placement_addrs)
{
    u16 addr;
    for (const named_addr& placement : placement_addrs) {
        addr = buf.len();
        buf.insert_range(range<byte>((byte *) &addr, 2), placement.second);
        buf.append_range(placement.first.c_str(), placement.first.length());
        buf.append(0);
    }
}

void iof_builder::build_code(bytebuffer& buf)
{
    buf.append_range(m_code.get_range(0, m_code.len()));
}

void iof_builder::build_link_table(bytebuffer& buf)
{
    for (iof_link link : m_links)
        buf.append_range((const byte *) &link, sizeof(link));
}

void iof_builder::build_export_table(bytebuffer& buf)
{
    for (iof_export exported_symbol : m_exports) {
        buf.append_range((const byte *) &exported_symbol,
                         sizeof(exported_symbol));
    }
}
