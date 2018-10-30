#ifndef ACTOR_MOVE_HPP
#define ACTOR_MOVE_HPP

#include "direction.hpp"

class Actor;

namespace actor
{

void move(Actor& actor, const Dir dir);

}

#endif // ACTOR_MOVE_HPP
