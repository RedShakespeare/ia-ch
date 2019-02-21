// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ACTOR_MOVE_HPP
#define ACTOR_MOVE_HPP

#include "direction.hpp"


namespace actor
{

class Actor;


void move(Actor& actor, const Dir dir);

}

#endif // ACTOR_MOVE_HPP
