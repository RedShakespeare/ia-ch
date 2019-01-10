// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ATTACK_HPP
#define ATTACK_HPP

#include "global.hpp"

class Actor;
class Wpn;
struct P;

namespace attack
{

// NOTE: Attacker origin is needed since attacker may be a null pointer.
void melee(
        Actor* const attacker,
        const P& attacker_origin,
        Actor& defender,
        Wpn& wpn);

DidAction ranged(
        Actor* const attacker,
        const P& origin,
        const P& aim_pos,
        Wpn& wpn);

void ranged_hit_chance(
        const Actor& attacker,
        const Actor& defender,
        const Wpn& wpn);

} // attack

#endif // ATTACK_HPP
