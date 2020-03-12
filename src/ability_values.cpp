// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "ability_values.hpp"

#include <cmath>

#include "actor_player.hpp"
#include "colors.hpp"
#include "debug.hpp"
#include "map.hpp"
#include "misc.hpp"
#include "player_bon.hpp"
#include "property_handler.hpp"

int AbilityValues::val(
        const AbilityId id,
        const bool is_affected_by_props,
        const actor::Actor& actor) const
{
        int ret = m_ability_list[(size_t)id];

        if (actor.is_player())
        {
                ASSERT(ret == 0);
        }

        if (is_affected_by_props)
        {
                ret += actor.m_properties.ability_mod(id);
        }

        if (actor.is_player())
        {
                // TODO: This should probably also be included for monsters,
                // especially if they should be able to wear armor
                for (const InvSlot& slot : actor.m_inv.m_slots)
                {
                        if (!slot.item)
                        {
                                continue;
                        }

                        auto& d = slot.item->data();

                        ret += d.ability_mods_while_equipped[(size_t)id];
                }

                switch (id)
                {
                case AbilityId::searching:
                {
                        ret += 10;

                        if (player_bon::bg() == Bg::rogue)
                        {
                                ret += 10;
                        }
                }
                break;

                case AbilityId::melee:
                {
                        ret += 60;

                        if (player_bon::has_trait(Trait::adept_melee))
                        {
                                ret += 10;
                        }

                        if (player_bon::has_trait(Trait::expert_melee))
                        {
                                ret += 10;
                        }

                        if (player_bon::has_trait(Trait::master_melee))
                        {
                                ret += 10;
                        }
                }
                break;

                case AbilityId::ranged:
                {
                        ret += 70;

                        if (player_bon::has_trait(Trait::adept_marksman))
                        {
                                ret += 10;
                        }

                        if (player_bon::has_trait(Trait::expert_marksman))
                        {
                                ret += 10;
                        }

                        if (player_bon::has_trait(Trait::expert_marksman))
                        {
                                ret += 10;
                        }

                        if (player_bon::bg() == Bg::ghoul)
                        {
                                ret -= 15;
                        }
                }
                break;

                case AbilityId::dodging:
                {
                        if (player_bon::has_trait(Trait::dexterous))
                        {
                                ret += 25;
                        }

                        if (player_bon::has_trait(Trait::lithe))
                        {
                                ret += 25;
                        }
                }
                break;

                case AbilityId::stealth:
                {
                        if (player_bon::has_trait(Trait::stealthy))
                        {
                                ret += 45;
                        }

                        if (player_bon::has_trait(Trait::imperceptible))
                        {
                                ret += 45;
                        }
                }
                break;

                case AbilityId::END:
                        break;
                }

                if (id == AbilityId::searching)
                {
                        // Searching must ALWAYS be at least 1, to avoid
                        // trapping the player
                        ret = std::max(1, ret);
                }
        }

        // NOTE: Do NOT clamp the returned skill value here

        return ret;
}

void AbilityValues::reset()
{
        for (size_t i = 0; i < (size_t)AbilityId::END; ++i)
        {
                m_ability_list[i] = 0;
        }
}

void AbilityValues::set_val(const AbilityId ability, const int val)
{
        m_ability_list[(size_t)ability] = val;
}

void AbilityValues::change_val(const AbilityId ability, const int change)
{
        m_ability_list[(size_t)ability] += change;
}

namespace ability_roll
{

ActionResult roll(const int skill_value)
{
        // Example:
        // ------------
        // Skill value = 50

        //  1 -   2     Critical success
        //  3 -  25     Big success
        // 26 -  50     Normal success
        // 51 -  75     Normal fail
        // 76 -  98     Big fail
        // 99 - 100     Critical fail

        const int succ_cri_lmt = 2;

        const int succ_big_lmt =
                std::ceil((double)skill_value / 2.0);

        const int succ_nrm_lmt = skill_value;

        const int fail_nrm_lmt =
                std::ceil(100.0 - ((double)(100 - skill_value) / 2.0));

        const int fail_big_lmt = 98;

        const int roll = rnd::range(1, 100);

        // NOTE: We check critical success and fail first, since they should be
        // completely unaffected by skill values - they can always happen, and
        // always have the same chance to happen, regardless of skills
        if (roll <= succ_cri_lmt)
        {
                return ActionResult::success_critical;
        }

        if (roll > fail_big_lmt)
        {
                return ActionResult::fail_critical;
        }

        if (roll <= succ_big_lmt)
        {
                return ActionResult::success_big;
        }

        if (roll <= succ_nrm_lmt)
        {
                return ActionResult::success;
        }

        if (roll <= fail_nrm_lmt)
        {
                return ActionResult::fail;
        }

        ASSERT(roll <= fail_big_lmt);

        return ActionResult::fail_big;
}

int hit_chance_pct_actual(const int value)
{
        return constr_in_range(2, value, 98);
}

} // namespace ability_roll
