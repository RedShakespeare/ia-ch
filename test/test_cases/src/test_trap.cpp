#include "catch.hpp"

#include "actor_factory.hpp"
#include "actor_mon.hpp"
#include "feature_rigid.hpp"
#include "feature_trap.hpp"
#include "map.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "rl_utils.hpp"
#include "test_utils.hpp"

TEST_CASE("Spider web")
{
        // Test that a monster can get stuck in a spider web, and that they can
        // break free

        const P pos_l(5, 7);
        const P pos_r(6, 7);

        // There is randomness involved in getting stuck or breaking free, run
        // until we have encountered both of these cases
        bool tested_stuck = false;
        bool tested_unstuck = false;

        while (!(tested_stuck && tested_unstuck))
        {
                test_utils::init_all();

                map::put(new Floor(pos_l));

                map::put(
                        new Trap(
                                pos_r,
                                new Floor(pos_r),
                                TrapId::web));

                Actor* const actor =
                        actor_factory::make(ActorId::zombie, pos_l);

                Mon* const mon = static_cast<Mon*>(actor);

                // Awareness > 0 required for triggering trap
                mon->aware_of_player_counter_ = 42;

                // Move the monster into the trap, and back again
                mon->pos = pos_l;
                mon->move(Dir::right);

                // It should never be possible to move on the first try
                REQUIRE(mon->pos == pos_r);

                REQUIRE(mon->properties.has(PropId::entangled));

                // This may or may not unstuck the monster
                mon->move(Dir::left);

                // If the move above did unstuck the monster, this command will
                // move it one step to the left
                mon->move(Dir::left);

                if (mon->pos == pos_r)
                {
                        tested_stuck = true;
                }
                else if (mon->pos == pos_l)
                {
                        tested_unstuck = true;

                        REQUIRE(!mon->properties.has(PropId::entangled));
                }

                test_utils::cleanup_all();
        }

        REQUIRE(tested_stuck);
        REQUIRE(tested_unstuck);
}
