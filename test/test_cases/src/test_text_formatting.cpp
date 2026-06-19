// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <string>
#include <vector>

#include "catch.hpp"
#include "text_format.hpp"

TEST_CASE("Text formatting")
{
    std::string str = "one two three four";

    std::vector<std::string> lines;

    lines = text_format::split(str, 100);
    REQUIRE(lines[0] == str);
    REQUIRE(lines.size() == 1);

    lines = text_format::split(str, 18);
    REQUIRE("one two three four" == lines[0]);
    REQUIRE(1 == (int)lines.size());

    lines = text_format::split(str, 17);
    REQUIRE("one two three" == lines[0]);
    REQUIRE("four" == lines[1]);
    REQUIRE(2 == (int)lines.size());

    lines = text_format::split(str, 15);
    REQUIRE("one two three" == lines[0]);
    REQUIRE("four" == lines[1]);
    REQUIRE(2 == (int)lines.size());

    lines = text_format::split(str, 11);
    REQUIRE("one two" == lines[0]);
    REQUIRE("three four" == lines[1]);
    REQUIRE(2 == (int)lines.size());

    str = "123456";
    lines = text_format::split(str, 4);
    REQUIRE("123456" == lines[0]);
    REQUIRE(1 == (int)lines.size());

    str = "12 345678";
    lines = text_format::split(str, 4);
    REQUIRE("12" == lines[0]);
    REQUIRE("345678" == lines[1]);
    REQUIRE(2 == (int)lines.size());

    str = "one\ntwo\n\nthree four";
    lines = text_format::split(str, 10);
    REQUIRE(lines.size() == 4);
    REQUIRE(lines[0] == "one");
    REQUIRE(lines[1] == "two");
    REQUIRE(lines[2] == "");
    REQUIRE(lines[3] == "three four");

    str = "";
    lines = text_format::split(str, 4);
    REQUIRE(lines.empty());
}

TEST_CASE("Text formatting counts Chinese UTF-8 characters as display cells")
{
    std::string str = "one 玩家 two";

    const auto lines = text_format::split(str, 5);

    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == "one");
    REQUIRE(lines[1] == "玩家");
    REQUIRE(lines[2] == "two");
}

TEST_CASE("Text formatting wraps Chinese UTF-8 text without spaces")
{
    const auto lines = text_format::split("玩家角色探索", 3);

    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0] == "玩家角");
    REQUIRE(lines[1] == "色探索");
}
