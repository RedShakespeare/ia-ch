// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "paths.hpp"

#include "SDL.h"

#include "debug.hpp"
#include "version.hpp"


namespace paths
{

std::string gfx_dir()
{
        return "gfx/";
}

std::string fonts_dir()
{
        return gfx_dir() + "/fonts/";
}

std::string tiles_dir()
{
        return gfx_dir() + "/tiles/24x24/";
}

std::string images_dir()
{
        return gfx_dir() + "/images/";
}

std::string logo_img_path()
{
        return images_dir() + "/main_menu_logo.png";
}

std::string skull_img_path()
{
        return images_dir() + "/skull.png";
}

std::string audio_dir()
{
        return "audio/";
}

std::string data_dir()
{
        return "data/";
}

std::string user_dir()
{
        std::string version_str;

        if (version_info::g_version_str.empty())
        {
                version_str = version_info::read_git_sha1_str_from_file();
        }
        else
        {
                version_str = version_info::g_version_str;
        }

        const auto path_ptr =
                // NOTE: This is somewhat of a hack, see the function arguments
                SDL_GetPrefPath(
                        "infra_arcana",         // "Organization"
                        version_str.c_str());   // "Application"

        const std::string path_str = path_ptr;

        SDL_free(path_ptr);

        TRACE << "User data directory: " << path_str << std::endl;

        return path_str;
}

std::string save_file_path()
{
        return user_dir() + "save";
}

std::string config_file_path()
{
        return user_dir() + "config";
}

std::string highscores_file_path()
{
        return user_dir() + "highscores";
}

} // paths
