// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ACTOR_FACTORY_HPP
#define ACTOR_FACTORY_HPP

#include <vector>


struct P;
struct R;


namespace actor
{

class Actor;
class Mon;

enum class Id;


enum class MakeMonAware
{
        no,
        yes
};

struct MonSpawnResult
{
public:
        MonSpawnResult() :
                monsters() {}

        MonSpawnResult& set_leader(Actor* const leader);

        MonSpawnResult& make_aware_of_player();

        std::vector<Mon*> monsters;
};


void delete_all_mon();

Actor* make(const Id id, const P& pos);

MonSpawnResult spawn(
        const P& origin,
        const std::vector<Id>& monster_ids,
        const R& area_allowed);

MonSpawnResult spawn_random_position(
        const std::vector<Id>& monster_ids,
        const R& area_allowed);

} // actor_factory

#endif // ACTOR_FACTORY_HPP
