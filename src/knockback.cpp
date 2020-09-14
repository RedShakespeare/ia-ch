// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "knockback.hpp"

#include <algorithm>
#include <vector>

#include "actor_death.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "actor_see.hpp"
#include "attack.hpp"
#include "config.hpp"
#include "game_time.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "terrain.hpp"
#include "terrain_mob.hpp"
#include "terrain_trap.hpp"
#include "text_format.hpp"

namespace knockback
{
void run(
        actor::Actor& defender,
        const P& attacked_from_pos,
        const bool is_spike_gun,
        const Verbose verbose,
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

        if (map::first_actor_at_pos(new_pos, ActorState::alive))
        {
                // Target position is occupied by another actor
                return;
        }

        const auto defender_can_move_into_tgt_pos =
                !map_parsers::BlocksActor(defender, ParseActors::no)
                         .cell(new_pos);

        const std::vector<terrain::Id> deep_terrains = {
                terrain::Id::chasm,
                terrain::Id::liquid_deep};

        const bool is_tgt_pos_deep =
                map_parsers::IsAnyOfTerrains(deep_terrains)
                        .cell(new_pos);

        auto& tgt_cell = map::g_cells.at(new_pos);

        if (!defender_can_move_into_tgt_pos && !is_tgt_pos_deep)
        {
                // Defender nailed to a wall from a spike gun?
                if (is_spike_gun)
                {
                        if (!tgt_cell.terrain->is_projectile_passable())
                        {
                                auto* prop = new PropNailed();

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
                : actor::can_player_see_actor(defender);

        bool player_is_aware_of_defender = true;

        if (!is_defender_player)
        {
                player_is_aware_of_defender = defender.is_player_aware_of_me();
        }

        std::string defender_name =
                player_can_see_defender
                ? text_format::first_to_upper(defender.name_the())
                : "It";

        if ((verbose == Verbose::yes) && player_is_aware_of_defender)
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

        auto* prop = new PropParalyzed();

        prop->set_duration(1 + paralyze_extra_turns);

        defender.m_properties.apply(prop);

        // Leave current cell
        map::g_cells.at(defender.m_pos).terrain->on_leave(defender);

        defender.m_pos = new_pos;

        if (!defender_can_move_into_tgt_pos && is_tgt_pos_deep)
        {
                if (is_defender_player)
                {
                        msg_log::add(
                                "I perish in the depths!",
                                colors::msg_bad());
                }
                else if (
                        player_is_aware_of_defender &&
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

}  // namespace knockback
