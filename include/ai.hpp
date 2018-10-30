#ifndef AI_HPP
#define AI_HPP

#include <vector>

#include "array2.hpp"
#include "global.hpp"

struct P;
class Mon;

namespace ai
{

// -----------------------------------------------------------------------------
// Things that cost turns for the monster
// -----------------------------------------------------------------------------
namespace action
{

bool try_cast_random_spell(Mon& mon);

bool handle_closed_blocking_door(Mon& mon, std::vector<P> path);

bool handle_inventory(Mon& mon);

bool make_room_for_friend(Mon& mon);

bool move_to_random_adj_cell(Mon& mon);

bool move_to_target_simple(Mon& mon);

bool step_path(Mon& mon, std::vector<P>& path);

bool step_to_lair_if_los(Mon& mon, const P& lair_p);

} // action

// -----------------------------------------------------------------------------
// Information gathering
// -----------------------------------------------------------------------------
namespace info
{

bool look(Mon& mon);

std::vector<P> find_path_to_lair_if_no_los(Mon& mon, const P& lair_p);

std::vector<P> find_path_to_leader(Mon& mon);

std::vector<P> find_path_to_target(Mon& mon);

void set_special_blocked_cells(Mon& mon, Array2<bool>& a);

} // info

} // ai

#endif // AI_HPP
