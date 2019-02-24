// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ATTACK_HPP
#define ATTACK_HPP

#include "global.hpp"


namespace item
{
class Wpn;
}

namespace actor
{
class Actor;
}

struct P;


namespace attack
{

// NOTE: Attacker origin is needed since attacker may be a null pointer.
void melee(
        actor::Actor* const attacker,
        const P& attacker_origin,
        actor::Actor& defender,
        item::Wpn& wpn);

DidAction ranged(
        actor::Actor* const attacker,
        const P& origin,
        const P& aim_pos,
        item::Wpn& wpn);

void ranged_hit_chance(
        const actor::Actor& attacker,
        const actor::Actor& defender,
        const item::Wpn& wpn);

} // attack

#endif // ATTACK_HPP
