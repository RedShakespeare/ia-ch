// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ABILITY_VALUES_HPP
#define ABILITY_VALUES_HPP

#include <cstddef>
#include <string>

namespace actor
{
class Actor;
}  // namespace actor

enum class AbilityId
{
        melee,
        ranged,
        dodging,
        stealth,
        searching,
        END
};

enum class ActionResult
{
        fail_critical,
        fail_big,
        fail,
        success,
        success_big,
        success_critical
};

// Each actor has an instance of this class
class AbilityValues
{
public:
        AbilityValues()
        {
                reset();
        }

        AbilityValues& operator=(const AbilityValues& other) = default;

        void reset();

        int val(AbilityId id,
                bool is_affected_by_props,
                const actor::Actor& actor) const;

        int raw_val(const AbilityId id) const
        {
                return m_ability_list[(size_t)id];
        }

        void set_val(AbilityId ability, int val);

        void change_val(AbilityId ability, int change);

private:
        int m_ability_list[(size_t)AbilityId::END];
};

namespace ability_roll
{
ActionResult roll(int skill_value);

// Intended for presenting hit chance values to the player (when aiming etc).
// The parameter value (which could for example be > 100% due to bonuses) is
// clamped within the actual possible hit chance range, considering critical
// hits and misses.
int hit_chance_pct_actual(int value);

}  // namespace ability_roll

#endif  // ABILITY_VALUES_HPP
