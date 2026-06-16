// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <string>

#include "catch.hpp"
#include "utf8.hpp"

TEST_CASE("UTF-8 Chinese text display width")
{
    REQUIRE(utf8::is_valid("玩家"));
    REQUIRE(utf8::display_width("玩家") == 2);
    REQUIRE(utf8::display_width("A玩家7") == 4);
}

TEST_CASE("UTF-8 Chinese text truncation preserves codepoint boundaries")
{
    REQUIRE(utf8::truncate_to_display_width("玩家角色", 3) == "玩家角");
    REQUIRE(utf8::truncate_to_display_width("AB玩家", 3) == "AB玩");
}

TEST_CASE("UTF-8 backspace erases a complete Chinese character")
{
    std::string str = "玩家A";

    utf8::erase_last_codepoint(str);
    REQUIRE(str == "玩家");

    utf8::erase_last_codepoint(str);
    REQUIRE(str == "玩");

    utf8::erase_last_codepoint(str);
    REQUIRE(str.empty());
}
