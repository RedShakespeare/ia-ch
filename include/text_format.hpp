// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef TEXT_FORMAT_HPP
#define TEXT_FORMAT_HPP

#include <string>
#include <vector>

namespace text_format
{

// Reads a line of space separated words, and splits them into several lines
// with the given maximum width.  If any single word in the "line" parameter is
// longer than the maximum width, this word will NOT be split (the entire word
// is simply added to the output vector, breaking the maximum width).
std::vector<std::string> split(std::string line, int max_w);

std::vector<std::string> space_separated_list(const std::string& line);

std::string replace_all(
        const std::string& line,
        const std::string& from,
        const std::string& to);

std::string pad_before(
        const std::string& str,
        size_t tot_w,
        char c = ' ');

std::string pad_after(
        const std::string& str,
        size_t tot_w,
        char c = ' ');

std::string first_to_lower(const std::string& str);
std::string first_to_upper(const std::string& str);
std::string all_to_upper(const std::string& str);

void append_with_space(
        std::string& base_str,
        const std::string& addition);

} // namespace text_format

#endif // TEXT_FORMAT_HPP
