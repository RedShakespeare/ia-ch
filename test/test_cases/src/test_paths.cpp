// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <filesystem>
#include <fstream>
#include <string>

#include "catch.hpp"
#include "paths.hpp"
#include "test_utils.hpp"

static std::string dir_string(const std::filesystem::path& path)
{
    std::string result = path.generic_string();

    if (!result.empty() && (result.back() != '/')) {
        result += "/";
    }

    return result;
}

static std::filesystem::path normalized_path(const std::string& path)
{
    return std::filesystem::path(path).lexically_normal();
}

TEST_CASE("Installed resource paths resolve from SDL base directory")
{
    const std::filesystem::path game_dir("test_path_game_dir");

    std::filesystem::remove_all(game_dir);
    std::filesystem::create_directories(game_dir);

    test_utils::set_sdl_base_dir_stub(dir_string(game_dir));
    paths::init();

    REQUIRE(normalized_path(paths::data_dir()) == normalized_path(dir_string(game_dir / "data")));
    REQUIRE(normalized_path(paths::gfx_dir()) == normalized_path(dir_string(game_dir / "gfx")));
    REQUIRE(normalized_path(paths::audio_dir()) == normalized_path(dir_string(game_dir / "audio")));

    test_utils::reset_sdl_path_stubs();
    paths::init();
    std::filesystem::remove_all(game_dir);
}

TEST_CASE("Relative user data path resolves from game directory")
{
    const std::filesystem::path game_dir("test_path_user_data_dir");

    std::filesystem::remove_all(game_dir);
    std::filesystem::create_directories(game_dir);

    {
        std::ofstream file(game_dir / "user_data.ini", std::ios::trunc);

        REQUIRE(file.is_open());

        file << "[paths]\n";
        file << "user_data=profile\n";
    }

    test_utils::set_sdl_base_dir_stub(dir_string(game_dir));
    paths::init();

    REQUIRE(
        normalized_path(paths::user_dir()) ==
        normalized_path(dir_string(game_dir / "profile")));

    test_utils::reset_sdl_path_stubs();
    paths::init();
    std::filesystem::remove_all(game_dir);
}
