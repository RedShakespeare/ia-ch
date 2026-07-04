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
// Exorcist Cleansing Fire
// -----------------------------------------------------------------------------
std::string SpellCleansingFire::name() const
{
    return i18n::get("spells.cleansing_fire.name", "Cleansing Fire");
}

SpellId SpellCleansingFire::id() const
{
    return SpellId::cleansing_fire;
}

SpellDomain SpellCleansingFire::domain() const
{
    return SpellDomain::END;
}

SpellShock SpellCleansingFire::shock_type() const
{
    return SpellShock::disturbing;
}

int SpellCleansingFire::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

bool SpellCleansingFire::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

Range SpellCleansingFire::burn_duration_range() const
{
    return {3, 5};
}

void SpellCleansingFire::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)player_aware;

    if (!caster) {
        return;
    }

    std::vector<actor::Actor*> targets;

    if (seen_targets.empty()) {
        return;
    }

    if (skill == SpellSkill::basic) {
        targets.push_back(rnd::element(seen_targets));
    }
    else {
        // Skill greater than basic - target all seen foes
        targets = seen_targets;
    }

    for (auto* const actor : targets) {
        // Spell resistance?
        if (actor->m_properties.has(prop::Id::r_spell)) {
            on_resist(*actor);

            continue;
        }

        for (const auto& d : dir_utils::g_dir_list) {
            const auto p(actor->m_pos + d);

            // Hit the terrain with burning several times, to increase the chance of it
            // catching fire.
            for (int i = 0; i < 6; ++i) {
                map::g_terrain.at(p)->hit(DmgType::fire, nullptr);
            }
        }

        prop::Prop* const burning = prop::make(prop::Id::burning);

        burning->set_duration(burn_duration_range().roll());

        actor->m_properties.apply(burning);
    }
}

std::vector<std::string> SpellCleansingFire::descr_specific(
    SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.cleansing_fire.burn_prefix",
            "Causes the spell's victims to burn for ") +
        burn_duration_range().str() +
        i18n::get(
            "spells.cleansing_fire.burn_suffix",
            " turns, and scorches the ground around them with fire "
            "(be careful with hitting adjacent creatures)."));

    descr.emplace_back(
        skill == SpellSkill::basic
            ? i18n::get(
                  "spells.target.one_visible_hostile",
                  "Affects one random visible hostile creature.")
            : i18n::get(
                  "spells.target.all_visible_hostile",
                  "Affects all visible hostile creatures."));

    return descr;
}

// -----------------------------------------------------------------------------
// Exorcist Sanctuary


// -----------------------------------------------------------------------------
// Exorcist Sanctuary
// -----------------------------------------------------------------------------
std::string SpellSanctuary::name() const
{
    return i18n::get("spells.sanctuary.name", "Sanctuary");
}

SpellId SpellSanctuary::id() const
{
    return SpellId::sanctuary;
}

SpellDomain SpellSanctuary::domain() const
{
    return SpellDomain::END;
}

SpellShock SpellSanctuary::shock_type() const
{
    return SpellShock::mild;
}

int SpellSanctuary::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 5;
}

bool SpellSanctuary::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

Range SpellSanctuary::duration(const SpellSkill skill) const
{
    if (skill == SpellSkill::basic) {
        return {3, 5};
    }
    else {
        return {5, 10};
    }
}

void SpellSanctuary::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    if (!caster) {
        return;
    }

    const auto prop_duration = duration(skill);

    auto* const sanctuary = prop::make(prop::Id::sanctuary);

    sanctuary->set_duration(prop_duration.roll());

    caster->m_properties.apply(sanctuary);
}

std::vector<std::string> SpellSanctuary::descr_specific(
    SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.sanctuary.descr",
            "The caster is ignored by all hostile creatures for the "
            "duration of the spell. The effect is interrupted if the "
            "caster moves or performs a melee or ranged attack."));

    descr.emplace_back(spell_duration_descr(duration(skill).str()));

    return descr;
}

// -----------------------------------------------------------------------------
// Exorcist Purge


// -----------------------------------------------------------------------------
// Exorcist Purge
// -----------------------------------------------------------------------------
std::string SpellPurge::name() const
{
    return i18n::get("spells.purge.name", "Purge");
}

SpellId SpellPurge::id() const
{
    return SpellId::purge;
}

SpellDomain SpellPurge::domain() const
{
    return SpellDomain::END;
}

bool SpellPurge::can_be_improved_with_skill() const
{
    return false;
}

SpellShock SpellPurge::shock_type() const
{
    return SpellShock::mild;
}

int SpellPurge::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 4;
}

bool SpellPurge::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

Range SpellPurge::dmg_range() const
{
    return {5, 10};
}

Range SpellPurge::fear_duration_range() const
{
    return {3, 6};
}

void SpellPurge::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)skill;
    (void)seen_targets;
    (void)player_aware;

    if (!caster) {
        return;
    }

    for (const P& d : dir_utils::g_dir_list) {
        const auto p(caster->m_pos + d);

        terrain::Terrain* const terrain = map::g_terrain.at(p);

        switch (terrain->id()) {
        case terrain::Id::altar:
        case terrain::Id::monolith:
        case terrain::Id::mirror:
        case terrain::Id::gong:     {
            if (map::g_seen.at(p)) {
                draw_blast_at_cells({p}, colors::light_white());
            }

            terrain->hit(DmgType::pure, caster);
        } break;

        default: {
        } break;
        }
    }

    for (actor::Actor* const actor : game_time::g_actors) {
        if ((actor == caster) ||
            !actor->m_pos.is_adjacent(caster->m_pos) ||
            !actor->m_properties.has(prop::Id::undead)) {
            continue;
        }

        // Is adjacent undead creature

        if (actor::can_player_see_actor(*actor)) {
            const auto name = text_format::first_to_upper(actor::name_the(*actor));

            msg_log::add(
                name +
                    i18n::get("spells.is_struck_suffix", " is struck."),
                colors::msg_good());

            draw_blast_at_cells({actor->m_pos}, colors::light_white());
        }

        actor::hit(
            *actor,
            dmg_range().roll(),
            DmgType::pure,
            caster);

        if (actor::is_alive(*actor)) {
            prop::Prop* const fear = prop::make(prop::Id::terrified);

            fear->set_duration(fear_duration_range().roll());

            actor->m_properties.apply(fear);
        }
    }
}

std::vector<std::string> SpellPurge::descr_specific(
    SpellSkill skill) const
{
    (void)skill;

    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.purge.destroy_adjacent_descr",
            "Destroys any altars, monoliths, gongs, or mirrors adjacent to the caster."));

    descr.emplace_back(
        i18n::get(
            "spells.purge.undead_struck_prefix",
            "All Undead creatures adjacent to the caster (seen or not) are "
            "struck with ") +
        dmg_range().str() +
        i18n::get(
            "spells.purge.undead_struck_middle",
            " damage, and become terrified for ") +
        fear_duration_range().str() +
        i18n::get(
            "spells.purge.undead_struck_suffix",
            " turns (unless they resist fear)."));

    return descr;
}

// -----------------------------------------------------------------------------
// Ghoul frenzy


// -----------------------------------------------------------------------------
// Ghoul frenzy
// -----------------------------------------------------------------------------
std::string SpellFrenzy::name() const
{
    return i18n::get("spells.frenzy.name", "Incite Frenzy");
}

SpellId SpellFrenzy::id() const
{
    return SpellId::frenzy;
}

SpellDomain SpellFrenzy::domain() const
{
    return SpellDomain::END;
}

bool SpellFrenzy::can_be_improved_with_skill() const
{
    return false;
}

SpellShock SpellFrenzy::shock_type() const
{
    return SpellShock::mild;
}

int SpellFrenzy::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 0;
}

bool SpellFrenzy::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

void SpellFrenzy::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)skill;
    (void)seen_targets;
    (void)player_aware;

    prop::Prop* prop = prop::make(prop::Id::frenzied);

    prop->set_duration(rnd::range(30, 40));

    caster->m_properties.apply(prop);
}

std::vector<std::string> SpellFrenzy::descr_specific(
    const SpellSkill skill) const
{
    (void)skill;

    return {
        i18n::get(
            "spells.frenzy.descr",
            "Incites a great rage in the caster, who will charge their "
            "enemies with a terrible, uncontrollable fury.")};
}

// -----------------------------------------------------------------------------
// Bless

