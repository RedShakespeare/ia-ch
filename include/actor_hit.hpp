// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
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
        DmgType dmg_type,
        DmgMethod method = DmgMethod::END,
        AllowWound allow_wound = AllowWound::yes);

ActorDied hit_sp(
        Actor& actor,
        int dmg,
        Verbose verbose = Verbose::yes);

} // namespace actor

#endif // ACTOR_HIT_HPP
