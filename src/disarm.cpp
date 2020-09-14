// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "disarm.hpp"

#include "actor_player.hpp"
#include "actor_see.hpp"
#include "common_text.hpp"
#include "game_time.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "property_factory.hpp"
#include "query.hpp"
#include "terrain_trap.hpp"

namespace disarm
{
void player_disarm()
{
        if (!map::g_player->m_properties.allow_see())
        {
                msg_log::add("Not while blind.");

                return;
        }

        if (map::g_player->m_properties.has(PropId::entangled))
        {
                msg_log::add("Not while entangled.");

                return;
        }

        msg_log::add(
                "Which direction? " + common_text::g_cancel_hint,
                colors::light_white(),
                MsgInterruptPlayer::no,
                MorePromptOnMsg::no,
                CopyToMsgHistory::no);

        const auto input_dir = query::dir(AllowCenter::yes);

        msg_log::clear();

        if (input_dir == Dir::END)
        {
                return;
        }

        const auto pos = map::g_player->m_pos + dir_utils::offset(input_dir);

        if (!map::g_cells.at(pos).is_seen_by_player)
        {
                msg_log::add("I cannot see there.");

                return;
        }

        auto* const terrain = map::g_cells.at(pos).terrain;

        terrain::Trap* trap = nullptr;

        if (terrain->id() == terrain::Id::trap)
        {
                trap = static_cast<terrain::Trap*>(terrain);
        }

        if (!trap || trap->is_hidden())
        {
                msg_log::add(
                        common_text::g_disarm_no_trap,
                        colors::text(),
                        MsgInterruptPlayer::no,
                        MorePromptOnMsg::no,
                        CopyToMsgHistory::no);

                states::draw();

                return;
        }

        // There is a known and seen trap here

        const auto* const actor_on_trap = map::first_actor_at_pos(pos);

        if (actor_on_trap && !actor_on_trap->is_player())
        {
                if (can_player_see_actor(*actor_on_trap))
                {
                        msg_log::add("It's blocked.");
                }
                else
                {
                        msg_log::add("Something is blocking it.");
                }

                return;
        }

        trap->disarm();

        game_time::tick();

}  // player_disarm

}  // namespace disarm
