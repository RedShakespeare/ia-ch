// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "attack_data.hpp"

#include "actor.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "feature_rigid.hpp"
#include "feature_trap.hpp"
#include "item.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "property.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static bool is_defender_aware_of_attack(
        const actor::Actor* const attacker,
        const actor::Actor& defender)
{
        bool is_aware = false;

        if (defender.is_player())
        {
                if (attacker)
                {
                        const auto* const mon =
                                static_cast<const actor::Mon*>(attacker);

                        is_aware = mon->m_player_aware_of_me_counter > 0;
                }
                else // No attacker actor (e.g. a trap)
                {
                        is_aware = true;
                }
        }
        else // Defender is monster
        {
                auto* const mon = static_cast<const actor::Mon*>(&defender);

                is_aware = mon->m_aware_of_player_counter > 0;
        }

        return is_aware;
}

// -----------------------------------------------------------------------------
// Attack data
// -----------------------------------------------------------------------------
AttData::AttData(
        actor::Actor* const the_attacker,
        actor::Actor* const the_defender,
        const item::Item& att_item) :
        attacker(the_attacker),
        defender(the_defender),
        skill_mod(0),
        wpn_mod(0),
        dodging_mod(0),
        state_mod(0),
        hit_chance_tot(0),
        att_result(ActionResult::fail),
        dmg(0),
        is_intrinsic_att(
                (att_item.data().type == ItemType::melee_wpn_intr) ||
                (att_item.data().type == ItemType::ranged_wpn_intr)) {}

MeleeAttData::MeleeAttData(
        actor::Actor* const the_attacker,
        actor::Actor& the_defender,
        const item::Wpn& wpn) :
        AttData(the_attacker, &the_defender, wpn),
        is_backstab(false),
        is_weak_attack(false)
{
        const bool is_defender_aware =
                is_defender_aware_of_attack(attacker, *defender);

        // Determine attack result
        skill_mod =
                attacker
                ? attacker->ability(AbilityId::melee, true)
                : 50;

        wpn_mod = wpn.data().melee.hit_chance_mod;

        dodging_mod = 0;

        const bool player_is_handling_armor =
                map::g_player->m_handle_armor_countdown > 0;

        const int dodging_ability = defender->ability(AbilityId::dodging, true);

        // Player gets melee dodging bonus from wielding a Pitchfork
        if (defender->is_player())
        {
                const auto* const item =
                        defender->m_inv.item_in_slot(SlotId::wpn);

                if (item && (item->id() == item::Id::pitch_fork))
                {
                        dodging_mod -= 15;
                }
        }

        const bool allow_positive_doge =
                is_defender_aware &&
                !(defender->is_player() &&
                  player_is_handling_armor);

        if (allow_positive_doge ||
            (dodging_ability < 0))
        {
                dodging_mod -= dodging_ability;
        }

        // Attacker gets a penalty against unseen targets

        // NOTE: The AI never attacks unseen targets, so in the case of a
        // monster attacker, we can assume the target is seen. We only need to
        // check if target is seen when player is attacking.
        bool can_attacker_see_tgt = true;

        if (attacker == map::g_player)
        {
                can_attacker_see_tgt = map::g_player->can_see_actor(*defender);
        }

        // Check for extra attack bonuses, such as defender being immobilized.

        // TODO: This is weird - just handle dodging penalties with properties!
        bool is_big_att_bon = false;
        bool is_small_att_bon = false;

        if (!is_defender_aware)
        {
                // Give big attack bonus if defender is unaware of the attacker.
                is_big_att_bon = true;
        }

        if (!is_big_att_bon)
        {
                // Check if attacker gets a bonus due to a defender property.

                if (defender->m_properties.has(PropId::paralyzed) ||
                    defender->m_properties.has(PropId::nailed) ||
                    defender->m_properties.has(PropId::fainted) ||
                    defender->m_properties.has(PropId::entangled))
                {
                        // Give big attack bonus if defender is completely
                        // unable to fight.
                        is_big_att_bon = true;
                }
                else if (defender->m_properties.has(PropId::confused) ||
                         defender->m_properties.has(PropId::slowed) ||
                         defender->m_properties.has(PropId::burning))
                {
                        // Give small attack bonus if defender has problems
                        // fighting
                        is_small_att_bon = true;
                }
        }

        // Give small attack bonus if defender cannot see.
        if (!is_big_att_bon &&
            !is_small_att_bon &&
            !defender->m_properties.allow_see())
        {
                is_small_att_bon = true;
        }

        state_mod =
                is_big_att_bon
                ? 50
                : is_small_att_bon
                ? 20
                : 0;

        // Lower hit chance if attacker cannot see target (e.g. attacking
        // invisible creature)
        if (!can_attacker_see_tgt)
        {
                state_mod -= 25;
        }

        // Lower hit chance if defender is ethereal (except if Bane of the
        // Undead bonus applies)
        const bool apply_undead_bane_bon =
                (attacker == map::g_player) &&
                player_bon::gets_undead_bane_bon(*defender->m_data);

        const bool apply_ethereal_defender_pen =
                defender->m_properties.has(PropId::ethereal) &&
                !apply_undead_bane_bon;

        if (apply_ethereal_defender_pen)
        {
                state_mod -= 50;
        }

        hit_chance_tot =
                skill_mod
                + wpn_mod
                + dodging_mod
                + state_mod;

        // NOTE: Total skill may be negative or above 100 (the attacker may
        // still critically hit or miss)
        att_result = ability_roll::roll(hit_chance_tot);

        // Roll the damage dice
        {
                Dice dmg_dice = wpn.melee_dmg(attacker);

                if (apply_undead_bane_bon)
                {
                        dmg_dice.plus += 2;
                }

                // Roll the damage dice
                if (att_result == ActionResult::success_critical)
                {
                        // Critical hit (max damage)
                        dmg = std::max(1, dmg_dice.max());
                }
                else // Not critical hit
                {
                        dmg = std::max(1, dmg_dice.roll());
                }
        }

        if (attacker && attacker->m_properties.has(PropId::weakened))
        {
                // Weak attack (halved damage)
                dmg /= 2;

                dmg = std::max(1, dmg);

                is_weak_attack = true;
        }
        // Attacker not weakened, or not an actor attacking (e.g. a trap)
        else if (attacker && !is_defender_aware)
        {
                TRACE << "Melee attack is backstab" << std::endl;

                // Backstab, +50% damage
                int dmg_pct = 150;

                // +150% if player is Vicious
                if ((attacker == map::g_player) &&
                    player_bon::has_trait(Trait::vicious))
                {
                        dmg_pct += 150;
                }

                // +300% damage if attacking with a dagger
                const auto id = wpn.data().id;

                if ((id == item::Id::dagger) || (id == item::Id::spirit_dagger))
                {
                        dmg_pct += 300;
                }

                dmg = (dmg * dmg_pct) / 100;

                is_backstab = true;
        }
}

RangedAttData::RangedAttData(
        actor::Actor* const the_attacker,
        const P& attacker_origin,
        const P& the_aim_pos,
        const P& current_pos,
        const item::Wpn& wpn) :
        AttData(the_attacker, nullptr, wpn),
        aim_pos(the_aim_pos),
        aim_lvl((actor::Size)0),
        defender_size((actor::Size)0),
        dist_mod(0)
{
        // Determine aim level
        // TODO: Quick hack, Incinerators always aim at the floor
        if (wpn.id() == item::Id::incinerator)
        {
                aim_lvl = actor::Size::floor;
        }
        else // Not incinerator
        {
                auto* const actor_aimed_at = map::actor_at_pos(aim_pos);

                if (actor_aimed_at)
                {
                        aim_lvl = actor_aimed_at->m_data->actor_size;
                }
                else // No actor aimed at
                {
                        const bool is_cell_blocked =
                                map_parsers::BlocksProjectiles().
                                cell(aim_pos);

                        aim_lvl =
                                is_cell_blocked
                                ? actor::Size::humanoid
                                : actor::Size::floor;
                }
        }

        defender = map::actor_at_pos(current_pos);

        if (defender && (defender != attacker))
        {
                TRACE_VERBOSE << "Defender found" << std::endl;

                const P& def_pos(defender->m_pos);

                skill_mod =
                        attacker
                        ? attacker->ability(AbilityId::ranged, true)
                        : 50;

                wpn_mod = wpn.data().ranged.hit_chance_mod;

                const bool is_defender_aware =
                        is_defender_aware_of_attack(attacker, *defender);

                dodging_mod = 0;

                const bool player_is_handling_armor =
                        map::g_player->m_handle_armor_countdown > 0;

                const bool allow_positive_doge =
                        is_defender_aware &&
                        !(defender->is_player() &&
                          player_is_handling_armor);

                const int dodging_ability =
                        defender->ability(AbilityId::dodging, true);

                if (allow_positive_doge ||
                    (dodging_ability < 0))
                {
                        const int defender_dodging =
                                defender->ability(
                                        AbilityId::dodging,
                                        true);

                        dodging_mod = -defender_dodging;
                }

                const int dist_to_tgt = king_dist(attacker_origin, def_pos);

                dist_mod = 15 - (dist_to_tgt * 5);

                defender_size = defender->m_data->actor_size;

                state_mod = 0;

                // Lower hit chance if attacker cannot see target (e.g.
                // attacking invisible creature)
                if (attacker)
                {
                        bool can_attacker_see_tgt = true;

                        if (attacker->is_player())
                        {
                                can_attacker_see_tgt =
                                        map::g_player->can_see_actor(*defender);
                        }
                        else // Attacker is monster
                        {
                                auto* const mon =
                                        static_cast<actor::Mon*>(attacker);

                                Array2<bool> hard_blocked_los(map::dims());

                                const R fov_rect =
                                        fov::fov_rect(
                                                attacker->m_pos,
                                                hard_blocked_los.dims());

                                map_parsers::BlocksLos()
                                        .run(hard_blocked_los,
                                             fov_rect,
                                             MapParseMode::overwrite);

                                can_attacker_see_tgt =
                                        mon->can_see_actor(
                                                *defender,
                                                hard_blocked_los);
                        }

                        if (!can_attacker_see_tgt)
                        {
                                state_mod -= 25;
                        }
                }

                // Player gets attack bonus for attacking unaware monster
                if (attacker == map::g_player)
                {
                        auto* const mon = static_cast<actor::Mon*>(defender);

                        if (mon->m_aware_of_player_counter <= 0)
                        {
                                state_mod += 25;
                        }
                }

                const bool apply_undead_bane_bon =
                        (attacker == map::g_player) &&
                        player_bon::gets_undead_bane_bon(*defender->m_data);

                const bool apply_ethereal_defender_pen =
                        defender->m_properties.has(PropId::ethereal) &&
                        !apply_undead_bane_bon;

                if (apply_ethereal_defender_pen)
                {
                        state_mod -= 50;
                }

                hit_chance_tot =
                        skill_mod
                        + wpn_mod
                        + dodging_mod
                        + dist_mod
                        + state_mod;

                set_constr_in_range(5, hit_chance_tot, 99);

                att_result = ability_roll::roll(hit_chance_tot);

                if (att_result >= ActionResult::success)
                {
                        TRACE_VERBOSE << "Attack roll succeeded" << std::endl;

                        bool player_has_aim_bon = false;

                        if (attacker == map::g_player)
                        {
                                player_has_aim_bon =
                                        attacker->m_properties
                                        .has(PropId::aiming);
                        }

                        Dice dmg_dice = wpn.ranged_dmg(attacker);

                        if ((attacker == map::g_player) &&
                            player_bon::gets_undead_bane_bon(*defender->m_data))
                        {
                                dmg_dice.plus += 2;
                        }

                        dmg =
                                player_has_aim_bon
                                ? dmg_dice.max()
                                : dmg_dice.roll();

                        // Outside effective range limit?
                        if (!wpn.is_in_effective_range_lmt(
                                    attacker_origin,
                                    defender->m_pos))
                        {
                                TRACE_VERBOSE << "Outside effetive range limit"
                                              << std::endl;

                                dmg = std::max(1, dmg / 2);
                        }
                }
        }
}

ThrowAttData::ThrowAttData(
        actor::Actor* const the_attacker,
        const P& aim_pos,
        const P& current_pos,
        const item::Item& item) :
        AttData(the_attacker, nullptr, item),
        aim_lvl((actor::Size)0),
        defender_size((actor::Size)0),
        dist_mod(0)
{
        auto* const actor_aimed_at = map::actor_at_pos(aim_pos);

        // Determine aim level
        if (actor_aimed_at)
        {
                aim_lvl = actor_aimed_at->m_data->actor_size;
        }
        else // Not aiming at actor
        {
                const bool is_cell_blocked =
                        map_parsers::BlocksProjectiles()
                        .cell(current_pos);

                aim_lvl =
                        is_cell_blocked
                        ? actor::Size::humanoid
                        : actor::Size::floor;
        }

        defender = map::actor_at_pos(current_pos);

        if (defender && (defender != attacker))
        {
                TRACE_VERBOSE << "Defender found" << std::endl;

                skill_mod =
                        attacker
                        ? attacker->ability(AbilityId::ranged, true)
                        : 50;

                wpn_mod = item.data().ranged.throw_hit_chance_mod;

                const bool is_defender_aware =
                        is_defender_aware_of_attack(attacker, *defender);

                dodging_mod = 0;

                const bool player_is_handling_armor =
                        map::g_player->m_handle_armor_countdown > 0;

                const int dodging_ability =
                        defender->ability(AbilityId::dodging, true);

                const bool allow_positive_doge =
                        is_defender_aware &&
                        !(defender->is_player() &&
                          player_is_handling_armor);

                if (allow_positive_doge ||
                    (dodging_ability < 0))
                {
                        const int defender_dodging =
                                defender->ability(AbilityId::dodging, true);;

                        dodging_mod = -defender_dodging;
                }

                const P& att_pos(attacker->m_pos);
                const P& def_pos(defender->m_pos);

                const int dist_to_tgt =
                        king_dist(att_pos.x,
                                  att_pos.y,
                                  def_pos.x,
                                  def_pos.y);

                dist_mod = 15 - (dist_to_tgt * 5);

                defender_size = defender->m_data->actor_size;

                state_mod = 0;

                bool can_attacker_see_tgt = true;

                if (attacker == map::g_player)
                {
                        can_attacker_see_tgt =
                                map::g_player->can_see_actor(*defender);
                }

                // Lower hit chance if attacker cannot see target (e.g.
                // attacking invisible creature)
                if (!can_attacker_see_tgt)
                {
                        state_mod -= 25;
                }

                // Player gets attack bonus for attacking unaware monster
                if (attacker == map::g_player)
                {
                        const auto* const mon =
                                static_cast<actor::Mon*>(defender);

                        if (mon->m_aware_of_player_counter <= 0)
                        {
                                state_mod += 25;
                        }
                }

                const bool apply_undead_bane_bon =
                        (attacker == map::g_player) &&
                        player_bon::gets_undead_bane_bon(*defender->m_data);

                const bool apply_ethereal_defender_pen =
                        defender->m_properties.has(PropId::ethereal) &&
                        !apply_undead_bane_bon;

                if (apply_ethereal_defender_pen)
                {
                        state_mod -= 50;
                }

                hit_chance_tot =
                        skill_mod
                        + wpn_mod
                        + dodging_mod
                        + dist_mod
                        + state_mod;

                set_constr_in_range(5, hit_chance_tot, 99);

                att_result = ability_roll::roll(hit_chance_tot);

                if (att_result >= ActionResult::success)
                {
                        TRACE_VERBOSE << "Attack roll succeeded" << std::endl;

                        bool player_has_aim_bon = false;

                        if (attacker == map::g_player)
                        {
                                player_has_aim_bon =
                                        attacker->m_properties
                                        .has(PropId::aiming);
                        }

                        Dice dmg_dice = item.thrown_dmg(attacker);

                        if (apply_undead_bane_bon)
                        {
                                dmg_dice.plus += 2;
                        }

                        dmg =
                                player_has_aim_bon
                                ? dmg_dice.max()
                                : dmg_dice.roll();

                        // Outside effective range limit?
                        if (!item.is_in_effective_range_lmt(
                                    attacker->m_pos,
                                    defender->m_pos))
                        {
                                TRACE_VERBOSE << "Outside effetive range limit"
                                              << std::endl;

                                dmg = std::max(1, dmg / 2);
                        }
                }
        }
}
