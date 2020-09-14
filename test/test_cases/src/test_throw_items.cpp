// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "catch.hpp"

#include "actor_factory.hpp"
#include "actor_player.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "line_calc.hpp"
#include "map.hpp"
#include "terrain.hpp"
#include "test_utils.hpp"
#include "throwing.hpp"

TEST_CASE("Throw weapon at wall")
{
        // Throwing a weapon at a wall should make it land in front of the wall,
        // i.e. the last cell it travelled through BEFORE the wall.

        // Setup:
        // . <- Floor                              (5,  7)
        // # <- Wall  --- Aim position             (5,  8)
        // . <- Floor --- Weapon should land here  (5,  9)
        // @ <- Floor --- Origin position          (5, 10)

        test_utils::init_all();

        map::put(new terrain::Floor({5, 7}));
        map::put(new terrain::Wall({5, 8}));
        map::put(new terrain::Floor({5, 9}));
        map::put(new terrain::Floor({5, 10}));
        map::g_player->m_pos = {5, 10};

        auto* item = item::make(item::Id::thr_knife);

        throwing::throw_item(*(map::g_player), {5, 8}, *item);

        REQUIRE(map::g_cells.at(5, 9).item == item);

        test_utils::cleanup_all();
}

TEST_CASE("Throw potion at monster")
{
        test_utils::init_all();

        map::put(new terrain::Floor({5, 7}));
        map::put(new terrain::Floor({6, 7}));

        map::g_player->m_pos = {5, 7};

        auto* const mon = actor::make(actor::Id::zombie, {6, 7});

        REQUIRE(!mon->m_properties.has(PropId::r_fire));

        bool did_test_r_fire = false;

        // Throw potions at the monster until it is killed, plus one more throw
        // at the corpse
        while (true)
        {
                bool is_dead = false;

                if (mon->m_state != ActorState::alive)
                {
                        is_dead = true;

                        // Clear fire resistance, throwing at the corpse should
                        // not re-apply it
                        mon->m_properties.end_prop(PropId::r_fire);
                }

                throwing::throw_item(
                        *map::g_player,
                        {6, 7},
                        *item::make(item::Id::potion_r_fire));

                if (is_dead)
                {
                        REQUIRE(!mon->m_properties.has(PropId::r_fire));
                }
                else
                {
                        // Not dead
                        if (mon->m_hp < actor::max_hp(*mon))
                        {
                                did_test_r_fire = true;

                                REQUIRE(mon->m_properties.has(PropId::r_fire));
                        }
                }

                if (is_dead)
                {
                        break;
                }
        }

        REQUIRE(did_test_r_fire);
}
