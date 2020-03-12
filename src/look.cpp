// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "look.hpp"

#include <climits>
#include <string>

#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "attack_data.hpp"
#include "game_time.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "item.hpp"
#include "map.hpp"
#include "marker.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "query.hpp"
#include "terrain.hpp"
#include "terrain_mob.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// private
// -----------------------------------------------------------------------------
static int s_mon_descr_x0 = 1;


static int mon_descr_max_w()
{
        return panels::w(Panel::screen) - 2;
}

static std::string get_mon_memory_turns_descr(const actor::Actor& actor)
{
        const int nr_turns_aware = actor.m_data->nr_turns_aware;

        if (nr_turns_aware <= 0)
        {
                return "";
        }

        const std::string name_a = text_format::first_to_upper(actor.name_a());

        if (nr_turns_aware < 50)
        {
                const std::string nr_turns_aware_str =
                        std::to_string(nr_turns_aware);

                return
                        name_a +
                        " will remember hostile creatures for at least " +
                        nr_turns_aware_str +
                        " turns.";
        }
        else // Very high number of turns awareness
        {
                return
                        name_a +
                        " remembers hostile creatures for a very long time.";
        }
}

static std::string get_mon_dlvl_descr(const actor::Actor& actor)
{
        const auto& d = *actor.m_data;

        const int dlvl = d.spawn_min_dlvl;

        if ((dlvl <= 1) || (dlvl >= g_dlvl_last))
        {
                return "";
        }

        const std::string dlvl_str = std::to_string(dlvl);

        if (d.is_unique)
        {
                return
                        d.name_the +
                        " usually dwells beneath level " +
                        dlvl_str +
                        ".";
        }
        else // Not unique
        {
                return
                        "They usually dwell beneath level " +
                        dlvl_str +
                        ".";
        }
}

static std::string mon_speed_type_to_str(const actor::Actor& actor)
{
        switch (actor.m_data->speed)
        {
        case actor::Speed::slow:
                return "slowly";

        case actor::Speed::normal:
                return "";

        case actor::Speed::fast:
                return "fast";

        case actor::Speed::very_fast:
                return "very swiftly";
        }

        ASSERT(false);

        return "";
}

static std::string get_mon_speed_descr(const actor::Actor& actor)
{
        const auto& d = *actor.m_data;

        const std::string speed_type_str = mon_speed_type_to_str(actor);

        if (speed_type_str.empty())
        {
                return "";;
        }

        if (d.is_unique)
        {
                return
                        d.name_the +
                        " appears to move " +
                        speed_type_str +
                        ".";
        }
        else // Not unique
        {
                return
                        "They appear to move " +
                        speed_type_str +
                        ".";
        }
}

static void mon_shock_lvl_to_str(
        const actor::Actor& actor,
        std::string& shock_str_out,
        std::string& punct_str_out)
{
        shock_str_out = "";
        punct_str_out = "";

        switch (actor.m_data->mon_shock_lvl)
        {
        case ShockLvl::unsettling:
                shock_str_out = "unsettling";
                punct_str_out = ".";
                break;

        case ShockLvl::frightening:
                shock_str_out = "frightening";
                punct_str_out = ".";
                break;

        case ShockLvl::terrifying:
                shock_str_out = "terrifying";
                punct_str_out = "!";
                break;

        case ShockLvl::mind_shattering:
                shock_str_out = "mind shattering";
                punct_str_out = "!";
                break;

        case ShockLvl::none:
        case ShockLvl::END:
                break;
        }
}

static std::string get_mon_shock_descr(const actor::Actor& actor)
{
        std::string shock_str;

        std::string shock_punct_str;

        mon_shock_lvl_to_str(actor, shock_str, shock_punct_str);

        if (shock_str.empty())
        {
                return "";
        }

        if (actor.m_data->is_unique)
        {
                return
                        actor.name_the() +
                        " is " +
                        shock_str +
                        " to behold" +
                        shock_punct_str;
        }
        else // Not unique
        {
                return
                        "They are " +
                        shock_str +
                        " to behold" +
                        shock_punct_str;
        }
}

static std::string get_melee_hit_chance_descr(actor::Actor& actor)
{
        const auto* wielded_item =
                map::g_player->m_inv.item_in_slot(SlotId::wpn);

        const auto* const player_wpn =
                wielded_item
                ? static_cast<const item::Wpn*>(wielded_item)
                : &map::g_player->unarmed_wpn();

        if (!player_wpn)
        {
                ASSERT(false);

                return "";
        }

        const MeleeAttData att_data(map::g_player, actor, *player_wpn);

        const int hit_chance =
                ability_roll::hit_chance_pct_actual(
                        att_data.hit_chance_tot);

        return
                "The chance to hit " +
                actor.name_the() +
                " in melee combat is currently " +
                std::to_string(hit_chance) +
                "%.";
}

// -----------------------------------------------------------------------------
// View actor description
// -----------------------------------------------------------------------------
StateId ViewActorDescr::id()
{
        return StateId::view_actor;
}

void ViewActorDescr::on_start()
{
        // Fixed decription
        const auto fixed_descr = m_actor.descr();

        {
                const auto fixed_lines =
                        text_format::split(
                                fixed_descr,
                                mon_descr_max_w());

                for (const auto& line : fixed_lines)
                {
                        m_lines.emplace_back(
                                        line,
                                        colors::text());
                }
        }

        // Auto description
        {
                const auto auto_descr =
                        m_actor.m_data->allow_generated_descr
                        ? auto_description_str()
                        : "";

                if (!auto_descr.empty())
                {
                        m_lines.resize(m_lines.size() + 1);

                        const auto auto_descr_lines =
                                text_format::split(
                                        auto_descr,
                                        mon_descr_max_w());

                        for (const auto& line : auto_descr_lines)
                        {
                                m_lines.emplace_back(
                                                line,
                                                colors::text());
                        }
                }
        }

        // Add the full description
        m_lines.resize(m_lines.size() + 1);

        m_lines.emplace_back("Current properties", colors::text());

        auto prop_list =
                m_actor.m_properties
                .property_names_temporary_negative();

        // Remove all non-negative properties (we should not show temporary
        // spell resistance for example), and all natural properties (properties
        // which all monsters of this type starts with)
        for (auto it = begin(prop_list); it != end(prop_list);)
        {
                auto* const prop = it->prop;

                const auto id = prop->id();

                const bool is_natural_prop =
                        m_actor.m_data->natural_props[(size_t)id];

                if (is_natural_prop ||
                    (prop->duration_mode() == PropDurationMode::indefinite) ||
                    (prop->alignment() != PropAlignment::bad))
                {
                        it = prop_list.erase(it);
                }
                else
                {
                        // Not a natural property
                        ++it;
                }
        }

        const std::string offset = "   ";

        if (prop_list.empty())
        {
                m_lines.emplace_back(
                        offset + "None",
                        colors::text());

                // End with an empty line
                m_lines.emplace_back("", colors::text());
        }
        else
        {
                // Has properties
                const int max_w_descr = (panels::x1(Panel::screen) * 3) / 4;

                for (const auto& e : prop_list)
                {
                        const auto& title = e.title;

                        m_lines.emplace_back(offset + title.str, e.title.color);

                        const auto descr_formatted =
                                text_format::split(
                                        e.descr,
                                        max_w_descr);

                        for (const auto& descr_line : descr_formatted)
                        {
                                m_lines.emplace_back(
                                                offset + descr_line,
                                                colors::gray());
                        }

                        // Add an empty line between each property, and also
                        // after the last one
                        m_lines.emplace_back("", colors::text());
                }
        }
}

void ViewActorDescr::draw()
{
        io::cover_panel(Panel::screen);

        draw_interface();

        const int nr_lines_tot = m_lines.size();

        int btm_nr = std::min(
                m_top_idx + max_nr_lines_on_screen() - 1,
                nr_lines_tot - 1);

        int screen_y = 1;

        for (int y = m_top_idx; y <= btm_nr; ++y)
        {
                const auto& line = m_lines[y];

                io::draw_text(
                        line.str,
                        Panel::screen,
                        P(s_mon_descr_x0, screen_y),
                        line.color);

                ++screen_y;
        }
}

void ViewActorDescr::update()
{
        const int line_jump = 3;
        const int nr_lines_tot = m_lines.size();

        const auto input = io::get();

        switch (input.key)
        {
        case SDLK_KP_2:
        case SDLK_DOWN:
                m_top_idx += line_jump;

                if (nr_lines_tot <= max_nr_lines_on_screen())
                {
                        m_top_idx = 0;
                }
                else
                {
                        m_top_idx = std::min(
                                nr_lines_tot - max_nr_lines_on_screen(),
                                m_top_idx);
                }
                break;

        case SDLK_KP_8:
        case SDLK_UP:
                m_top_idx = std::max(0, m_top_idx - line_jump);
                break;

        case SDLK_SPACE:
        case SDLK_ESCAPE:
                // Exit screen
                states::pop();
                break;

        default:
                break;
        }
}

std::string ViewActorDescr::auto_description_str() const
{
        std::string str;

        text_format::append_with_space(
                str,
                get_melee_hit_chance_descr(m_actor));

        text_format::append_with_space(
                str,
                get_mon_dlvl_descr(m_actor));

        text_format::append_with_space(
                str,
                get_mon_speed_descr(m_actor));

        text_format::append_with_space(
                str,
                get_mon_memory_turns_descr(m_actor));

        if (m_actor.m_data->is_undead)
        {
                text_format::append_with_space(str, "This creature is undead.");
        }

        text_format::append_with_space(str, get_mon_shock_descr(m_actor));

        return str;
}

std::string ViewActorDescr::title() const
{
        const std::string mon_name =
                text_format::first_to_upper(
                        m_actor.name_a());

        return mon_name;
}

// -----------------------------------------------------------------------------
// Look
// -----------------------------------------------------------------------------
namespace look
{

void print_location_info_msgs(const P& pos)
{
        Cell* cell = nullptr;

        bool is_cell_seen = false;

        if (map::is_pos_inside_map(pos))
        {
                cell = &map::g_cells.at(pos);

                is_cell_seen = cell->is_seen_by_player;
        }

        if (is_cell_seen)
        {
                // Describe terrain
                std::string str = cell->terrain->name(Article::a);

                str = text_format::first_to_upper(str);

                msg_log::add(
                        str + ".",
                        colors::text(),
                        MsgInterruptPlayer::no,
                        MorePromptOnMsg::no,
                        CopyToMsgHistory::no);

                // Describe mobile terrains
                for (auto* mob : game_time::g_mobs)
                {
                        if (mob->pos() == pos)
                        {
                                str = mob->name(Article::a);

                                str = text_format::first_to_upper(str);

                                msg_log::add(
                                        str  + ".",
                                        colors::text(),
                                        MsgInterruptPlayer::no,
                                        MorePromptOnMsg::no,
                                        CopyToMsgHistory::no);
                        }
                }

                // Describe darkness
                if (map::g_dark.at(pos) && !map::g_light.at(pos))
                {
                        msg_log::add(
                                "It is very dark here.",
                                colors::text(),
                                MsgInterruptPlayer::no,
                                MorePromptOnMsg::no,
                                CopyToMsgHistory::no);
                }

                // Describe item
                auto* item = cell->item;

                if (item)
                {
                        str = item->name(
                                ItemRefType::plural,
                                ItemRefInf::yes,
                                ItemRefAttInf::wpn_main_att_mode);

                        str = text_format::first_to_upper(str);

                        msg_log::add(
                                str + ".",
                                colors::text(),
                                MsgInterruptPlayer::no,
                                MorePromptOnMsg::no,
                                CopyToMsgHistory::no);
                }

                // Describe dead actors
                for (auto* actor : game_time::g_actors)
                {
                        if (actor->is_corpse() && actor->m_pos == pos)
                        {
                                ASSERT(!actor->m_data->corpse_name_a.empty());

                                str = text_format::first_to_upper(
                                        actor->m_data->corpse_name_a);

                                msg_log::add(
                                        str + ".",
                                        colors::text(),
                                        MsgInterruptPlayer::no,
                                        MorePromptOnMsg::no,
                                        CopyToMsgHistory::no);
                        }
                }

        }

        print_living_actor_info_msg(pos);

        if (!is_cell_seen)
        {
                msg_log::add(
                        "I have no vision here.",
                        colors::text(),
                        MsgInterruptPlayer::no,
                        MorePromptOnMsg::no,
                        CopyToMsgHistory::no);
        }
}

void print_living_actor_info_msg(const P& pos)
{
        auto* actor = map::first_actor_at_pos(pos);

        if (!actor ||
            actor->is_player() ||
            !actor->is_alive())
        {
                return;
        }

        if (map::g_player->can_see_actor(*actor))
        {
                const std::string str =
                        text_format::first_to_upper(
                                actor->name_a());

                msg_log::add(
                        str + ".",
                        colors::text(),
                        MsgInterruptPlayer::no,
                        MorePromptOnMsg::no,
                        CopyToMsgHistory::no);
        }
        else // Cannot see actor
        {
                const auto* const mon = static_cast<actor::Mon*>(actor);

                if (mon->m_player_aware_of_me_counter > 0)
                {
                        msg_log::add(
                                "There is a creature here.",
                                colors::text(),
                                MsgInterruptPlayer::no,
                                MorePromptOnMsg::no,
                                CopyToMsgHistory::no);
                }
        }
}

} // namespace look
