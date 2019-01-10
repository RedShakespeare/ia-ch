// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "version.hpp"

#include <fstream>

#include "debug.hpp"

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

namespace version_info
{

// This shall be set when (and only when) building a tagged release. Use the
// format "vMAJOR.MINOR".
const std::string version_str = "";

const std::string copyright_str =
        "(c) 2011-2019 Martin Tornqvist";

const std::string license_str =
        "Infra Arcana is free software, see LICENSE.txt.";

const std::string date_str = __DATE__;

const std::string read_git_sha1_str_from_file()
{
        const std::string default_sha1 = "unknown_revision";

        const std::string sha1_file_path = "res/git-sha1.txt";

        std::ifstream file(sha1_file_path);

        if (!file.is_open())
        {
                TRACE << "Failed to open git sha1 file at "
                      << sha1_file_path
                      << std::endl;

                return default_sha1;
        }

        std::string sha1 = "";

        getline(file, sha1);

        file.close();

        if (sha1.empty())
        {
                return default_sha1;
        }

        return sha1;
}

} // version_info
