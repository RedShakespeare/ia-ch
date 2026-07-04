// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <string>

#include "catch.hpp"
#include "io.hpp"
#include "test_utils.hpp"
#include "text_format.hpp"

// Regression test for UTF-8 corruption in to_upper/first_to_upper/first_to_lower.
//
// Previously these functions used ::toupper/::tolower (or std::transform with
// ::toupper) on the raw char bytes. Since char is signed on most platforms,
// CJK UTF-8 lead bytes (0xE0-0xEF, negative as signed char) were passed to
// ::toupper, which is undefined behavior (the C standard requires the
// argument to be representable as unsigned char or EOF). On glibc this was
// typically a no-op, but on other platforms (e.g. MSVC/MinGW, likely for
// Chinese players) it could corrupt the byte, breaking the UTF-8 sequence.
// A corrupted string then produced wrong pixel advances from the text-run
// cache, causing the side stats panel labels to overflow / overlap / shift.
//
// The fix only uppercases ASCII lowercase letters (a-z) and leaves all other
// bytes untouched, preserving valid UTF-8.

TEST_CASE("to_upper preserves CJK UTF-8 bytes")
{
    const std::string cjk = "\xe7\xbc\x93\xe6\x85\xa2";  // 缓慢
    const std::string upper = text_format::to_upper(cjk);

    REQUIRE(upper == cjk);
    REQUIRE(upper.size() == cjk.size());
}

TEST_CASE("first_to_upper preserves CJK UTF-8 bytes")
{
    const std::string cjk = "\xe7\x8e\xa9\xe5\xae\xb6\xe5\x80\x92\xe4\xb8\x8b";  // 玩家倒下
    const std::string result = text_format::first_to_upper(cjk);

    REQUIRE(result == cjk);
    REQUIRE(result.size() == cjk.size());
}

TEST_CASE("first_to_lower preserves CJK UTF-8 bytes")
{
    const std::string cjk = "\xe7\x8e\xa9\xe5\xae\xb6";  // 玩家
    const std::string result = text_format::first_to_lower(cjk);

    REQUIRE(result == cjk);
    REQUIRE(result.size() == cjk.size());
}

TEST_CASE("to_upper still uppercases ASCII")
{
    REQUIRE(text_format::to_upper("abc") == "ABC");
    REQUIRE(text_format::to_upper("a1b2c3") == "A1B2C3");
    REQUIRE(text_format::to_upper("") == "");
}

TEST_CASE("first_to_upper still uppercases ASCII")
{
    REQUIRE(text_format::first_to_upper("abc") == "Abc");
    REQUIRE(text_format::first_to_upper("") == "");
}

TEST_CASE("first_to_lower still lowercases ASCII")
{
    REQUIRE(text_format::first_to_lower("ABC") == "aBC");
    REQUIRE(text_format::first_to_lower("") == "");
}

TEST_CASE("to_upper does not change CJK text advance")
{
    test_utils::init_all();

    const std::string cjk = "\xe7\xbc\x93\xe6\x85\xa2";  // 缓慢
    const std::string upper = text_format::to_upper(cjk);

    const int adv_before = io::text_advance_px(cjk);
    const int adv_after = io::text_advance_px(upper);

    REQUIRE(adv_before > 0);
    REQUIRE(adv_after == adv_before);

    test_utils::cleanup_all();
}

TEST_CASE("first_to_upper does not change CJK text advance")
{
    test_utils::init_all();

    const std::string cjk = "\xe7\x8e\xa9\xe5\xae\xb6";  // 玩家
    const std::string result = text_format::first_to_upper(cjk);

    const int adv_before = io::text_advance_px(cjk);
    const int adv_after = io::text_advance_px(result);

    REQUIRE(adv_before > 0);
    REQUIRE(adv_after == adv_before);

    test_utils::cleanup_all();
}

TEST_CASE("to_upper handles mixed ASCII and CJK")
{
    const std::string mixed = "abc\xe7\xbc\x93\xe6\x85\xa2";  // abc缓慢
    const std::string result = text_format::to_upper(mixed);

    REQUIRE(result == "ABC\xe7\xbc\x93\xe6\x85\xa2");
    REQUIRE(result.size() == mixed.size());
}
