// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "paths.hpp"

namespace paths
{

const std::string g_gfx_dir = "gfx";

const std::string g_fonts_dir = g_gfx_dir + "/fonts";
const std::string g_tiles_dir = g_gfx_dir + "/tiles/24x24";
const std::string g_images_dir = g_gfx_dir + "/images";

const std::string g_logo_img_path = g_images_dir + "/main_menu_logo.png";
const std::string g_skull_img_path = g_images_dir + "/skull.png";

const std::string g_audio_dir = "audio";

const std::string g_data_dir = "data";

const std::string g_user_dir = "user";

const std::string g_save_file_path = g_user_dir + "/save";

const std::string g_config_file_path = g_user_dir + "/config";

const std::string g_highscores_file_path = g_user_dir + "/highscores";

} // paths
