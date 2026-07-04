// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "spells.hpp"
#include "spells_internal.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "actor.hpp"
#include "actor_data.hpp"
#include "actor_death.hpp"
#include "actor_eat.hpp"
#include "actor_factory.hpp"
#include "actor_hit.hpp"
#include "actor_player_state.hpp"
#include "actor_see.hpp"
#include "array2.hpp"
#include "attack.hpp"
#include "audio.hpp"
#include "audio_data.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "direction.hpp"
#include "draw_blast.hpp"
#include "drop.hpp"
#include "explosion.hpp"
#include "flood.hpp"
#include "fov.hpp"
#include "game_time.hpp"
#include "gfx.hpp"
#include "global.hpp"
#include "i18n.hpp"
#include "inventory.hpp"
#include "inventory_handling.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "item_weapon.hpp"
#include "knockback.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "marker.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "pathfind.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "pos.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "property_handler.hpp"
#include "rect.hpp"
#include "sound.hpp"
#include "state.hpp"
#include "teleport.hpp"
#include "terrain.hpp"
#include "terrain_data.hpp"
#include "terrain_door.hpp"
#include "terrain_factory.hpp"
#include "terrain_trap.hpp"
#include "text_format.hpp"
#include "viewport.hpp"
#include "wpn_dmg.hpp"


// -----------------------------------------------------------------------------
// Haste
// -----------------------------------------------------------------------------
std::string SpellHaste::name() const
{
    return i18n::get("spells.haste.name", "Haste");
}

SpellId SpellHaste::id() const
{
    return SpellId::haste;
}

SpellDomain SpellHaste::domain() const
{
    return SpellDomain::time;
}

SpellShock SpellHaste::shock_type() const
{
    return SpellShock::mild;
}

bool SpellHaste::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

Range SpellHaste::duration_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {5, 10};
    case SpellSkill::expert:       return {10, 20};
    case SpellSkill::master:       return {15, 30};
    case SpellSkill::transcendent: return {300, 600};
    }

    ASSERT(false);

    return {1, 1};
}

int SpellHaste::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 5;
}

void SpellHaste::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    prop::Prop* prop = prop::make(prop::Id::hasted);

    prop->set_duration(duration_range(skill).roll());

    caster->m_properties.apply(prop);
}

std::vector<std::string> SpellHaste::descr_specific(const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.haste.descr",
            "The caster moves faster relative to the world around them."));

    descr.push_back(spell_duration_descr(duration_range(skill).str()));

    return descr;
}

int SpellHaste::mon_cooldown() const
{
    return 20;
}

bool SpellHaste::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    return (
        !seen_targets.empty() &&
        !mon.m_properties.has(prop::Id::hasted));
}

// -----------------------------------------------------------------------------
// Premonition


// -----------------------------------------------------------------------------
// Teleport
// -----------------------------------------------------------------------------
int SpellTeleport::mon_cooldown() const
{
    return 20;
}

std::string SpellTeleport::name() const
{
    return i18n::get("spells.teleport.name", "Teleport");
}

SpellId SpellTeleport::id() const
{
    return SpellId::teleport;
}

SpellDomain SpellTeleport::domain() const
{
    return SpellDomain::time;
}

SpellShock SpellTeleport::shock_type() const
{
    return SpellShock::disturbing;
}

int SpellTeleport::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 8;
}

bool SpellTeleport::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellTeleport::max_dist(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return 5;
    case SpellSkill::expert:       return 10;
    case SpellSkill::master:
    case SpellSkill::transcendent: return 15;
    }

    ASSERT(false);
    return -1;
}

int SpellTeleport::invis_duration(const SpellSkill skill) const
{
    return ((skill == SpellSkill::transcendent) ? 6 : 3);
}

void SpellTeleport::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    if (skill >= SpellSkill::master) {
        auto* const invis = prop::make(prop::Id::invis);

        invis->set_duration(invis_duration(skill));

        caster->m_properties.apply(invis);
    }

    const int max_d = max_dist(skill);

    teleport(*caster, ShouldCtrlTele::if_tele_ctrl_prop, max_d);
}

bool SpellTeleport::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    const bool is_low_hp = (mon.m_hp <= (actor::max_hp(mon) / 2));

    return !seen_targets.empty() && is_low_hp && rnd::fraction(3, 4);
}

std::vector<std::string> SpellTeleport::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.teleport.descr_main",
            "Instantly moves the caster to a different position."));

    descr.emplace_back(
        i18n::get(
            "spells.teleport.max_dist_prefix",
            "Maximum teleport distance is ") +
        std::to_string(max_dist(skill)) +
        i18n::get(
            "spells.teleport.max_dist_suffix",
            "."));

    if (skill >= SpellSkill::master) {
        descr.push_back(
            i18n::get(
                "spells.teleport.invis_prefix",
                "On teleporting, the caster is invisible for ") +
            std::to_string(invis_duration(skill)) +
            i18n::get(
                "spells.teleport.invis_suffix",
                " turns."));
    }

    return descr;
}

// -----------------------------------------------------------------------------
// Expulsion


// -----------------------------------------------------------------------------
// Expulsion
// -----------------------------------------------------------------------------
SpellId SpellExpulsion::id() const
{
    return SpellId::expulsion;
}

SpellDomain SpellExpulsion::domain() const
{
    return SpellDomain::time;
}

SpellShock SpellExpulsion::shock_type() const
{
    return SpellShock::disturbing;
}

bool SpellExpulsion::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

std::string SpellExpulsion::name() const
{
    return i18n::get("spells.expulsion.name", "Expulsion");
}

int SpellExpulsion::max_dist(SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return 8;
    case SpellSkill::expert:       return 14;
    case SpellSkill::master:       return 20;
    case SpellSkill::transcendent: return -1;
    }

    ASSERT(false);
    return -1;
}

int SpellExpulsion::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)caster;
    (void)skill;

    return 6;
}

void SpellExpulsion::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    if (seen_targets.empty()) {
        if (actor::is_player(caster)) {
            msg_log::add(i18n::get(
                "spells.momentary_void",
                "A momentary void opens and closes."));
        }

        return;
    }

    // There are targets available

    const std::vector<actor::Actor*> targets =
        (skill == SpellSkill::transcendent)
        ? seen_targets
        : std::vector {rnd::element(seen_targets)};

    if (player_aware == PlayerAwareOfCast::yes) {
        draw_blast_at_seen_actors(targets, colors::gray());
    }

    for (actor::Actor* const target : targets) {
        // Spell resistance?
        if (target->m_properties.has(prop::Id::r_spell)) {
            on_resist(*target);

            // Spell reflection?
            if (target->m_properties.has(prop::Id::spell_reflect)) {
                if (actor::can_player_see_actor(*target)) {
                    msg_log::add(spell_reflect_msg());
                }

                // Run effect with the target as caster, and the
                // caster as seen target instead.
                run_effect(target, skill, {caster}, player_aware);
            }

            continue;
        }

        teleport(*target, ShouldCtrlTele::never, max_dist(skill));

        if (!actor::is_player(target)) {
            target->m_mon_aware_state.aware_counter = 0;
            target->m_mon_aware_state.wary_counter = 0;
        }
    }
}

std::vector<std::string> SpellExpulsion::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    if (skill == SpellSkill::transcendent) {
        descr.emplace_back(
            i18n::get(
                "spells.expulsion.descr_all",
                "All visible hostile creatures are teleported away."));
    }
    else {
        descr.emplace_back(
            i18n::get(
                "spells.expulsion.descr_one",
                "One random visible hostile creature is teleported away."));
    }

    descr.emplace_back(
        i18n::get(
            "spells.expulsion.max_dist_prefix",
            "Max distance is ") +
        std::to_string(max_dist(skill)) +
        i18n::get(
            "spells.expulsion.max_dist_suffix",
            " steps."));

    descr.emplace_back(
        i18n::get(
            "spells.expulsion.forced",
            "The teleportation is forced; the target can never control it."));

    return descr;
}

int SpellExpulsion::mon_cooldown() const
{
    return 30;
}

bool SpellExpulsion::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    const bool is_low_hp = (mon.m_hp <= (actor::max_hp(mon) / 2));

    return !seen_targets.empty() && is_low_hp && rnd::fraction(3, 4);
}

// -----------------------------------------------------------------------------
// Knockback


// -----------------------------------------------------------------------------
// Temporal Echo
// -----------------------------------------------------------------------------
int SpellTemporalEcho::pct_damage_dealt(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return 75;
    case SpellSkill::expert:       return 100;
    case SpellSkill::master:       return 125;
    case SpellSkill::transcendent: return 200;
    }

    ASSERT(false);

    return 100;
}

Range SpellTemporalEcho::duration_range() const
{
    return {6, 8};
}

SpellId SpellTemporalEcho::id() const
{
    return SpellId::temporal_echo;
}

SpellDomain SpellTemporalEcho::domain() const
{
    return SpellDomain::time;
}

SpellShock SpellTemporalEcho::shock_type() const
{
    return SpellShock::mild;
}

bool SpellTemporalEcho::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

std::string SpellTemporalEcho::name() const
{
    return i18n::get("spells.temporal_echo.name", "Temporal Echo");
}

int SpellTemporalEcho::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 4;
}

int SpellTemporalEcho::mon_cooldown() const
{
    return 10;
}

void SpellTemporalEcho::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    if (seen_targets.empty()) {
        if (actor::is_player(caster)) {
            msg_log::add(i18n::get(
                "spells.faint_stutter_in_time",
                "There is a faint stutter in time."));
        }

        return;
    }

    // There are targets available

    if (player_aware == PlayerAwareOfCast::yes) {
        draw_blast_at_seen_actors(seen_targets, colors::magenta());
    }

    const int duration = duration_range().roll();

    for (actor::Actor* const target : seen_targets) {
        // Spell resistance?
        if (target->m_properties.has(prop::Id::r_spell)) {
            on_resist(*target);

            // Spell reflection?
            if (target->m_properties.has(prop::Id::spell_reflect)) {
                if (actor::can_player_see_actor(*target)) {
                    msg_log::add(spell_reflect_msg());
                }

                // Run effect with the target as caster, and the caster as seen target instead.
                run_effect(target, skill, {caster}, player_aware);
            }

            continue;
        }

        apply_temporal_echo_effect(*target, skill, duration);
    }
}

void SpellTemporalEcho::apply_temporal_echo_effect(
    actor::Actor& target,
    const SpellSkill skill,
    const int duration) const
{
    prop::Prop* const temporal_echo = prop::make(prop::Id::temporal_echo);

    temporal_echo->set_duration(duration);

    const int pct_dmg = pct_damage_dealt(skill);

    static_cast<prop::TemporalEcho*>(temporal_echo)->set_percent_damage_dealt(pct_dmg);

    target.m_properties.apply(temporal_echo);
}

std::vector<std::string> SpellTemporalEcho::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr = {
        i18n::get(
            "spells.temporal_echo.descr_main",
            "For all visible enemies, time is manipulated so that damage taken during a "
            "brief period will recur when the effect ends.")};

    descr.push_back(
        i18n::get(
            "spells.temporal_echo.duration_prefix",
            "The effect lasts for ") +
        duration_range().str() +
        i18n::get(
            "spells.temporal_echo.duration_middle",
            " turns (their turns). ") +
        std::to_string(pct_damage_dealt(skill)) +
        i18n::get(
            "spells.temporal_echo.duration_suffix",
            "% of the damage taken during the effect is dealt again."));

    return descr;
}

bool SpellTemporalEcho::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Slow


// -----------------------------------------------------------------------------
// Slow
// -----------------------------------------------------------------------------
std::string SpellSlow::name() const
{
    return i18n::get("spells.slow.name", "Slow");
}

SpellId SpellSlow::id() const
{
    return SpellId::slow;
}

SpellDomain SpellSlow::domain() const
{
    return SpellDomain::time;
}

SpellShock SpellSlow::shock_type() const
{
    return SpellShock::mild;
}

bool SpellSlow::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

Range SpellSlow::duration_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {5, 10};
    case SpellSkill::expert:       return {7, 12};
    case SpellSkill::master:       return {9, 14};
    case SpellSkill::transcendent: return {30, 50};
    }

    ASSERT(false);

    return {1, 1};
}

int SpellSlow::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 5;
}

void SpellSlow::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    const int duration = duration_range(skill).roll();

    if (seen_targets.empty()) {
        if (actor::is_player(caster)) {
            msg_log::add(i18n::get(
                "spells.bugs_move_slowly",
                "The bugs on the ground suddenly move very slowly."));
        }

        return;
    }

    // There are targets available

    const std::vector<actor::Actor*> targets =
        (skill == SpellSkill::basic)
        ? std::vector {rnd::element(seen_targets)}
        : seen_targets;

    if (player_aware == PlayerAwareOfCast::yes) {
        draw_blast_at_seen_actors(targets, colors::magenta());
    }

    for (actor::Actor* const target : targets) {
        // Spell resistance?
        if (target->m_properties.has(prop::Id::r_spell)) {
            on_resist(*target);

            // Spell reflection?
            if (target->m_properties.has(prop::Id::spell_reflect)) {
                if (actor::can_player_see_actor(*target)) {
                    msg_log::add(spell_reflect_msg());
                }

                // Run effect with the target as caster, and the caster as seen target instead.
                run_effect(target, skill, {caster}, player_aware);
            }

            continue;
        }

        auto* const prop = prop::make(prop::Id::slowed);

        prop->set_duration(duration);

        target->m_properties.apply(prop);
    }
}

std::vector<std::string> SpellSlow::descr_specific(
    const SpellSkill skill) const
{
    (void)skill;

    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.slow.descr",
            "Causes the spell's victims to move more slowly."));

    descr.emplace_back(not_alerting_mon_descr());

    descr.emplace_back(
        skill == SpellSkill::basic
            ? i18n::get(
                "spells.target.one_visible_hostile",
                "Affects one random visible hostile creature.")
            : i18n::get(
                "spells.target.all_visible_hostile",
                "Affects all visible hostile creatures."));

    descr.push_back(spell_duration_descr(duration_range(skill).str()));

    return descr;
}

int SpellSlow::mon_cooldown() const
{
    return 20;
}

bool SpellSlow::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Terrify

