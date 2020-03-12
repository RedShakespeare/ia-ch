// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef AI_HPP
#define AI_HPP

#include <vector>

#include "array2.hpp"
#include "global.hpp"


namespace actor
{
class Mon;
} // namespace actor

struct P;


namespace ai
{

// -----------------------------------------------------------------------------
// Things that cost turns for the monster
// -----------------------------------------------------------------------------
namespace action
{

bool try_cast_random_spell(actor::Mon& mon);

bool handle_closed_blocking_door(actor::Mon& mon, std::vector<P> path);

bool handle_inventory(actor::Mon& mon);

bool make_room_for_friend(actor::Mon& mon);

bool move_to_random_adj_cell(actor::Mon& mon);

bool move_to_target_simple(actor::Mon& mon);

bool step_path(actor::Mon& mon, std::vector<P>& path);

bool step_to_lair_if_los(actor::Mon& mon, const P& lair_p);

} // namespace action

// -----------------------------------------------------------------------------
// Information gathering
// -----------------------------------------------------------------------------
namespace info
{

bool look(actor::Mon& mon);

std::vector<P> find_path_to_lair_if_no_los(actor::Mon& mon, const P& lair_p);

std::vector<P> find_path_to_leader(actor::Mon& mon);

std::vector<P> find_path_to_target(actor::Mon& mon);

void set_special_blocked_cells(actor::Mon& mon, Array2<bool>& a);

} // namespace info

} // namespace ai

#endif // AI_HPP
