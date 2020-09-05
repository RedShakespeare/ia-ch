// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ACTOR_SNEAK_HPP
#define ACTOR_SNEAK_HPP

enum class ActionResult;

namespace actor
{
class Actor;

struct SneakData
{
        const actor::Actor* actor_sneaking { nullptr };
        const actor::Actor* actor_searching { nullptr };
};

// This function is not concerned with whether actors are within FOV, or if they
// are actually hidden or not. It merely performs a skill check, taking various
// conditions such as light/dark into concern.
ActionResult roll_sneak( const SneakData& data );

}  // namespace actor

#endif  // ACTOR_SNEAK_HPP
