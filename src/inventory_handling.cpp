// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "inventory_handling.hpp"

#include "init.hpp"

#include <iomanip>
#include <sstream>
#include <vector>

#include "actor_player.hpp"
#include "audio.hpp"
#include "browser.hpp"
#include "common_text.hpp"
#include "draw_box.hpp"
#include "drop.hpp"
#include "game_time.hpp"
#include "io.hpp"
#include "item_factory.hpp"
#include "item_potion.hpp"
#include "item_scroll.hpp"
#include "map.hpp"
#include "marker.hpp"
#include "msg_log.hpp"
#include "query.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static const int s_nr_turns_to_handle_armor = 7;

// Index can mean Slot index or Backpack Index (both start from zero)
static bool run_drop_query(
        item::Item& item,
        const InvType inv_type,
        const size_t idx)
{
        TRACE_FUNC_BEGIN;

        const auto& data = item.data();

        msg_log::clear();

        if (data.is_stackable && (item.m_nr_items > 1)) {
                TRACE << "Item is stackable and more than one" << std::endl;

                io::clear_screen();

                states::draw();

                const std::string nr_str =
                        "1-" +
                        std::to_string(item.m_nr_items);

                const std::string drop_str = "Drop how many (" + nr_str + ")?:";

                io::draw_text(
                        drop_str,
                        Panel::screen,
                        P(0, 0),
                        colors::light_white());

                io::update_screen();

                const P nr_query_pos((int)drop_str.size() + 1, 0);

                const int max_digits = 3;
                const P done_inf_pos = nr_query_pos + P(max_digits + 2, 0);

                io::draw_text(
                        "[enter] to drop " + common_text::g_cancel_hint,
                        Panel::screen,
                        done_inf_pos,
                        colors::light_white());

                const int nr_to_drop =
                        query::number(
                                nr_query_pos,
                                colors::light_white(),
                                0,
                                3,
                                item.m_nr_items,
                                false);

                if (nr_to_drop <= 0) {
                        TRACE << "Nr to drop <= 0, nothing to be done"
                              << std::endl;

                        TRACE_FUNC_END;

                        return false;
                } else {
                        // Number to drop is at least one
                        item_drop::drop_item_from_inv(
                                *map::g_player,
                                inv_type,
                                idx,
                                nr_to_drop);

                        TRACE_FUNC_END;

                        return true;
                }
        } else {
                // Not a stack
                TRACE << "Item not stackable, or only one item" << std::endl;

                item_drop::drop_item_from_inv(
                        *map::g_player,
                        inv_type,
                        idx);

                TRACE_FUNC_END;

                return true;
        }

        TRACE_FUNC_END;

        return false;
}

static void cap_str_to_menu_x1(
        std::string& str,
        const int str_x0)
{
        const int name_max_len = panels::x1(Panel::item_menu) - str_x0;

        if ((int)str.length() > name_max_len) {
                str.erase(name_max_len, std::string::npos);
        }
}

// -----------------------------------------------------------------------------
// Abstract inventory screen state
// -----------------------------------------------------------------------------
InvState::InvState()

        = default;

StateId InvState::id()
{
        return StateId::inventory;
}

void InvState::draw_slot(
        const SlotId id,
        const int y,
        const char key,
        const bool is_marked,
        const ItemRefAttInf att_inf) const
{
        // Draw key
        const Color color =
                is_marked
                ? colors::light_white()
                : colors::menu_dark();

        P p(0, y);

        std::string key_str = "?) ";

        key_str[0] = key;

        io::draw_text(
                key_str,
                Panel::item_menu,
                p,
                color);

        p.x += 3;

        // Draw slot label
        const InvSlot& slot = map::g_player->m_inv.m_slots[(size_t)id];

        const std::string slot_name = slot.name;

        io::draw_text(
                slot_name,
                Panel::item_menu,
                p,
                color);

        p.x += 9; // Offset to leave room for slot label

        // Draw item
        const auto* const item = slot.item;

        if (item) {
                // An item is equipped here
                // draw_item_symbol(*item, p);

                // p.x += 2;

                std::string item_name =
                        item->name(
                                ItemRefType::plural,
                                ItemRefInf::yes,
                                att_inf);

                ASSERT(!item_name.empty());

                item_name = text_format::first_to_upper(item_name);

                const Color color_item =
                        is_marked
                        ? colors::light_white()
                        : item->interface_color();

                cap_str_to_menu_x1(item_name, p.x);

                io::draw_text(
                        item_name,
                        Panel::item_menu,
                        p,
                        color_item);

                draw_weight_pct_and_dots(
                        p,
                        item_name.size(),
                        *item,
                        color_item,
                        is_marked);
        } else {
                // No item in this slot
                p.x += 2;

                io::draw_text(
                        "<empty>",
                        Panel::item_menu,
                        p,
                        color);
        }

        if (is_marked) {
                draw_detailed_item_descr(item, att_inf);
        }
}

void InvState::draw_backpack_item(
        const size_t backpack_idx,
        const int y,
        const char key,
        const bool is_marked,
        const ItemRefAttInf att_info) const
{
        // Draw key
        const Color color =
                is_marked
                ? colors::light_white()
                : colors::menu_dark();

        std::string key_str = "?) ";

        key_str[0] = key;

        P p(0, y);

        io::draw_text(
                key_str,
                Panel::item_menu,
                p,
                color);

        p.x += 3;

        // Draw item
        const auto* const item = map::g_player->m_inv.m_backpack[backpack_idx];

        // draw_item_symbol(*item, p);

        // p.x += 2;

        std::string item_name = item->name(
                ItemRefType::plural,
                ItemRefInf::yes,
                att_info);

        item_name = text_format::first_to_upper(item_name);

        cap_str_to_menu_x1(item_name, p.x);

        const Color color_item =
                is_marked
                ? colors::light_white()
                : item->interface_color();

        io::draw_text(
                item_name,
                Panel::item_menu,
                p,
                color_item);

        draw_weight_pct_and_dots(
                p,
                item_name.size(),
                *item,
                color_item,
                is_marked);

        if (is_marked) {
                draw_detailed_item_descr(
                        item,
                        att_info);
        }
}

void InvState::draw_weight_pct_and_dots(
        const P item_pos,
        const size_t item_name_len,
        const item::Item& item,
        const Color& item_name_color,
        const bool is_marked) const
{
        const int weight_carried_tot = map::g_player->m_inv.total_item_weight();

        int item_weight_pct = 0;

        if (weight_carried_tot > 0) {
                item_weight_pct = (item.weight() * 100) / weight_carried_tot;
        }

        std::string weight_str = std::to_string(item_weight_pct) + "%";

        int weight_x = panels::w(Panel::item_menu) - (int)weight_str.size();

        ASSERT(item_weight_pct >= 0 && item_weight_pct <= 100);

        if (item_weight_pct > 0 && item_weight_pct < 100) {
                const P weight_pos(weight_x, item_pos.y);

                const Color weight_color =
                        is_marked
                        ? colors::light_white()
                        : colors::menu_dark();

                io::draw_text(
                        weight_str,
                        Panel::item_menu,
                        weight_pos,
                        weight_color);
        } else {
                // Zero weight, or 100% of weight

                // No weight percent is displayed
                weight_str = "";
                weight_x = panels::w(Panel::item_menu);
        }

        int dots_x = item_pos.x + (int)item_name_len;
        int dots_w = weight_x - dots_x;

        if (dots_w == 0) {
                // Item name fits exactly, just skip drawing dots
                return;
        }

        if (dots_w < 0) {
                // Item name does not fit at all, draw dots until the weight %
                dots_w = 3;
                const int dots_x1 = weight_x - 1;
                dots_x = dots_x1 - dots_w + 1;
        }

        const std::string dots_str(dots_w, '.');

        Color dots_color =
                is_marked
                ? colors::white()
                : item_name_color;

        if (!is_marked) {
                dots_color = dots_color.fraction(2.0);
        }

        io::draw_text(
                dots_str,
                Panel::item_menu,
                P(dots_x, item_pos.y),
                dots_color);
}

void InvState::draw_detailed_item_descr(
        const item::Item* const item,
        const ItemRefAttInf att_inf) const
{
        std::vector<ColoredString> lines;

        if (item) {
                // -------------------------------------------------------------
                // Base description
                // -------------------------------------------------------------
                const auto base_descr = item->descr();

                if (!base_descr.empty()) {
                        for (const std::string& paragraph : base_descr) {
                                lines.emplace_back(
                                        paragraph,
                                        colors::light_white());
                        }
                }

                const bool is_plural =
                        item->m_nr_items > 1 &&
                        item->data().is_stackable;

                const std::string ref_str =
                        is_plural
                        ? "They are "
                        : "It is ";

                const auto& d = item->data();

                // -------------------------------------------------------------
                // Damage dice
                // -------------------------------------------------------------
                if (d.allow_display_dmg) {
                        const std::string dmg_str =
                                item->dmg_str(
                                        att_inf,
                                        ItemRefDmg::range);

                        const std::string dmg_str_avg =
                                item->dmg_str(
                                        att_inf,
                                        ItemRefDmg::average);

                        if (!dmg_str.empty() && !dmg_str_avg.empty()) {
                                lines.emplace_back(
                                        "Damage: " +
                                                dmg_str +
                                                " (average " +
                                                dmg_str_avg +
                                                ")",
                                        colors::light_white());
                        }

                        const std::string hit_mod_str =
                                item->hit_mod_str(att_inf);

                        if (!hit_mod_str.empty()) {
                                lines.emplace_back(
                                        "Hit chance modifier: " +
                                                hit_mod_str,
                                        colors::light_white());
                        }
                }

                // -------------------------------------------------------------
                // Can be used for breaking doors or destroying corpses?
                // -------------------------------------------------------------
                const bool can_att_terrain = d.melee.att_terrain;
                const bool can_att_corpse = d.melee.att_corpse;

                std::string att_obj_str;

                if (can_att_terrain || can_att_corpse) {
                        att_obj_str = "Can be used for ";
                }

                if (can_att_terrain) {
                        att_obj_str += "breaching doors";
                }

                if (can_att_corpse) {
                        if (can_att_terrain) {
                                att_obj_str += " and ";
                        }

                        att_obj_str += "destroying corpses";
                }

                if (can_att_terrain || can_att_corpse) {
                        att_obj_str +=
                                " more effectively (bonus is based on attack "
                                "damage).";

                        lines.emplace_back(
                                att_obj_str,
                                colors::light_white());
                }

                // -------------------------------------------------------------
                // Weight
                // -------------------------------------------------------------
                const std::string weight_str =
                        ref_str + item->weight_str() + " to carry.";

                lines.emplace_back(weight_str, colors::green());

                const int weight_carried_tot =
                        map::g_player->m_inv.total_item_weight();

                int weight_pct = 0;

                if (weight_carried_tot > 0) {
                        weight_pct =
                                (item->weight() * 100) /
                                weight_carried_tot;
                }

                ASSERT(weight_pct >= 0 && weight_pct <= 100);

                if (weight_pct > 0 && weight_pct < 100) {
                        const std::string pct_str =
                                "(" +
                                std::to_string(weight_pct) +
                                "% of total carried weight)";

                        lines.emplace_back(
                                pct_str,
                                colors::green());
                }
        }

        // We draw the description box regardless of whether the lines are
        // empty or not, just to clear this area on the screen.
        io::draw_descr_box(lines);
}

void InvState::activate(const size_t backpack_idx)
{
        auto* item = map::g_player->m_inv.m_backpack[backpack_idx];

        auto result = item->activate(map::g_player);

        if (result == ConsumeItem::yes) {
                map::g_player->m_inv.decr_item_in_backpack(backpack_idx);
        }
}

// -----------------------------------------------------------------------------
// Inventory browsing state
// -----------------------------------------------------------------------------
void BrowseInv::on_start()
{
        map::g_player->m_inv.sort_backpack();

        const int list_size =
                (int)SlotId::END +
                (int)map::g_player->m_inv.m_backpack.size();

        m_browser.reset(
                list_size,
                panels::h(Panel::item_menu));

        m_browser.disable_selection_audio();

        map::g_player->m_inv.sort_backpack();

        audio::play(audio::SfxId::backpack);
}

void BrowseInv::draw()
{
        io::clear_screen();

        draw_box(panels::area(Panel::screen));

        const int browser_y = m_browser.y();

        const auto nr_slots = (size_t)SlotId::END;

        io::draw_text_center(
                " Browsing inventory " + common_text::g_screen_exit_hint + " ",
                Panel::screen,
                P(panels::center_x(Panel::screen), 0),
                colors::title());

        const Range idx_range_shown = m_browser.range_shown();

        int y = 0;

        for (int i = idx_range_shown.min; i <= idx_range_shown.max; ++i) {
                const char key = m_browser.menu_keys()[y];

                const bool is_marked = browser_y == i;

                if (i < (int)nr_slots) {
                        const auto slot_id = (SlotId)i;

                        draw_slot(
                                slot_id,
                                y,
                                key,
                                is_marked,
                                ItemRefAttInf::wpn_main_att_mode);
                } else {
                        // This index is in backpack
                        const size_t backpack_idx = i - nr_slots;

                        draw_backpack_item(
                                backpack_idx,
                                y,
                                key,
                                is_marked,
                                ItemRefAttInf::wpn_main_att_mode);
                }

                ++y;
        }

        // Draw "more" labels
        if (!m_browser.is_on_top_page()) {
                io::draw_text(
                        common_text::g_next_page_up_hint,
                        Panel::item_menu,
                        P(0, -1),
                        colors::light_white());
        }

        if (!m_browser.is_on_btm_page()) {
                io::draw_text(
                        common_text::g_next_page_down_hint,
                        Panel::item_menu,
                        P(0, panels::h(Panel::item_menu)),
                        colors::light_white());
        }
}

void BrowseInv::update()
{
        const auto input = io::get();

        auto& inv = map::g_player->m_inv;

        const MenuAction action =
                m_browser.read(input, MenuInputMode::scrolling_and_letters);

        switch (action) {
        case MenuAction::selected: {
                const auto inv_type_marked =
                        (m_browser.y() < (int)SlotId::END)
                        ? InvType::slots
                        : InvType::backpack;

                if (inv_type_marked == InvType::slots) {
                        InvSlot& slot = inv.m_slots[m_browser.y()];

                        if (!slot.item) {
                                states::push(std::make_unique<Equip>(slot));

                                return;
                        }

                        // Has item in selected slot
                        states::pop();

                        msg_log::clear();

                        if (slot.id == SlotId::body) {
                                on_body_slot_item_selected();
                        } else {
                                inv.unequip_slot(slot.id);
                        }

                        game_time::tick();

                        return;
                } else {
                        // In backpack inventory
                        const size_t backpack_idx =
                                m_browser.y() - (int)SlotId::END;

                        // Exit screen
                        states::pop();

                        auto* item = inv.m_backpack[backpack_idx];

                        const auto& data = item->data();

                        // TODO: Also allow equipping body and head items by
                        // selecting them

                        if ((data.type == ItemType::melee_wpn) ||
                            (data.type == ItemType::ranged_wpn) ||
                            (data.type == ItemType::armor) ||
                            (data.type == ItemType::head_wear)) {
                                on_equipable_backpack_item_selected(
                                        backpack_idx);
                        } else {
                                activate(backpack_idx);
                        }

                        return;
                }
        } break;

        case MenuAction::esc:
        case MenuAction::space: {
                // Exit screen
                states::pop();

                return;
        } break;

        default:
                break;
        }
}

void BrowseInv::on_body_slot_item_selected() const
{
        if (map::g_player->m_properties.has(PropId::burning)) {
                msg_log::add("Not while burning.");

                return;
        }

        if (map::g_player->m_properties.has(PropId::swimming)) {
                msg_log::add("Not while swimming.");

                return;
        }

        map::g_player->m_remove_armor_countdown = s_nr_turns_to_handle_armor;
}

void BrowseInv::on_equipable_backpack_item_selected(
        const size_t backpack_idx) const
{
        auto& inv = map::g_player->m_inv;
        auto* const item_to_equip = inv.m_backpack[backpack_idx];
        const auto item_type = item_to_equip->data().type;

        switch (item_type) {
        case ItemType::melee_wpn:
        case ItemType::ranged_wpn: {
                if (inv.has_item_in_slot(SlotId::wpn)) {
                        inv.unequip_slot(SlotId::wpn);

                        map::g_player->m_item_equipping = item_to_equip;
                } else {
                        inv.equip_backpack_item(backpack_idx, SlotId::wpn);
                }
        } break;

        case ItemType::head_wear: {
                if (inv.has_item_in_slot(SlotId::head)) {
                        inv.unequip_slot(SlotId::head);

                        map::g_player->m_item_equipping = item_to_equip;
                } else {
                        inv.equip_backpack_item(backpack_idx, SlotId::head);
                }
        } break;

        case ItemType::armor: {
                if (map::g_player->m_properties.has(PropId::burning)) {
                        msg_log::add("Not while burning.");

                        return;
                }

                if (map::g_player->m_properties.has(PropId::swimming)) {
                        msg_log::add("Not while swimming.");

                        return;
                }

                if (inv.has_item_in_slot(SlotId::body)) {
                        map::g_player->m_remove_armor_countdown =
                                s_nr_turns_to_handle_armor;
                }

                map::g_player->m_item_equipping = item_to_equip;

                map::g_player->m_equip_armor_countdown =
                        s_nr_turns_to_handle_armor;
        } break;

        default:
        {
                ASSERT(false);
        } break;
        }

        game_time::tick();
}

// -----------------------------------------------------------------------------
// Apply item state
// -----------------------------------------------------------------------------
void Apply::on_start()
{
        map::g_player->m_inv.sort_backpack();

        auto& backpack = map::g_player->m_inv.m_backpack;

        m_filtered_backpack_indexes.clear();

        m_filtered_backpack_indexes.reserve(backpack.size());

        for (size_t i = 0; i < backpack.size(); ++i) {
                const auto* const item = backpack[i];

                const auto& d = item->data();

                if (d.has_std_activate) {
                        m_filtered_backpack_indexes.push_back(i);
                }
        }

        if (m_filtered_backpack_indexes.empty()) {
                // Exit screen
                states::pop();

                msg_log::add("I carry nothing to apply.");

                return;
        }

        m_browser.reset(
                m_filtered_backpack_indexes.size(),
                panels::h(Panel::item_menu));

        m_browser.disable_selection_audio();

        audio::play(audio::SfxId::backpack);
}

void Apply::draw()
{
        // Only draw this state if it's the current state, so that messages
        // can be drawn on the map
        if (!states::is_current_state(*this)) {
                return;
        }

        io::clear_screen();

        draw_box(panels::area(Panel::screen));

        const int browser_y = m_browser.y();

        io::draw_text_center(
                " Apply which item? " + common_text::g_screen_exit_hint + " ",
                Panel::screen,
                P(panels::center_x(Panel::screen), 0),
                colors::title());

        const Range idx_range_shown = m_browser.range_shown();

        int y = 0;

        for (int i = idx_range_shown.min; i <= idx_range_shown.max; ++i) {
                const char key = m_browser.menu_keys()[y];

                const bool is_marked = browser_y == i;

                const size_t backpack_idx = m_filtered_backpack_indexes[i];

                draw_backpack_item(
                        backpack_idx,
                        y,
                        key,
                        is_marked,
                        ItemRefAttInf::wpn_main_att_mode);

                ++y;
        }

        // Draw "more" labels
        if (!m_browser.is_on_top_page()) {
                io::draw_text(
                        common_text::g_next_page_up_hint,
                        Panel::item_menu,
                        P(0, -1),
                        colors::light_white());
        }

        if (!m_browser.is_on_btm_page()) {
                io::draw_text(
                        common_text::g_next_page_down_hint,
                        Panel::item_menu,
                        P(0, panels::h(Panel::item_menu)),
                        colors::light_white());
        }
}

void Apply::update()
{
        auto input = io::get();

        const MenuAction action =
                m_browser.read(input, MenuInputMode::scrolling_and_letters);

        switch (action) {
        case MenuAction::selected: {
                if (!m_filtered_backpack_indexes.empty()) {
                        const size_t backpack_idx =
                                m_filtered_backpack_indexes[m_browser.y()];

                        // Exit screen
                        states::pop();

                        activate(backpack_idx);

                        return;
                }
        } break;

        case MenuAction::esc:
        case MenuAction::space: {
                // Exit screen
                states::pop();
                return;
        } break;

        default:
                break;
        }
}

// -----------------------------------------------------------------------------
// Drop item state
// -----------------------------------------------------------------------------
void Drop::on_start()
{
        map::g_player->m_inv.sort_backpack();

        const int list_size =
                (int)SlotId::END +
                (int)map::g_player->m_inv.m_backpack.size();

        m_browser.reset(
                list_size,
                panels::h(Panel::item_menu));

        m_browser.disable_selection_audio();

        audio::play(audio::SfxId::backpack);
}

void Drop::draw()
{
        io::clear_screen();

        draw_box(panels::area(Panel::screen));

        io::draw_text_center(
                " Drop which item? " + common_text::g_screen_exit_hint + " ",
                Panel::screen,
                P(panels::center_x(Panel::screen), 0),
                colors::title());

        const int browser_y = m_browser.y();

        const auto idx_range_shown = m_browser.range_shown();

        int y = 0;

        for (int i = idx_range_shown.min; i <= idx_range_shown.max; ++i) {
                const char key = m_browser.menu_keys()[y];

                const bool is_marked = browser_y == i;

                if (i < (int)SlotId::END) {
                        const auto slot_id = (SlotId)i;

                        draw_slot(
                                slot_id,
                                y,
                                key,
                                is_marked,
                                ItemRefAttInf::wpn_main_att_mode);
                } else {
                        // This index is in backpack
                        const auto backpack_idx =
                                (size_t)(i - (int)SlotId::END);

                        draw_backpack_item(
                                backpack_idx,
                                y,
                                key,
                                is_marked,
                                ItemRefAttInf::wpn_main_att_mode);
                }

                ++y;
        }

        // Draw "more" labels
        if (!m_browser.is_on_top_page()) {
                io::draw_text(
                        common_text::g_next_page_up_hint,
                        Panel::item_menu,
                        P(0, -1),
                        colors::light_white());
        }

        if (!m_browser.is_on_btm_page()) {
                io::draw_text(
                        common_text::g_next_page_down_hint,
                        Panel::item_menu,
                        P(0, panels::h(Panel::item_menu)),
                        colors::light_white());
        }
}

void Drop::update()
{
        const auto input = io::get();

        const MenuAction action =
                m_browser.read(input, MenuInputMode::scrolling_and_letters);

        switch (action) {
        case MenuAction::selected: {
                const int browser_y = m_browser.y();

                const auto inv_type_marked =
                        (m_browser.y() < (int)SlotId::END)
                        ? InvType::slots
                        : InvType::backpack;

                auto idx = (size_t)browser_y;

                item::Item* item = nullptr;

                if (inv_type_marked == InvType::slots) {
                        if (!map::g_player->m_inv.has_item_in_slot((SlotId)idx)) {
                                return;
                        }

                        item = map::g_player->m_inv.m_slots[idx].item;
                } else {
                        // Backpack item marked
                        idx -= (size_t)SlotId::END;

                        item = map::g_player->m_inv.m_backpack[idx];
                }

                ASSERT(item);

                // Exit screen
                states::pop();

                if (item->current_curse().is_active()) {
                        const auto name =
                                item->name(
                                        ItemRefType::plain,
                                        ItemRefInf::none,
                                        ItemRefAttInf::none);

                        msg_log::add("I refuse to drop the " + name + "!");

                        return;
                }

                if ((inv_type_marked == InvType::slots) &&
                    (idx == (size_t)SlotId::body)) {
                        // Body slot marked, start dropping the armor
                        map::g_player->m_remove_armor_countdown =
                                s_nr_turns_to_handle_armor;

                        map::g_player
                                ->m_is_dropping_armor_from_body_slot = true;

                        game_time::tick();
                } else {
                        // Not dropping from body slot, drop immediately
                        const bool did_drop =
                                run_drop_query(*item, inv_type_marked, idx);

                        if (did_drop) {
                                game_time::tick();
                        }
                }

                return;
        } break;

        case MenuAction::esc:
        case MenuAction::space: {
                // Exit screen
                states::pop();

                return;
        } break;

        default:
                break;
        }
}

// -----------------------------------------------------------------------------
// Equip state
// -----------------------------------------------------------------------------
void Equip::on_start()
{
        map::g_player->m_inv.sort_backpack();

        // Filter backpack
        const auto& backpack = map::g_player->m_inv.m_backpack;

        m_filtered_backpack_indexes.clear();

        for (size_t i = 0; i < backpack.size(); ++i) {
                const auto* const item = backpack[i];
                const auto& data = item->data();

                switch (m_slot_to_equip.id) {
                case SlotId::wpn:
                        if ((data.melee.is_melee_wpn) ||
                            (data.ranged.is_ranged_wpn)) {
                                m_filtered_backpack_indexes.push_back(i);
                        }
                        break;

                case SlotId::wpn_alt:
                        if ((data.melee.is_melee_wpn) ||
                            (data.ranged.is_ranged_wpn)) {
                                m_filtered_backpack_indexes.push_back(i);
                        }
                        break;

                case SlotId::body:
                        if (data.type == ItemType::armor) {
                                m_filtered_backpack_indexes.push_back(i);
                        }
                        break;

                case SlotId::head:
                        if (data.type == ItemType::head_wear) {
                                m_filtered_backpack_indexes.push_back(i);
                        }
                        break;

                case SlotId::END:
                        break;
                }
        }

        m_browser.reset(
                m_filtered_backpack_indexes.size(),
                panels::h(Panel::item_menu));

        m_browser.disable_selection_audio();

        m_browser.set_y(0);
}

void Equip::draw()
{
        draw_box(panels::area(Panel::screen));

        const bool has_item = !m_filtered_backpack_indexes.empty();

        std::string heading;

        switch (m_slot_to_equip.id) {
        case SlotId::wpn:
                heading =
                        has_item
                        ? "Wield which item?"
                        : "I carry no weapon to wield.";
                break;

        case SlotId::wpn_alt:
                heading =
                        has_item
                        ? "Prepare which weapon?"
                        : "I carry no weapon to wield.";
                break;

        case SlotId::body:
                heading =
                        has_item
                        ? "Wear which armor?"
                        : "I carry no armor.";
                break;

        case SlotId::head:
                heading =
                        has_item
                        ? "Wear what on head?"
                        : "I carry no headwear.";
                break;

        case SlotId::END:
                break;
        }

        if (!has_item) {
                io::draw_text(
                        " " + heading + " " + common_text::g_any_key_hint + " ",
                        Panel::screen,
                        P(0, 0),
                        colors::light_white());

                return;
        }

        // An item is available

        io::draw_text_center(
                " " + heading + " " + common_text::g_screen_exit_hint + " ",
                Panel::screen,
                P(panels::center_x(Panel::screen), 0),
                colors::title());

        const int browser_y = m_browser.y();

        const Range idx_range_shown = m_browser.range_shown();

        int y = 0;

        for (int i = idx_range_shown.min; i <= idx_range_shown.max; ++i) {
                const char key = m_browser.menu_keys()[y];

                const bool is_marked = browser_y == i;

                const size_t backpack_idx =
                        m_filtered_backpack_indexes[i];

                auto* const item =
                        map::g_player->m_inv.m_backpack[backpack_idx];

                const auto& d = item->data();

                ItemRefAttInf att_inf = ItemRefAttInf::none;

                if ((m_slot_to_equip.id == SlotId::wpn) ||
                    (m_slot_to_equip.id == SlotId::wpn_alt)) {
                        // Thrown weapons are forced to show melee info instead
                        att_inf =
                                (d.main_att_mode == AttMode::thrown)
                                ? ItemRefAttInf::melee
                                : ItemRefAttInf::wpn_main_att_mode;
                }

                draw_backpack_item(
                        backpack_idx,
                        y,
                        key,
                        is_marked,
                        att_inf);

                ++y;
        }

        // Draw "more" labels
        if (!m_browser.is_on_top_page()) {
                io::draw_text(
                        common_text::g_next_page_up_hint,
                        Panel::item_menu,
                        P(0, -1),
                        colors::light_white());
        }

        if (!m_browser.is_on_btm_page()) {
                io::draw_text(
                        common_text::g_next_page_down_hint,
                        Panel::item_menu,
                        P(0, panels::h(Panel::item_menu)),
                        colors::light_white());
        }
}

void Equip::update()
{
        const auto input = io::get();

        if (m_filtered_backpack_indexes.empty() ||
            (input.key == SDLK_SPACE) ||
            (input.key == SDLK_ESCAPE)) {
                // Leave screen, and go back to inventory
                states::pop();

                return;
        }

        const MenuAction action =
                m_browser.read(input, MenuInputMode::scrolling_and_letters);

        switch (action) {
        case MenuAction::selected: {
                const size_t idx = m_filtered_backpack_indexes[m_browser.y()];

                const auto slot_id = m_slot_to_equip.id;

                states::pop_until(StateId::game);

                if (slot_id == SlotId::body) {
                        if (map::g_player->m_properties.has(PropId::burning)) {
                                msg_log::add("Not while burning.");

                                return;
                        }

                        if (map::g_player->m_properties.has(PropId::swimming)) {
                                msg_log::add("Not while swimming.");

                                return;
                        }

                        // Start putting on armor
                        map::g_player->m_equip_armor_countdown =
                                s_nr_turns_to_handle_armor;

                        map::g_player->m_item_equipping =
                                map::g_player->m_inv.m_backpack[idx];
                } else {
                        // Not the body slot, equip the item immediately
                        map::g_player->m_inv.equip_backpack_item(idx, slot_id);
                }

                game_time::tick();

                return;
        } break;

        default:
                break;
        }
}

// -----------------------------------------------------------------------------
// Select throwing state
// -----------------------------------------------------------------------------
void SelectThrow::on_start()
{
        map::g_player->m_inv.sort_backpack();

        // Filter slots
        for (InvSlot& slot : map::g_player->m_inv.m_slots) {
                const auto* const item = slot.item;

                if (item) {
                        const auto& d = item->data();

                        if (d.ranged.is_throwable_wpn) {
                                FilteredInvEntry entry;
                                entry.relative_idx = (size_t)slot.id;
                                entry.is_slot = true;

                                m_filtered_inv.push_back(entry);
                        }
                }
        }

        // Filter backpack
        const auto& backpack = map::g_player->m_inv.m_backpack;

        for (size_t i = 0; i < backpack.size(); ++i) {
                const auto* const item = backpack[i];

                const auto& d = item->data();

                if (d.ranged.is_throwable_wpn) {
                        FilteredInvEntry entry;
                        entry.relative_idx = i;
                        entry.is_slot = false;

                        if (item == map::g_player->m_last_thrown_item) {
                                // Last thrown item - show it at the top
                                m_filtered_inv.insert(
                                        std::begin(m_filtered_inv),
                                        entry);
                        } else {
                                // Not the last thrown item - append to bottom
                                m_filtered_inv.push_back(entry);
                        }
                }
        }

        const auto list_size = m_filtered_inv.size();

        if (list_size == 0) {
                // Nothing to throw, exit screen
                states::pop();

                msg_log::add("I carry no throwing weapons.");

                return;
        }

        m_browser.reset(
                list_size,
                panels::h(Panel::item_menu));

        m_browser.disable_selection_audio();

        // Set up custom menu keys - a specific key is always reserved for the
        // last thrown item (if any), and never used for throwing any other item
        auto custom_keys = m_browser.menu_keys();

        const char throw_key = 't';

        {
                const auto it =
                        std::find(
                                std::begin(custom_keys),
                                std::end(custom_keys),
                                throw_key);

                if (it != std::end(custom_keys)) {
                        custom_keys.erase(it);
                }
        }

        if (map::g_player->m_last_thrown_item) {
                custom_keys.insert(std::begin(custom_keys), throw_key);
        }

        m_browser.set_custom_menu_keys(custom_keys);

        audio::play(audio::SfxId::backpack);
}

void SelectThrow::draw()
{
        draw_box(panels::area(Panel::screen));

        io::draw_text_center(
                std::string(
                        " Throw which item? " +
                        common_text::g_screen_exit_hint) +
                        " ",
                Panel::screen,
                P(panels::center_x(Panel::screen), 0),
                colors::title());

        const int browser_y = m_browser.y();

        const Range idx_range_shown = m_browser.range_shown();

        int y = 0;

        for (int i = idx_range_shown.min; i <= idx_range_shown.max; ++i) {
                const char key = m_browser.menu_keys()[y];

                const bool is_marked = browser_y == i;

                const auto& inv_entry = m_filtered_inv[i];

                if (inv_entry.is_slot) {
                        const auto slot_id = (SlotId)inv_entry.relative_idx;

                        draw_slot(
                                slot_id,
                                y,
                                key,
                                is_marked,
                                ItemRefAttInf::thrown);
                } else {
                        // This index is in backpack
                        const size_t backpack_idx = inv_entry.relative_idx;

                        draw_backpack_item(
                                backpack_idx,
                                y,
                                key,
                                is_marked,
                                ItemRefAttInf::thrown);
                }

                ++y;
        }

        // Draw "more" labels
        if (!m_browser.is_on_top_page()) {
                io::draw_text(
                        common_text::g_next_page_up_hint,
                        Panel::item_menu,
                        P(0, -1),
                        colors::light_white());
        }

        if (!m_browser.is_on_btm_page()) {
                io::draw_text(
                        common_text::g_next_page_down_hint,
                        Panel::item_menu,
                        P(0, panels::h(Panel::item_menu)),
                        colors::light_white());
        }
}

void SelectThrow::update()
{
        const auto input = io::get();

        const MenuAction action =
                m_browser.read(input, MenuInputMode::scrolling_and_letters);

        const auto& inv_entry_marked = m_filtered_inv[m_browser.y()];

        const auto& inv = map::g_player->m_inv;

        item::Item* item = nullptr;

        if (inv_entry_marked.is_slot) {
                item = inv.m_slots[inv_entry_marked.relative_idx].item;
        } else {
                // Backpack item selected
                item = inv.m_backpack[inv_entry_marked.relative_idx];
        }

        ASSERT(item);

        switch (action) {
        case MenuAction::selected: {
                states::pop();

                if (item->current_curse().is_active()) {
                        const auto name =
                                item->name(
                                        ItemRefType::plain,
                                        ItemRefInf::none,
                                        ItemRefAttInf::none);

                        msg_log::add("I refuse to throw the " + name + "!");

                        return;
                }

                states::push(
                        std::make_unique<Throwing>(
                                map::g_player->m_pos,
                                *item));

                return;
        } break;

        case MenuAction::esc:
        case MenuAction::space: {
                // Exit screen
                states::pop();

                return;
        } break;

        default:
                break;
        }
}

// -----------------------------------------------------------------------------
// Select identify state
// -----------------------------------------------------------------------------
void SelectIdentify::on_start()
{
        map::g_player->m_inv.sort_backpack();

        auto is_allowed_item_type =
                [](const ItemType item_type,
                   const std::vector<ItemType>& allowed_item_types) {
                        if (allowed_item_types.empty()) {
                                return true;
                        } else {
                                const auto result = std::find(
                                        begin(allowed_item_types),
                                        end(allowed_item_types),
                                        item_type);

                                return result != end(allowed_item_types);
                        }
                };

        // Filter slots
        for (InvSlot& slot : map::g_player->m_inv.m_slots) {
                const auto* const item = slot.item;

                if (!item) {
                        continue;
                }

                const auto& d = item->data();

                if (!d.is_identified &&
                    is_allowed_item_type(d.type, m_item_types_allowed)) {
                        FilteredInvEntry entry;
                        entry.relative_idx = (size_t)slot.id;
                        entry.is_slot = true;

                        m_filtered_inv.push_back(entry);
                }
        }

        // Filter backpack
        for (size_t i = 0; i < map::g_player->m_inv.m_backpack.size(); ++i) {
                const auto* const item = map::g_player->m_inv.m_backpack[i];

                const auto& d = item->data();

                if (!d.is_identified &&
                    is_allowed_item_type(d.type, m_item_types_allowed)) {
                        FilteredInvEntry entry;
                        entry.relative_idx = i;
                        entry.is_slot = false;

                        m_filtered_inv.push_back(entry);
                }
        }

        const auto list_size = m_filtered_inv.size();

        if (list_size == 0) {
                // Nothing to identify, exit screen
                states::pop();

                msg_log::add("There is nothing to identify.");

                return;
        }

        m_browser.reset(
                list_size,
                panels::h(Panel::item_menu));

        m_browser.disable_selection_audio();

        audio::play(audio::SfxId::backpack);
}

void SelectIdentify::draw()
{
        io::clear_screen();

        draw_box(panels::area(Panel::screen));

        const int browser_y = m_browser.y();

        io::draw_text_center(
                " Identify which item? ",
                Panel::screen,
                P(panels::center_x(Panel::screen), 0),
                colors::title());

        const Range idx_range_shown = m_browser.range_shown();

        int y = 0;

        for (int i = idx_range_shown.min; i <= idx_range_shown.max; ++i) {
                const char key = m_browser.menu_keys()[y];

                const bool is_marked = browser_y == i;

                const auto& inv_entry = m_filtered_inv[i];

                if (inv_entry.is_slot) {
                        const auto slot_id = (SlotId)inv_entry.relative_idx;

                        draw_slot(
                                slot_id,
                                y,
                                key,
                                is_marked,
                                ItemRefAttInf::wpn_main_att_mode);
                } else {
                        // This index is in backpack
                        const size_t backpack_idx = inv_entry.relative_idx;

                        draw_backpack_item(
                                backpack_idx,
                                y,
                                key,
                                is_marked,
                                ItemRefAttInf::wpn_main_att_mode);
                }

                ++y;
        }

        // Draw "more" labels
        if (!m_browser.is_on_top_page()) {
                io::draw_text(
                        common_text::g_next_page_up_hint,
                        Panel::item_menu,
                        P(0, -1),
                        colors::light_white());
        }

        if (!m_browser.is_on_btm_page()) {
                io::draw_text(
                        common_text::g_next_page_down_hint,
                        Panel::item_menu,
                        P(0, panels::h(Panel::item_menu)),
                        colors::light_white());
        }
}

void SelectIdentify::update()
{
        const auto input = io::get();

        const MenuAction action =
                m_browser.read(
                        input,
                        MenuInputMode::scrolling_and_letters);

        const auto& inv = map::g_player->m_inv;

        switch (action) {
        case MenuAction::selected: {
                const auto& inv_entry_marked = m_filtered_inv[m_browser.y()];

                item::Item* item_to_identify;

                if (inv_entry_marked.is_slot) {
                        item_to_identify =
                                inv.m_slots[inv_entry_marked.relative_idx].item;
                } else {
                        // Backpack item selected
                        item_to_identify =
                                inv.m_backpack[inv_entry_marked.relative_idx];
                }

                // Exit screen
                states::pop();

                io::clear_screen();

                map::update_vision();

                // Identify item
                item_to_identify->identify(Verbose::yes);

                return;
        } break;

        default:
                break;

        } // action switch
}
