// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "line_calc.hpp"

#include <cmath>
#include <vector>

#include "global.hpp"
#include "map.hpp"
#include "misc.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static double s_fov_abs_distances[g_fov_w_int][g_fov_w_int];

static std::vector<P> s_fov_delta_lines[g_fov_w_int][g_fov_w_int];


// -----------------------------------------------------------------------------
// line_calc
// -----------------------------------------------------------------------------
namespace line_calc
{

void init()
{
        // Calculate FOV absolute distances
        for (int y = 0; y < g_fov_w_int; ++y)
        {
                for (int x = 0; x < g_fov_w_int; ++x)
                {
                        s_fov_abs_distances[x][y] = 0;
                }
        }

        // delta_x corresponds to 'i - fov_w_int', and vice versa
        // So:
        // i = 0                => delta_x = -fov_w_int
        // i = fov_w_int * 2    => delta_x =  fov_w_int

        const int r = g_fov_radi_int;

        for (int x = 0; x <= r * 2; ++x)
        {
                for (int y = 0; y <= r * 2; ++y)
                {
                        const auto delta_x_db = (double)x - g_fov_radi_db;
                        const auto delta_y_db = (double)y - g_fov_radi_db;

                        const auto hypot_db = sqrt(
                                (delta_x_db * delta_x_db) +
                                (delta_y_db * delta_y_db));

                        s_fov_abs_distances[x][y] = floor(hypot_db);
                }
        }

        // Calculate FOV delta lines
        for (int delta_x = -r; delta_x <= r; delta_x++)
        {
                for (int delta_y = -r; delta_y <= r; delta_y++)
                {
                        const P origin(0, 0);
                        const P target(P(delta_x, delta_y));

                        s_fov_delta_lines[delta_x + r][delta_y + r] =
                                calc_new_line(origin, target, true, 999, true);
                }
        }
}

const std::vector<P>* fov_delta_line(
        const P& delta,
        const double& max_dist_abs)
{
        const int x = delta.x + g_fov_radi_int;
        const int y = delta.y + g_fov_radi_int;

        if (x >= 0 && y >= 0 && x < g_fov_w_int && y < g_fov_w_int)
        {
                if (s_fov_abs_distances[x][y] <= max_dist_abs)
                {
                        return &(s_fov_delta_lines[x][y]);
                }
        }

        return nullptr;
}

std::vector<P> calc_new_line(
        const P& origin,
        const P& target,
        const bool should_stop_at_target,
        const int king_dist_limit,
        const bool allow_outside_map)
{
        std::vector<P> line;

        if (target == origin)
        {
                line.push_back(origin);

                return line;
        }

        const auto delta_x_db = double(target.x - origin.x);
        const auto delta_y_db = double(target.y - origin.y);

        const double hypot_db =
                sqrt((delta_x_db * delta_x_db) +
                     (delta_y_db * delta_y_db));

        const double x_incr_db = (delta_x_db / hypot_db);
        const double y_incr_db = (delta_y_db / hypot_db);

        double current_x_db = double(origin.x) + 0.5;
        double current_y_db = double(origin.y) + 0.5;

        P current_pos = P(int(current_x_db), int(current_y_db));

        const double step_size_db = 0.04;

        for (double i = 0.0; i <= 9999.0; i += step_size_db)
        {
                current_x_db += x_incr_db * step_size_db;
                current_y_db += y_incr_db * step_size_db;

                current_pos.set(floor(current_x_db), floor(current_y_db));

                if (!allow_outside_map && !map::is_pos_inside_map(current_pos))
                {
                        return line;
                }

                bool is_pos_ok_to_add = false;

                if (line.empty())
                {
                        is_pos_ok_to_add = true;
                }
                else
                {
                        is_pos_ok_to_add = line.back() != current_pos;
                }

                if (is_pos_ok_to_add)
                {
                        line.push_back(current_pos);
                }

                // Check distance limits
                if (should_stop_at_target && (current_pos == target))
                {
                        return line;
                }

                const int current_king_dist = king_dist(origin, current_pos);

                if (current_king_dist >= king_dist_limit)
                {
                        return line;
                }
        }

        return line;
}

} // namespace line_calc
