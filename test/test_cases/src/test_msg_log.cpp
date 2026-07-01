// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <vector>

#include "catch.hpp"
#include "config.hpp"
#include "msg_log.hpp"
#include "panel.hpp"
#include "test_utils.hpp"

namespace
{
std::vector<test_utils::CapturedTextDraw> log_draws()
{
    std::vector<test_utils::CapturedTextDraw> result;

    for (const auto& draw : test_utils::captured_text_draws()) {
        if (draw.panel == Panel::log) {
            result.push_back(draw);
        }
    }

    return result;
}
}  // namespace

TEST_CASE("Message log uses rendered width for adjacent CJK messages")
{
    test_utils::init_all();
    msg_log::init();

    test_utils::enable_wide_cjk_text_stub(true);
    test_utils::clear_captured_text_draws();

    msg_log::add("你好");
    msg_log::add("世界");
    msg_log::draw();

    const auto draws = log_draws();

    REQUIRE(draws.size() == 2);
    REQUIRE(draws[0].text == "你好");
    REQUIRE(draws[1].text == "世界");

    // Layout is now pixel-based. Each CJK glyph renders at 2*cell px with the wide-CJK
    // stub, so "你好" occupies 4*cell px, plus a 1-cell gap -> the second message starts
    // at 5*cell px. Only the x component is layout-derived; y is the panel pixel origin.
    const int cell = config::gui_cell_px_w();

    REQUIRE(draws[0].pos.x == 0);
    REQUIRE(draws[1].pos.x == 5 * cell);

    test_utils::enable_wide_cjk_text_stub(false);
    test_utils::cleanup_all();
}

TEST_CASE("Message log spacing for ASCII messages is unchanged")
{
    test_utils::init_all();
    msg_log::init();

    test_utils::enable_wide_cjk_text_stub(false);
    test_utils::clear_captured_text_draws();

    msg_log::add("ab");
    msg_log::add("cd");
    msg_log::draw();

    const auto draws = log_draws();

    REQUIRE(draws.size() == 2);
    REQUIRE(draws[0].text == "ab");
    REQUIRE(draws[1].text == "cd");

    const int cell = config::gui_cell_px_w();

    REQUIRE(draws[0].pos.x == 0);
    REQUIRE(draws[1].pos.x == 3 * cell);

    test_utils::cleanup_all();
}
