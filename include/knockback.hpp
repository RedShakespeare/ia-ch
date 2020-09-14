// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef KNOCKBACK_HPP
#define KNOCKBACK_HPP

#include "global.hpp"

namespace actor
{
class Actor;
}  // namespace actor

struct P;

namespace knockback
{
void run(
        actor::Actor& defender,
        const P& attacked_from_pos,
        bool is_spike_gun,
        Verbose verbose = Verbose::yes,
        int paralyze_extra_turns = 0);

}  // namespace knockback

#endif  // KNOCKBACK_HPP
