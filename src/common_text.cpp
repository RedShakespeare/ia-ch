// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "common_text.hpp"

#include "i18n.hpp"

namespace common_text
{
std::string g_next_page_up_hint;
std::string g_next_page_down_hint;
std::string g_screen_exit_hint;
std::string g_minimap_exit_hint;
std::string g_game_over_summary_exit_hint;
std::string g_set_option_hint;
std::string g_scroll_hint;
std::string g_scrollable_info_screen_hint;
std::string g_cancel_hint;
std::string g_confirm_hint;
std::string g_confirm_drop_hint;
std::string g_any_key_hint;
std::string g_yes_or_no_hint;
std::string g_direction_query;
std::string g_disarm_no_trap;
std::string g_not_while_blind;
std::string g_not_while_entangled;
std::string g_not_while_stuck;
std::string g_cannot_see_there;
std::string g_cannot_remove_normal_means;
std::string g_blocked;
std::string g_something_blocking_it;
std::string g_mon_prevent_cmd;
std::string g_fire_prevent_cmd;
std::string g_shock_prevent_cmd;
std::string g_mon_disappear;
std::string g_mon_disappear_reappear;
std::string g_miscast_player;
std::string g_miscast_mon;

std::vector<std::string> g_exorcist_purge_phrases;

void init()
{
    g_next_page_up_hint =
        i18n::get("common.next_page_up_hint", "[page up, <] more");

    g_next_page_down_hint =
        i18n::get("common.next_page_down_hint", "[page down, >] more");

    g_screen_exit_hint =
        i18n::get("common.screen_exit_hint", "[space, esc] to exit");

    g_minimap_exit_hint =
        i18n::get("common.minimap_exit_hint", "[space, esc, m] to exit");

    g_game_over_summary_exit_hint =
        i18n::get("common.game_over_summary_exit_hint", "[space, esc] to show high scores");

    g_set_option_hint =
        i18n::get("common.set_option_hint", "[enter, left, right] to set option");

    g_scroll_hint =
        i18n::get("common.scroll_hint", "[2/8, down/up, pgup/pgdown, home/end] to scroll");

    g_scrollable_info_screen_hint =
        g_scroll_hint +
        " " +
        g_screen_exit_hint;

    g_cancel_hint =
        i18n::get("common.cancel_hint", "[space, esc] to cancel");

    g_confirm_hint =
        i18n::get("common.confirm_hint", "[space, esc, enter] to continue");

    g_confirm_drop_hint =
        i18n::get("common.confirm_drop_hint", "[enter] to confirm");

    g_any_key_hint =
        i18n::get("common.any_key_hint", "[any key] to continue");

    g_yes_or_no_hint =
        i18n::get("common.yes_or_no_hint", "[y/n]");

    g_direction_query =
        i18n::get("common.direction_query", "Which direction?");

    g_disarm_no_trap =
        i18n::get("common.disarm_no_trap", "I find nothing there to disarm.");

    g_not_while_blind =
        i18n::get("common.not_while_blind", "Not while blind.");

    g_not_while_entangled =
        i18n::get("common.not_while_entangled", "Not while entangled.");

    g_not_while_stuck =
        i18n::get("common.not_while_stuck", "Not while stuck.");

    g_cannot_see_there =
        i18n::get("common.cannot_see_there", "I cannot see there.");

    g_cannot_remove_normal_means =
        i18n::get("common.cannot_remove_normal_means", "It cannot be removed through normal means.");

    g_blocked =
        i18n::get("common.blocked", "It's blocked.");

    g_something_blocking_it =
        i18n::get("common.something_blocking_it", "Something is blocking it.");

    g_mon_prevent_cmd =
        i18n::get("common.mon_prevent_cmd", "Not while an enemy is near.");

    g_shock_prevent_cmd =
        i18n::get("common.shock_prevent_cmd", "Not while insanity is near.");

    g_fire_prevent_cmd =
        i18n::get("common.fire_prevent_cmd", "Fire is spreading!");

    g_mon_disappear =
        i18n::get("common.mon_disappear", "suddenly disappears!");

    g_mon_disappear_reappear =
        i18n::get("common.mon_disappear_reappear", "suddenly disappears and reappears!");

    g_miscast_player =
        i18n::get("common.miscast_player", "I fail to concentrate!");

    g_miscast_mon =
        i18n::get("common.miscast_mon", "fails to concentrate.");

    g_exorcist_purge_phrases = {
        i18n::get("common.exorcist_purge_phrase_1", "This place feels more serene now."),
        i18n::get("common.exorcist_purge_phrase_2", "The sanctity of this place has been somewhat restored."),
        i18n::get("common.exorcist_purge_phrase_3", "A great wickedness has been extinguished."),
        i18n::get("common.exorcist_purge_phrase_4", "I sense a stillness permeating throughout the area.")};
}

}  // namespace common_text
