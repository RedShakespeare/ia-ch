// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "knockback.hpp"

#include <algorithm>
#include <vector>

#include "actor_death.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "attack.hpp"
#include "config.hpp"
#include "terrain_mob.hpp"
#include "terrain.hpp"
#include "terrain_trap.hpp"
#include "game_time.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "sdl_base.hpp"
#include "text_format.hpp"

namespace knockback
{

void run(actor::Actor& defender,
         const P& attacked_from_pos,
         const bool is_spike_gun,
         const Verbosity verbosity,
         const int paralyze_extra_turns)
{
        TRACE_FUNC_BEGIN;

        ASSERT(paralyze_extra_turns >= 0);

        const bool is_defender_player = defender.is_player();

        if (defender.m_data->prevent_knockback ||
            (defender.m_data->actor_size >= actor::Size::giant) ||
            defender.m_properties.has(PropId::entangled) ||
            defender.m_properties.has(PropId::ethereal) ||
            defender.m_properties.has(PropId::ooze) ||
            (is_defender_player && config::is_bot_playing()))
        {
                // Defender is not knockable

                TRACE_FUNC_END;

                return;
        }

        const P d = (defender.m_pos - attacked_from_pos).signs();

        const P new_pos = defender.m_pos + d;

        const bool defender_can_move_through_cell =
                !map_parsers::BlocksActor(defender, ParseActors::yes)
                .cell(new_pos);

        const std::vector<terrain::Id> deep_terrains = {
                terrain::Id::chasm,
                terrain::Id::liquid_deep
        };

        const bool is_cell_deep =
                map_parsers::IsAnyOfTerrains(deep_terrains)
                .cell(new_pos);

        auto& tgt_cell = map::g_cells.at(new_pos);

        if (!defender_can_move_through_cell && !is_cell_deep)
        {
                // Defender nailed to a wall from a spike gun?
                if (is_spike_gun)
                {
                        // TODO: Wouldn't it be better to check if the cell is
                        // blocking projectiles?
                        if (!tgt_cell.terrain->is_los_passable())
                        {
                                auto prop = new PropNailed();

                                prop->set_indefinite();

                                defender.m_properties.apply(prop);
                        }
                }

                TRACE_FUNC_END;

                return;
        }

        // TODO: Paralyzation and "knocked back" message should probably occur
        // even if the defender is just knocked into a wall

        const bool player_can_see_defender =
                is_defender_player
                ? true
                : map::g_player->can_see_actor(defender);

        bool player_is_aware_of_defender = true;

        if (!is_defender_player)
        {
                const auto* const mon =
                        static_cast<const actor::Mon*>(&defender);

                player_is_aware_of_defender =
                        mon->m_aware_of_player_counter > 0;
        }

        std::string defender_name =
                player_can_see_defender
                ? text_format::first_to_upper(defender.name_the())
                : "It";

        if ((verbosity == Verbosity::verbose) && player_is_aware_of_defender)
        {
                if (is_defender_player)
                {
                        msg_log::add("I am knocked back!");
                }
                else
                {
                        msg_log::add(defender_name + " is knocked back!");
                }
        }

        auto prop = new PropParalyzed();

        prop->set_duration(1 + paralyze_extra_turns);

        defender.m_properties.apply(prop);

        // Leave current cell
        map::g_cells.at(defender.m_pos).terrain->on_leave(defender);

        defender.m_pos = new_pos;

        if (is_cell_deep && !defender_can_move_through_cell)
        {
                if (is_defender_player)
                {
                        msg_log::add(
                                "I perish in the depths!",
                                colors::msg_bad());
                }
                else if (player_is_aware_of_defender &&
                         tgt_cell.is_seen_by_player)
                {
                        msg_log::add(
                                defender_name + " perishes in the depths.",
                                colors::msg_good());
                }

                actor::kill(
                        defender,
                        IsDestroyed::yes,
                        AllowGore::no,
                        AllowDropItems::no);

                TRACE_FUNC_END;

                return;
        }

        // Bump target cell
        const auto mobs = game_time::mobs_at_pos(defender.m_pos);

        for (auto* const mob : mobs)
        {
                mob->bump(defender);
        }

        if (!defender.is_alive())
        {
                TRACE_FUNC_END;
                return;
        }

        map::g_cells.at(defender.m_pos).terrain->bump(defender);

        TRACE_FUNC_END;
}

} // knockback
