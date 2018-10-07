#ifndef ACTOR_DIE_HPP
#define ACTOR_DIE_HPP

class Actor;

enum class IsDestroyed {no, yes};

enum class AllowGore {no, yes};

enum class AllowDropItems{no, yes};

namespace actor
{

void kill(
        Actor& actor,
        const IsDestroyed is_destroyed,
        const AllowGore allow_gore,
        const AllowDropItems allow_drop_items);

void unset_actor_as_leader_for_all_mon(Actor& actor);

} // actor

#endif // ACTOR_DIE_HPP
