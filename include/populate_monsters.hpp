// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef POPULATE_MON_HPP
#define POPULATE_MON_HPP

#include <vector>

#include "global.hpp"
#include "room.hpp"


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

std::vector<P> make_sorted_free_cells(
        const P& origin,
        const Array2<bool>& blocked);

// Unwalkable terrain or too close to the player
Array2<bool> forbidden_spawn_positions();

void spawn_for_repopulate_over_time();

void populate_std_lvl();

// Convenient function for special levels which should auto-spawn monsters
// instead of according to design (or in addition to hand-placed monsters)
void populate_lvl_as_room_types(const std::vector<RoomType>& room_types);

} // populate_mon

#endif // POPULATE_MON_HPP
