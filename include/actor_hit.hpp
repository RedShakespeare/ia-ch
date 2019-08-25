// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ACTOR_HIT_HPP
#define ACTOR_HIT_HPP

#include "global.hpp"


enum class ActorDied
{
        no,
        yes
};


namespace actor
{

class Actor;


ActorDied hit(
        Actor& actor,
        int dmg,
        const DmgType dmg_type,
        const DmgMethod method = DmgMethod::END,
        const AllowWound allow_wound = AllowWound::yes);

ActorDied hit_sp(
        Actor& actor,
        const int dmg,
        const Verbose verbose = Verbose::yes);

} // actor

#endif // ACTOR_HIT_HPP
