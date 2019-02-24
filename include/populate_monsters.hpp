// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef POPULATE_MON_HPP
#define POPULATE_MON_HPP

#include <vector>

#include "global.hpp"


namespace actor
{
enum class Id;
}

struct P;

template<typename T>
class Array2;


namespace populate_mon
{

void make_group_at(
        const actor::Id id,
        const std::vector<P>& sorted_free_cells,
        Array2<bool>* const blocked_out,
        const MonRoamingAllowed roaming_allowed);

// TODO: This is a very general function, it should not be here
std::vector<P> make_sorted_free_cells(
        const P& origin,
        const Array2<bool>& blocked);

void spawn_for_repopulate_over_time();

void populate_std_lvl();

} // populate_mon

#endif // POPULATE_MON_HPP
