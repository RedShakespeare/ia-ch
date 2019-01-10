// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "mapgen.hpp"

#include "actor.hpp"
#include "actor_player.hpp"
#include "feature_monolith.hpp"
#include "game_time.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"

namespace mapgen
{

void make_monoliths()
{
        // Determine number of Monoliths to place, by a weighted choice
        std::vector<int> nr_weights =
                {
                        50, // 0 monolith(s)
                        50, // 1 -
                        1,  // 2 -
                };

        const int nr_monoliths = rnd::weighted_choice(nr_weights);

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksRigid()
                .run(blocked, blocked.rect());

        blocked = map_parsers::expand(blocked, blocked.rect());

        for (Actor* const actor : game_time::actors)
        {
                blocked.at(actor->pos) = true;
        }

        // Block the area around the player
        const P& player_p = map::player->pos;

        const int r = fov_radi_int;

        const R fov_r(
                std::max(0, player_p.x - r),
                std::max(0, player_p.y - r),
                std::min(map::w() - 1, player_p.x + r),
                std::min(map::h() - 1, player_p.y + r));

        for (int x = fov_r.p0.x; x <= fov_r.p1.x; ++x)
        {
                for (int y = fov_r.p0.y; y <= fov_r.p1.y; ++y)
                {
                        blocked.at(x, y) = true;
                }
        }

        std::vector<P> spawn_weight_positions;

        std::vector<int> spawn_weights;

        mapgen::make_explore_spawn_weights(
                blocked,
                spawn_weight_positions,
                spawn_weights);

        for (int i = 0; i < nr_monoliths; ++i)
        {
                // Store non-blocked (false) cells in a vector
                const auto p_bucket = to_vec(blocked, false, blocked.rect());

                if (p_bucket.empty())
                {
                        // Unable to place Monolith
                        return;
                }

                const size_t spawn_p_idx = rnd::weighted_choice(spawn_weights);

                const P p = spawn_weight_positions[spawn_p_idx];

                map::cells.at(p).rigid = new Monolith(p);

                // Block this position and all adjacent positions
                for (const P& d : dir_utils::cardinal_list_w_center)
                {
                        const P p_adj(p + d);

                        blocked.at(p_adj) = true;

                        for (size_t i = 0;
                             i < spawn_weight_positions.size();
                             ++i)
                        {
                                if (spawn_weight_positions[i] != p_adj)
                                {
                                        continue;
                                }

                                spawn_weight_positions.erase(
                                        begin(spawn_weight_positions) + i);

                                spawn_weights.erase(
                                        begin(spawn_weights) + i);
                        }
                }

                ASSERT(spawn_weights.size() == spawn_weight_positions.size());
        }
}

} // namespace
