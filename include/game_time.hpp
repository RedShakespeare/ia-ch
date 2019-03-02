// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef GAME_TIME_HPP
#define GAME_TIME_HPP

#include <vector>

#include "actor_data.hpp"


namespace actor
{
class Actor;
}

namespace terrain
{
class Terrain;
}


namespace game_time
{

extern std::vector<actor::Actor*> g_actors;
extern std::vector<terrain::Terrain*> g_mobs;

extern bool g_is_magic_descend_nxt_std_turn;

void init();
void cleanup();

void save();
void load();

void add_actor(actor::Actor* actor);

void tick();

int turn_nr();

actor::Actor* current_actor();

std::vector<terrain::Terrain*> mobs_at_pos(const P& p);

void add_mob(terrain::Terrain* const t);

void erase_mob(terrain::Terrain* const t, const bool destroy_object);

void erase_all_mobs();

void reset_turn_type_and_actor_counters();

void update_light_map();

} // game_time

#endif // GAME_TIME_HPP
