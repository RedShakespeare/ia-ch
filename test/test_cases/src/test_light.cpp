#include "catch.hpp"

#include "actor_player.hpp"
#include "feature_rigid.hpp"
#include "game_time.hpp"
#include "map.hpp"
#include "rl_utils.hpp"
#include "test_utils.hpp"

TEST_CASE("Test light map")
{
        test_utils::init_all();

        std::fill(std::begin(map::light), std::end(map::light), false);
        std::fill(std::begin(map::dark), std::end(map::dark), true);

        map::player->pos.set(40, 12);

        const P burn_pos(40, 10);

        Rigid* const burn_f = map::cells.at(burn_pos).rigid;

        while (burn_f->burn_state_ != BurnState::burning)
        {
                burn_f->hit(
                        1,
                        DmgType::fire,
                        DmgMethod::elemental);
        }

        game_time::update_light_map();

        map::player->update_fov();

        for (const auto& d : dir_utils::dir_list_w_center)
        {
                const P p = burn_pos + d;

                // The cells around the burning floor should be lit
                REQUIRE(map::light.at(p));

                // The cells should also be dark (independent from light)
                REQUIRE(map::dark.at(p));
        }

        test_utils::cleanup_all();
}
