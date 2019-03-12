// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "catch.hpp"

#include "actor_factory.hpp"
#include "actor_mon.hpp"
#include "actor_move.hpp"
#include "map.hpp"
#include "pos.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "terrain.hpp"
#include "terrain_trap.hpp"
#include "test_utils.hpp"

TEST_CASE("Spider web")
{
        // Test that a monster can get stuck in a spider web, and that they can
        // break free

        const P pos_l(5, 7);
        const P pos_r(6, 7);

        // TODO: Is getting stuck deterministic now? Perhaps there is no need to
        // run this in a loop?

        bool tested_stuck = false;
        bool tested_unstuck = false;

        while (!(tested_stuck && tested_unstuck))
        {
                test_utils::init_all();

                map::put(new terrain::Floor(pos_l));

                map::put(
                        new terrain::Trap(
                                pos_r,
                                new terrain::Floor(pos_r),
                                terrain::TrapId::web));

                auto* const actor =
                        actor::make(actor::Id::zombie, pos_l);

                auto* const mon = static_cast<actor::Mon*>(actor);

                // Awareness > 0 required for triggering trap
                mon->m_aware_of_player_counter = 42;

                // Move the monster into the trap, and back again
                mon->m_pos = pos_l;
                actor::move(*mon, Dir::right);

                // It should never be possible to move on the first try
                REQUIRE(mon->m_pos == pos_r);

                REQUIRE(mon->m_properties.has(PropId::entangled));

                // This may or may not unstuck the monster
                actor::move(*mon, Dir::left);

                // If the move above did unstuck the monster, this command will
                // move it one step to the left
                actor::move(*mon, Dir::left);

                if (mon->m_pos == pos_r)
                {
                        tested_stuck = true;
                }
                else if (mon->m_pos == pos_l)
                {
                        tested_unstuck = true;

                        REQUIRE(!mon->m_properties.has(PropId::entangled));
                }

                test_utils::cleanup_all();
        }

        REQUIRE(tested_stuck);
        REQUIRE(tested_unstuck);
}
