// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "catch.hpp"

#include "actor_player.hpp"
#include "game_time.hpp"
#include "map.hpp"
#include "terrain.hpp"
#include "test_utils.hpp"

TEST_CASE("Test light map")
{
        test_utils::init_all();

        std::fill(std::begin(map::g_light), std::end(map::g_light), false);
        std::fill(std::begin(map::g_dark), std::end(map::g_dark), true);

        map::g_player->m_pos.set(40, 12);

        const P burn_pos(40, 10);

        auto* const burn_f = map::g_cells.at(burn_pos).terrain;

        while (burn_f->m_burn_state != BurnState::burning) {
                burn_f->hit(DmgType::fire, nullptr);
        }

        game_time::update_light_map();

        map::g_player->update_fov();

        for (const auto& d : dir_utils::g_dir_list_w_center) {
                const P p = burn_pos + d;

                // The cells around the burning floor should be lit
                REQUIRE(map::g_light.at(p));

                // The cells should also be dark (independent from light)
                REQUIRE(map::g_dark.at(p));
        }

        test_utils::cleanup_all();
}
