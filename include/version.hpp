// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef VERSION_HPP
#define VERSION_HPP

#include <string>

namespace version_info
{

extern const std::string version_str;

extern const std::string copyright_str;

extern const std::string license_str;

extern const std::string date_str;

extern const std::string read_git_sha1_str_from_file();

} // version_info

#endif // VERSION_HPP
