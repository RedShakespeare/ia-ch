#include "catch.hpp"

#include "actor_player.hpp"
#include "feature_rigid.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "line_calc.hpp"
#include "map.hpp"
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

        init::init_io();
        init::init_game();
        init::init_session();

        map::reset({20, 20});

        map::put(new Floor(P(5, 7)));
        map::put(new Floor(P(5, 9)));
        map::put(new Floor(P(5, 10)));
        map::player->pos = P(5, 10);

        Item* item = item_factory::make(ItemId::thr_knife);

        throwing::throw_item(*(map::player), {5, 8}, *item);

        REQUIRE(map::cells.at(5, 9).item == item);

        init::cleanup_session();
        init::cleanup_game();
        init::cleanup_io();
}
