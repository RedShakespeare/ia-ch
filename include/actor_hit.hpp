#ifndef ACTOR_HIT_HPP
#define ACTOR_HIT_HPP

#include "global.hpp"

class Actor;

enum class ActorDied {no ,yes};

namespace actor
{

ActorDied hit(
        Actor& actor,
        int dmg,
        const DmgType dmg_type,
        const DmgMethod method = DmgMethod::END,
        const AllowWound allow_wound = AllowWound::yes);

ActorDied hit_sp(
        Actor& actor,
        const int dmg,
        const Verbosity verbosity = Verbosity::verbose);

} // actor

#endif // ACTOR_HIT_HPP
