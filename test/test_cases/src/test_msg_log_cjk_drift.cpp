// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <algorithm>
#include <vector>

#include "catch.hpp"
#include "config.hpp"
#include "io.hpp"
#include "msg_log.hpp"
#include "panel.hpp"
#include "test_utils.hpp"

// Regression test for the CJK message-log ordering bug.
//
// Real CJK fonts (e.g. 14x24_zhaohua: cell=14, CJK advance=22) have a glyph
// advance that is NOT an integer multiple of the cell width. Previously the
// message log measured text width in COLUMNS via ceil(pixel_width / cell_px_w),
// which over-reserved space per CJK message and accumulated drift across
// adjacent messages. A sequence of short CJK messages whose real pixel width
// still fit on one log line was judged as not fitting, triggering a premature
// line break (or, on the last line, a more-prompt that cleared the whole log
// and re-added the message at line 0) -- visually reordering messages.
//
// The layout is now pixel-based, so this drift no longer occurs. This test
// feeds a run of single-CJK-glyph messages whose real pixel width fits on one
// line (with the worst-case repeat-string reservation) and asserts that every
// message is drawn on the same (first) log line.
//
// For ASCII (English), px == chars * cell exactly, so there was never any drift
// -- English was always unaffected.
namespace
{
std::vector<int> log_draw_ys()
{
    std::vector<int> ys;

    for (const auto& draw : test_utils::captured_text_draws()) {
        if (draw.panel == Panel::log) {
            ys.push_back(draw.pos.y);
        }
    }

    return ys;
}
}  // namespace

TEST_CASE("CJK non-integer glyph advance does not prematurely wrap log")
{
    test_utils::init_all();
    msg_log::init();

    const int cell = config::gui_cell_px_w();
    const int log_w_px = panels::w(Panel::log) * cell;

    // Use a CJK advance that is a non-integer multiple of the cell, matching
    // real font metrics (e.g. cell 14 -> adv 22; cell 13 -> adv 22; cell 16 ->
    // adv 24; cell 12 -> adv 16).
    const int adv =
        (cell == 14) ? 22 :
        (cell == 13) ? 22 :
        (cell == 16) ? 24 :
        (cell == 12) ? 16 :
        (cell * 3 + 1) / 2;  // generic fallback: ~1.5x, non-integer-multiple

    test_utils::set_cjk_advance_override_px(adv);

    const int cols_per_msg = (adv + cell - 1) / cell;  // ceil
    const int drift_per_msg = cols_per_msg * cell - adv;  // >0 for non-integer-multiple

    // Skip if this font happens to be integer-multiple (no drift, nothing to reproduce).
    if (drift_per_msg == 0) {
        test_utils::set_cjk_advance_override_px(0);
        test_utils::cleanup_all();
        return;
    }

    const int repeat_str_px = io::text_advance_px("(x9)");

    // Find the largest count whose real pixel width (with gaps + worst-case
    // repeat-string reservation) still fits on one line.
    int count = 0;

    while (true) {
        const int next = count + 1;
        const int px_end =
            next * (adv + cell) - cell + repeat_str_px;  // glyphs+gaps, no trailing gap, +repeat

        if (px_end >= log_w_px) {
            break;
        }

        count = next;
    }

    if (count < 3) {
        test_utils::set_cjk_advance_override_px(0);
        test_utils::cleanup_all();
        return;
    }

    test_utils::clear_captured_text_draws();

    const std::string one_cjk = "\xe4\xb8\x8a";  // 上

    for (int i = 0; i < count; ++i) {
        // Distinct messages so they don't collapse into a repeat counter.
        msg_log::add(one_cjk + std::to_string(i));
    }

    msg_log::draw();

    const auto ys = log_draw_ys();

    REQUIRE(!ys.empty());

    // Since the real pixel width fits on one line, every message must be drawn
    // on the same (first) log line. A differing y means the layout wrapped a
    // message to a later line prematurely.
    const int first_y = ys[0];

    bool wrapped_prematurely = false;

    for (const int y : ys) {
        if (y != first_y) {
            wrapped_prematurely = true;
        }
    }

    INFO("cell=" << cell << " adv=" << adv << " log_w_px=" << log_w_px
         << " count=" << count << " drift_per_msg=" << drift_per_msg
         << " px_end=" << (count * (adv + cell) - cell + repeat_str_px));

    REQUIRE_FALSE(wrapped_prematurely);

    test_utils::set_cjk_advance_override_px(0);
    test_utils::cleanup_all();
}
