// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SPELLS_INTERNAL_HPP
#define SPELLS_INTERNAL_HPP

#include <string>
#include <vector>

namespace actor
{
class Actor;
}  // namespace actor

// Internal helpers shared between spells.cpp and the per-domain spell
// implementation files (src/spells/*.cpp). These live in the global
// namespace to match the existing spells.cpp statics.

// "The spell is reflected!"
std::string spell_reflect_msg();

// "Casting this spell does not alert the victim to the caster's presence."
std::string not_alerting_mon_descr();

// "The spell lasts <duration> turns."
std::string spell_duration_descr(const std::string& duration);

// "The spell lasts indefinitely."
std::string spell_indefinite_duration_descr();

// Reward the player with spell points when a spell they cast is resisted
// and they have the absorption trait.
void give_player_sp_for_resist_with_absorption_trait();

// True if the player can see the caster and at least one target. Used by
// corruption-domain spells (Curse, Poison) to decide visibility-dependent
// side effects.
bool can_player_see_caster_and_any_target(
    const actor::Actor& caster,
    const std::vector<actor::Actor*>& targets);

#endif  // SPELLS_INTERNAL_HPP
