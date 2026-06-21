// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "utf8.hpp"

#include <algorithm>

namespace utf8
{
size_t codepoint_size(const std::string& str, const size_t pos)
{
    if (pos >= str.size()) {
        return 0;
    }

    const auto c = static_cast<unsigned char>(str[pos]);

    if (c < 0x80) {
        return 1;
    }

    if ((c & 0xe0) == 0xc0) {
        return 2;
    }

    if ((c & 0xf0) == 0xe0) {
        return 3;
    }

    if ((c & 0xf8) == 0xf0) {
        return 4;
    }

    // Invalid UTF-8 lead byte (0x80-0xBF continuation bytes as lead,
    // 0xC0-0xC1 overlong encodings, 0xF5-0xFF out of range).
    // Treat as single invalid byte and let caller handle corruption.
    return 1;
}

bool is_valid(const std::string& str)
{
    for (size_t pos = 0; pos < str.size();) {
        const size_t cp_size = codepoint_size(str, pos);

        if ((cp_size == 0) || ((pos + cp_size) > str.size())) {
            return false;
        }

        if (cp_size > 1) {
            for (size_t i = 1; i < cp_size; ++i) {
                const auto c = static_cast<unsigned char>(str[pos + i]);

                if ((c & 0xc0) != 0x80) {
                    return false;
                }
            }
        }

        pos += cp_size;
    }

    return true;
}

bool is_single_byte_ascii(const std::string& str)
{
    return (str.size() == 1) &&
           (static_cast<unsigned char>(str[0]) < 0x80);
}

size_t display_width(const std::string& str)
{
    size_t result = 0;

    for (size_t pos = 0; pos < str.size();) {
        const size_t cp_size = codepoint_size(str, pos);

        if (cp_size == 0) {
            break;
        }

        ++result;

        pos += std::min(cp_size, str.size() - pos);
    }

    return result;
}

void erase_last_codepoint(std::string& str)
{
    if (str.empty()) {
        return;
    }

    size_t pos = str.size() - 1;

    while ((pos > 0) &&
           ((static_cast<unsigned char>(str[pos]) & 0xc0) == 0x80)) {
        --pos;
    }

    str.erase(pos);
}

std::string truncate_to_display_width(
    const std::string& str,
    const size_t max_w)
{
    std::string result;
    size_t current_w = 0;

    for (size_t pos = 0; pos < str.size();) {
        const size_t cp_size = codepoint_size(str, pos);

        if ((cp_size == 0) || ((pos + cp_size) > str.size())) {
            break;
        }

        if (current_w >= max_w) {
            break;
        }

        result += str.substr(pos, cp_size);
        ++current_w;
        pos += cp_size;
    }

    return result;
}

}  // namespace utf8
