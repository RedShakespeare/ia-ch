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
#include "i18n.hpp"
#include "paths.hpp"
#include "saving.hpp"
#include "test_utils.hpp"

static void remove_logo_variant_if_exists(const std::string& path)
{
    std::error_code err;
    std::filesystem::remove(path, err);
}

TEST_CASE("Logo path prefers language-specific numbered variants")
{
    test_utils::init_all();

    config::set_language("zh_CN");
    i18n::reload();

    const auto logo_dir = paths::data_dir() + "locale/zh_CN/gfx/images";
    std::filesystem::create_directories(logo_dir);
    remove_logo_variant_if_exists(paths::images_dir() + "/main_menu_logo_0.png");
    remove_logo_variant_if_exists(paths::images_dir() + "/main_menu_logo_2.png");

    const auto base = logo_dir + "/main_menu_logo_";
    {
        std::ofstream(base + "0.png").put('x');
        std::ofstream(base + "1.png").put('x');
    }

    REQUIRE(
        std::filesystem::path(paths::logo_img_path()).lexically_normal() ==
        std::filesystem::path(base + "0.png").lexically_normal());

    std::filesystem::remove(base + "0.png");
    std::filesystem::remove(base + "1.png");

    test_utils::cleanup_all();
}

TEST_CASE("Logo path falls back to default numbered variants")
{
    test_utils::init_all();

    config::set_language("test_logo_fallback");
    i18n::reload();

    const auto logo_dir = paths::images_dir();
    const auto base = logo_dir + "/main_menu_logo_";
    {
        std::ofstream(base + "0.png").put('x');
        std::ofstream(base + "2.png").put('x');
    }

    REQUIRE(
        std::filesystem::path(paths::logo_img_path()).lexically_normal() ==
        std::filesystem::path(base + "0.png").lexically_normal());

    std::filesystem::remove(base + "0.png");
    std::filesystem::remove(base + "2.png");

    test_utils::cleanup_all();
}

TEST_CASE("Logo path uses unnumbered logo when no variants exist")
{
    test_utils::init_all();

    config::set_language("en");
    i18n::reload();
    remove_logo_variant_if_exists(paths::images_dir() + "/main_menu_logo_0.png");
    remove_logo_variant_if_exists(paths::images_dir() + "/main_menu_logo_2.png");
    remove_logo_variant_if_exists(paths::data_dir() + "locale/zh_CN/gfx/images/main_menu_logo_0.png");
    remove_logo_variant_if_exists(paths::data_dir() + "locale/zh_CN/gfx/images/main_menu_logo_1.png");

    REQUIRE(
        std::filesystem::path(paths::logo_img_path()).lexically_normal() ==
        std::filesystem::path(paths::images_dir() + "/main_menu_logo.png").lexically_normal());

    test_utils::cleanup_all();
}

TEST_CASE("Save metadata stores insanity for menu use")
{
    test_utils::init_all();

    const auto path = paths::save_insanity_file_path();

    {
        std::ofstream file(path, std::ios::trunc);
        REQUIRE(file.is_open());
        file << "73";
    }

    REQUIRE(saving::save_file_insanity_for_menu() == 73);

    std::filesystem::remove(path);

    test_utils::cleanup_all();
}
