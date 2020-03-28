// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "mapgen.hpp"

#include "actor.hpp"
#include "flood.hpp"
#include "game_time.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "terrain_pylon.hpp"

namespace mapgen {

void make_pylons_and_levers()
{
        // Never make Pylons late game (theme)
        if (map::g_dlvl >= g_dlvl_first_late_game) {
                return;
        }

        // Determine number of Pylons to place, by a weighted choice
        std::vector<int> nr_weights = {
                40, // 0 pylon(s)
                4, // 1 -
                1, // 2 -
        };

        const int nr_pylons = rnd::weighted_choice(nr_weights);

        Array2<bool> blocked(map::dims());

        map_parsers::IsNotFloorLike()
                .run(blocked, blocked.rect());

        blocked = map_parsers::expand(blocked, 2);

        for (auto* const actor : game_time::g_actors) {
                blocked.at(actor->m_pos) = true;
        }

        for (int i = 0; i < nr_pylons; ++i) {
                // Store non-blocked (false) cells in a vector
                const auto p_bucket = to_vec(blocked, false, blocked.rect());

                if (p_bucket.empty()) {
                        // No position available to place a Pylon - give up
                        return;
                }

                const P pylon_p = rnd::element(p_bucket);

                // Do not try this position again, regardless if we place this
                // pylon or not
                blocked.at(pylon_p) = true;

                // Find position for lever
                std::vector<P> lever_pos_bucket;

                {
                        const int lever_max_dist = 4;

                        const auto flood = floodfill(
                                pylon_p,
                                blocked,
                                lever_max_dist);

                        // Reserve worst case of push-backs
                        const int side = (lever_max_dist + 1) * 2;

                        lever_pos_bucket.reserve((side * side) - 1);

                        const int x0 = std::max(
                                0,
                                pylon_p.x - lever_max_dist);

                        const int y0 = std::max(
                                0,
                                pylon_p.y - lever_max_dist);

                        const int x1 = std::min(
                                map::w() - 1,
                                pylon_p.x + lever_max_dist);

                        const int y1 = std::min(
                                map::h() - 1,
                                pylon_p.y + lever_max_dist);

                        for (int x = x0; x <= x1; ++x) {
                                for (int y = y0; y <= y1; ++y) {
                                        if (flood.at(x, y) > 0) {
                                                lever_pos_bucket.emplace_back(
                                                        P(x, y));
                                        }
                                }
                        }
                }

                if (lever_pos_bucket.empty()) {
                        // Cannot place a Lever near this Pylon - too bad, next!
                        continue;
                }

                const P lever_p = rnd::element(lever_pos_bucket);

                // OK, valid positions found - place Pylon and Lever
                auto* const pylon =
                        new terrain::Pylon(
                                pylon_p,
                                terrain::PylonId::any);

                auto* const lever = new terrain::Lever(lever_p);

                lever->set_linked_terrain(*pylon);

                map::g_cells.at(pylon_p).terrain = pylon;

                map::g_cells.at(lever_p).terrain = lever;

                // Don't place other pylons too near
                {
                        const int min_dist_between = 8;

                        const int x0 = std::max(
                                0,
                                pylon_p.x - min_dist_between);

                        const int y0 = std::max(
                                0,
                                pylon_p.y - min_dist_between);

                        const int x1 = std::min(
                                map::w() - 1,
                                pylon_p.x + min_dist_between);

                        const int y1 = std::min(
                                map::h() - 1,
                                pylon_p.y + min_dist_between);

                        for (int x = x0; x <= x1; ++x) {
                                for (int y = y0; y <= y1; ++y) {
                                        blocked.at(x, y) = true;
                                }
                        }
                }
        } // Pylons loop
} // make_pylons_and_levers

} // namespace mapgen
