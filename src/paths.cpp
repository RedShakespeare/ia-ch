// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "paths.hpp"

namespace paths
{

const std::string gfx_dir = "gfx";

const std::string fonts_dir = gfx_dir + "/fonts";
const std::string tiles_dir = gfx_dir + "/tiles/24x24";
const std::string images_dir = gfx_dir + "/images";

const std::string logo_img_path = images_dir + "/main_menu_logo.png";
const std::string skull_img_path = images_dir + "/skull.png";

const std::string audio_dir = "audio";

const std::string data_dir = "data";

const std::string user_dir = "user";

const std::string save_file_path = user_dir + "/save";

const std::string config_file_path = user_dir + "/config";

const std::string highscores_file_path = user_dir + "/highscores";

} // paths
