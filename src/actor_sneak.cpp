// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "actor_sneak.hpp"

#include "ability_values.hpp"
#include "actor.hpp"
#include "map.hpp"
#include "misc.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static int calc_sneak_skill_mod(const actor::SneakData& data)
{
        return data.actor_sneaking->ability(AbilityId::stealth, true);
}

static int calc_search_mod(const actor::SneakData& data)
{
        const int search_skill =
                data.actor_searching->ability(
                        AbilityId::searching,
                        true);

        const int mod =
                data.actor_searching->is_player()
                ? -search_skill
                : 0;

        return mod;
}

static int calc_dist_mod(const actor::SneakData& data)
{
        const int dist =
                king_dist(
                        data.actor_sneaking->m_pos,
                        data.actor_searching->m_pos);

        // Distance  Sneak bonus
        // ----------------------
        // 1         -7
        // 2          0
        // 3          7
        // 4         14
        // 5         21
        // 6         28
        // 7         35
        // 8         42
        const int dist_mod = (dist - 2) * 7;

        return dist_mod;
}

static int calc_light_mod(const actor::SneakData& data)
{
        const bool is_lit = map::g_light.at(data.actor_sneaking->m_pos);

        const int light_mod = is_lit ? -40 : 0;

        return light_mod;
}

static int calc_dark_mod(const actor::SneakData& data)
{
        const bool is_lit = map::g_light.at(data.actor_sneaking->m_pos);
        const bool is_dark = map::g_dark.at(data.actor_sneaking->m_pos);

        const int dark_mod = (is_dark && !is_lit) ? 10 : 0;

        return dark_mod;
}

// -----------------------------------------------------------------------------
// actor
// -----------------------------------------------------------------------------
namespace actor
{
ActionResult roll_sneak(const SneakData& data)
{
        // NOTE: There is no need to cap the sneak value here, since there's
        // always critical fails
        const int tot_skill =
                calc_sneak_skill_mod(data) +
                calc_search_mod(data) +
                calc_dist_mod(data) +
                calc_light_mod(data) +
                calc_dark_mod(data);

        const auto result = ability_roll::roll(tot_skill);

        return result;
}

}  // namespace actor
