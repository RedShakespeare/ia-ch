// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ACTOR_DIE_HPP
#define ACTOR_DIE_HPP


enum class IsDestroyed
{
        no,
        yes
};

enum class AllowGore
{
        no,
        yes
};

enum class AllowDropItems
{
        no,
        yes
};


namespace actor
{

class Actor;


void kill(
        Actor& actor,
        const IsDestroyed is_destroyed,
        const AllowGore allow_gore,
        const AllowDropItems allow_drop_items);

void unset_actor_as_leader_for_all_mon(Actor& actor);

} // actor

#endif // ACTOR_DIE_HPP
