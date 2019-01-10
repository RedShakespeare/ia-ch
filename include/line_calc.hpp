// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef LINE_CALC_HPP
#define LINE_CALC_HPP

#include <vector>

struct P;

namespace line_calc
{

void init();

std::vector<P> calc_new_line(
        const P& origin,
        const P& target,
        const bool should_stop_at_target,
        const int king_dist_limit,
        const bool allow_outside_map);

const std::vector<P>* fov_delta_line(
        const P& delta,
        const double& max_dist_abs);

} // line_calc

#endif // LINE_CALC_HPP
