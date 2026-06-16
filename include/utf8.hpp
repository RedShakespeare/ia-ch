// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef UTF8_HPP
#define UTF8_HPP

#include <cstddef>
#include <string>

namespace utf8
{

size_t codepoint_size(const std::string& str, size_t pos);

bool is_valid(const std::string& str);

bool is_single_byte_ascii(const std::string& str);

size_t display_width(const std::string& str);

void erase_last_codepoint(std::string& str);

std::string truncate_to_display_width(
    const std::string& str,
    size_t max_w);

}  // namespace utf8

#endif  // UTF8_HPP
