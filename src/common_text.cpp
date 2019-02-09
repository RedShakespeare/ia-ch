// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "common_text.hpp"

namespace common_text
{

const std::string g_next_page_up_hint =
        "[page up, <] more";

const std::string g_next_page_down_hint =
        "[page down, >] more";

const std::string g_screen_exit_hint =
        "[space, esc] to exit";

const std::string g_scrollable_info_screen_hint =
        "[2/8, down/up] to scroll " +
        g_screen_exit_hint;

const std::string g_cancel_hint =
        "[space, esc] to cancel";

const std::string g_confirm_hint =
        "[space, esc, enter] to continue";

const std::string g_any_key_hint =
        "[any key] to continue";

const std::string g_yes_or_no_hint =
        "[y/n]";

const std::string g_disarm_no_trap =
        "I find nothing there to disarm.";

const std::string g_mon_prevent_cmd =
        "Not while an enemy is near.";

const std::string g_fire_prevent_cmd =
        "Fire is spreading!";

const std::string g_mon_disappear =
        "suddenly disappears!";

} // common_text
