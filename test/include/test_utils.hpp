// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <string>
#include <vector>

#include "panel.hpp"
#include "pos.hpp"

namespace test_utils
{
struct CapturedTextDraw
{
    std::string text {};
    Panel panel {Panel::screen};
    P pos {};
};

// Initialize a full game session
void init_all();

void cleanup_all();

void enable_wide_cjk_text_stub(bool is_enabled);
void set_cjk_advance_override_px(int px);

void set_sdl_base_dir_stub(const std::string& path);
void reset_sdl_path_stubs();

void clear_captured_text_draws();

const std::vector<CapturedTextDraw>& captured_text_draws();

}  // namespace test_utils

#endif  // TEST_UTILS_HPP
