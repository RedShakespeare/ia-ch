// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <filesystem>
#include <fstream>
#include <string>

#include "catch.hpp"
#include "config.hpp"
#include "config_options.hpp"
#include "paths.hpp"
#include "test_utils.hpp"

namespace
{
std::string dir_string(const std::filesystem::path& path)
{
    std::string result = path.generic_string();

    if (!result.empty() && (result.back() != '/')) {
        result += "/";
    }

    return result;
}

class IsolatedConfigDir
{
public:
    explicit IsolatedConfigDir(const std::filesystem::path& game_dir)
        : m_game_dir(game_dir)
    {
        std::filesystem::remove_all(m_game_dir);
        std::filesystem::create_directories(m_game_dir / "gfx" / "fonts");

        {
            std::ofstream font_file(
                m_game_dir / "gfx" / "fonts" / "14x24_zhaohua.png",
                std::ios::trunc);

            REQUIRE(font_file.is_open());
        }

        {
            std::ofstream user_data_ini(m_game_dir / "user_data.ini", std::ios::trunc);

            REQUIRE(user_data_ini.is_open());

            user_data_ini << "[paths]\n";
            user_data_ini << "user_data=profile\n";
        }

        test_utils::set_sdl_base_dir_stub(dir_string(m_game_dir));
        paths::init();
    }

    ~IsolatedConfigDir()
    {
        test_utils::reset_sdl_path_stubs();
        paths::init();
        std::filesystem::remove_all(m_game_dir);
    }

private:
    std::filesystem::path m_game_dir;
};
}  // namespace

TEST_CASE("Manual video scale change prevents later automatic default reset")
{
    IsolatedConfigDir isolated_config_dir("test_config_video_scale_dir");

    config::init();

    REQUIRE(config::video_scale_factor() == 1);

    config::VideoScaleOption option;
    option.change(config::OptionChangeCommand::right);

    REQUIRE(config::video_scale_factor() == 2);

    const bool changed = config::apply_default_video_scale_factor_if_unset();

    REQUIRE_FALSE(changed);
    REQUIRE(config::video_scale_factor() == 2);
}
