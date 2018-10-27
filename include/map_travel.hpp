#ifndef MAP_TRAVEL_HPP
#define MAP_TRAVEL_HPP

#include <vector>
#include <string>

#include "map_builder.hpp"

// TODO: Rename "map travel" to "dungeon"

namespace map_travel
{

extern std::vector<MapType> map_list;

void init();

void save();
void load();

void try_use_down_stairs();

void go_to_nxt();

} // map_travel

#endif // MAP_TRAVEL_HPP
