#ifndef ACTOR_DIE_HPP
#define ACTOR_DIE_HPP

class Actor;

enum class IsDestroyed {no, yes};

enum class AllowGore {no, yes};

enum class AllowDropItems{no, yes};

void kill_actor(
        Actor& actor,
        const IsDestroyed is_destroyed,
        const AllowGore allow_gore,
        const AllowDropItems allow_drop_items);

#endif // ACTOR_DIE_HPP
