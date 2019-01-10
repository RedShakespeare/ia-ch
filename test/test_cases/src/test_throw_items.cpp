// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "catch.hpp"

#include "actor_player.hpp"
#include "feature_rigid.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "line_calc.hpp"
#include "map.hpp"
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

        map::put(new Floor(P(5, 7)));
        map::put(new Wall(P(5, 8)));
        map::put(new Floor(P(5, 9)));
        map::put(new Floor(P(5, 10)));
        map::player->pos = P(5, 10);

        Item* item = item_factory::make(ItemId::thr_knife);

        throwing::throw_item(*(map::player), {5, 8}, *item);

        REQUIRE(map::cells.at(5, 9).item == item);

        test_utils::cleanup_all();
}
