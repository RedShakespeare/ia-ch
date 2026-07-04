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
// Aura of Decay
// -----------------------------------------------------------------------------
std::string SpellAuraOfDecay::name() const
{
    return i18n::get("spells.aura_of_decay.name", "Aura of Decay");
}

SpellId SpellAuraOfDecay::id() const
{
    return SpellId::aura_of_decay;
}

SpellDomain SpellAuraOfDecay::domain() const
{
    return SpellDomain::corruption;
}

SpellShock SpellAuraOfDecay::shock_type() const
{
    return SpellShock::disturbing;
}

bool SpellAuraOfDecay::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

Range SpellAuraOfDecay::dmg_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {1, 1};  // Avg 1.0
    case SpellSkill::expert:       return {1, 2};  // Avg 1.5
    case SpellSkill::master:
    case SpellSkill::transcendent: return {1, 3};  // Avg 2.0
    }

    ASSERT(false);

    return {1, 1};
}

Range SpellAuraOfDecay::duration_range(const SpellSkill skill) const
{
    const int k = std::min(3, (int)skill + 1);

    Range duration_range;
    duration_range.min = 15 * k;
    duration_range.max = duration_range.min * 2;

    return duration_range;
}

int SpellAuraOfDecay::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 6;
}

void SpellAuraOfDecay::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    auto* prop = static_cast<prop::AuraOfDecay*>(prop::make(prop::Id::aura_of_decay));

    prop->set_duration(duration_range(skill).roll());

    prop->set_dmg_range(dmg_range(skill));

    if (skill == SpellSkill::transcendent) {
        prop->set_allow_instant_kill();
    }

    caster->m_properties.apply(prop);
}

std::vector<std::string> SpellAuraOfDecay::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.aura_of_decay.descr",
            "The caster exudes death and decay. Creatures within a "
            "distance of two steps take damage each standard turn."));

    descr.push_back(
        i18n::get("spells.aura_of_decay.dmg_prefix", "The spell deals ") +
        dmg_range(skill).str() +
        i18n::get(
            "spells.aura_of_decay.dmg_suffix",
            " damage to each creature."));

    if (skill == SpellSkill::transcendent) {
        descr.emplace_back(
            i18n::get(
                "spells.aura_of_decay.instant_kill_descr",
                "Any time a creature takes damage from the spell, "
                "they may be destroyed immediately (2% chance)."));
    }

    descr.push_back(spell_duration_descr(duration_range(skill).str()));

    return descr;
}

int SpellAuraOfDecay::mon_cooldown() const
{
    return 30;
}

bool SpellAuraOfDecay::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    return (
        !seen_targets.empty() &&
        !mon.m_properties.has(prop::Id::aura_of_decay));
}

// -----------------------------------------------------------------------------
// Bolt spells


// -----------------------------------------------------------------------------
// Pestilence
// -----------------------------------------------------------------------------
int SpellPestilence::mon_cooldown() const
{
    return 21;
}

std::string SpellPestilence::name() const
{
    return i18n::get("spells.pestilence.name", "Pestilence");
}

SpellId SpellPestilence::id() const
{
    return SpellId::pestilence;
}

SpellDomain SpellPestilence::domain() const
{
    return SpellDomain::corruption;
}

SpellShock SpellPestilence::shock_type() const
{
    return SpellShock::disturbing;
}

bool SpellPestilence::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellPestilence::nr_rats_summoned(SpellSkill skill) const
{
    if (skill == SpellSkill::transcendent) {
        return 3;
    }
    else {
        return 6 + (int)skill * 3;
    }
}

Range SpellPestilence::duration_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {8, 12};
    case SpellSkill::expert:       return {12, 16};
    // NOTE: On master level, the rats are hasted, meaning they disappear twice as fast from
    // the perspective of a normal speed player.
    case SpellSkill::master:       return {40, 60};
    case SpellSkill::transcendent: return {40, 60};
    }

    ASSERT(false);

    return {1, 1};
}

int SpellPestilence::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

void SpellPestilence::on_rat_summoned(
    actor::Actor* const mon,
    const SpellSkill skill) const
{
    {
        prop::Prop* prop = prop::make(prop::Id::summoned);
        const int duration = duration_range(skill).roll();
        prop->set_duration(duration);
        mon->m_properties.apply(prop);
    }

    {
        prop::Prop* prop = prop::make(prop::Id::waiting);
        prop->set_duration(1);
        mon->m_properties.apply(prop);
    }

    if (skill == SpellSkill::master) {
        prop::Prop* prop = prop::make(prop::Id::hasted);

        prop->set_indefinite();

        mon->m_properties.apply(prop, prop::PropSrc::intr, true, Verbose::no);
    }
}

void SpellPestilence::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    const size_t nr_mon = nr_rats_summoned(skill);

    actor::Actor* leader = nullptr;

    if (actor::is_player(caster)) {
        leader = caster;
    }
    else {
        // Caster is monster
        actor::Actor* const caster_leader = caster->m_leader;

        leader = caster_leader ? caster_leader : caster;
    }

    std::vector<std::pair<SpellSkill, std::string>> to_summon;

    if (skill == SpellSkill::transcendent) {
        // On transcendent level, spawn a bunch of normal rats as if on expert level, plus
        // some magical "transcendent rats".
        to_summon.emplace_back(SpellSkill::expert, "MON_RAT");

        to_summon.emplace_back(SpellSkill::transcendent, "MON_TRANSCENDENT_RAT");
    }
    else {
        to_summon.emplace_back(skill, "MON_RAT");
    }

    bool is_any_summoned = false;
    bool is_any_seen_by_player = false;

    for (const auto& summon_entry : to_summon) {
        const SpellSkill skill_to_use = summon_entry.first;
        const std::string id = summon_entry.second;

        const actor::MonSpawnResult mon_summoned =
            actor::spawn(
                caster->m_pos,
                {nr_mon, id},
                g_fov_radi_int,
                actor::SpawnScattered::yes)
                .make_aware_of_player()
                .set_leader(leader);

        is_any_summoned = !mon_summoned.monsters.empty() || is_any_summoned;

        is_any_seen_by_player =
            std::any_of(
                std::begin(mon_summoned.monsters),
                std::end(mon_summoned.monsters),
                [](auto* const mon) {
                    return actor::can_player_see_actor(*mon);
                }) ||
            is_any_seen_by_player;

        std::for_each(
            std::begin(mon_summoned.monsters),
            std::end(mon_summoned.monsters),
            [skill_to_use, this](auto& mon) {
                on_rat_summoned(mon, skill_to_use);
            });
    }

    if (!is_any_summoned) {
        return;
    }

    if (actor::is_player(caster) || is_any_seen_by_player) {
        msg_log::add(i18n::get("spells.rats_appear", "Rats appear!"));
    }
}

std::vector<std::string> SpellPestilence::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(i18n::get(
        "spells.pestilence.descr",
        "A pack of rats appear around the caster."));

    if (skill < SpellSkill::transcendent) {
        // Normal description (basic/expert/master).

        const size_t nr_mon = nr_rats_summoned(skill);

        const Range duration = duration_range(skill);

        descr.emplace_back(
            i18n::get("spells.pestilence.summons_prefix", "Summons ") +
            std::to_string(nr_mon) +
            i18n::get(
                "spells.pestilence.summons_middle",
                " rats. They exist for ") +
            duration.str() +
            i18n::get(
                "spells.pestilence.summons_suffix",
                " turns (their own turns)."));

        if (skill == SpellSkill::master) {
            descr.emplace_back(i18n::get(
                "spells.pestilence.hasted_rats",
                "The rats are Hasted (moves faster)."));
        }
    }
    else {
        // Transcendent description.

        descr.emplace_back(
            i18n::get(
                "spells.pestilence.transcendent_rats",
                "Some of the rats are ethereal "
                "(much harder to hit, can move through solid objects), "
                "are immune to magic, can cast spells, and have "
                "extra hit points and damage."));
    }

    return descr;
}

bool SpellPestilence::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    // Always allow casting with a visible target.
    if (!seen_targets.empty()) {
        return true;
    }

    // Sometimes allow casting if monster has an unseen target.
    if (mon.m_ai_state.target && rnd::one_in(30)) {
        return true;
    }

    return false;
}

// -----------------------------------------------------------------------------
// Mirror Images


// -----------------------------------------------------------------------------
// Curse
// -----------------------------------------------------------------------------
int SpellCurse::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 3;
}

std::string SpellCurse::name() const
{
    return i18n::get("spells.curse.name", "Curse");
}

SpellId SpellCurse::id() const
{
    return SpellId::curse;
}

SpellDomain SpellCurse::domain() const
{
    return SpellDomain::corruption;
}

SpellShock SpellCurse::shock_type() const
{
    return SpellShock::mild;
}

int SpellCurse::mon_cooldown() const
{
    return 10;
}

bool SpellCurse::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

Range SpellCurse::duration_range(const SpellSkill skill) const
{
    Range duration_range;
    duration_range.min = 15 * ((int)skill + 1);
    duration_range.max = duration_range.min * 2;

    return duration_range;
}

int SpellCurse::pct_chance_doom(SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:  return 5;
    case SpellSkill::expert: return 10;
    case SpellSkill::master:
    case SpellSkill::transcendent:
        // Not applicable.
        break;
    }

    ASSERT(false);

    return 0;
}

void SpellCurse::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    const int duration = duration_range(skill).roll();

    if (seen_targets.empty()) {
        if (actor::is_player(caster)) {
            // TODO: There needs to be a message here.
        }

        return;
    }

    // There are targets available

    const std::vector<actor::Actor*> targets =
        (skill == SpellSkill::basic)
        ? std::vector {rnd::element(seen_targets)}
        : seen_targets;

    auto prop_id = prop::Id::cursed;
    auto sfx_id = audio::SfxId::curse_spell;

    if ((skill >= SpellSkill::master) || rnd::percent(pct_chance_doom(skill))) {
        prop_id = prop::Id::doomed;
        sfx_id = audio::SfxId::doom_spell;
    }

    if (player_aware == PlayerAwareOfCast::yes) {
        if (can_player_see_caster_and_any_target(*caster, targets)) {
            audio::play(sfx_id);
        }

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

                // Run effect with the target as caster, and the
                // caster as seen target instead.
                run_effect(target, skill, {caster}, player_aware);
            }

            continue;
        }

        prop::Prop* const prop = prop::make(prop_id);

        prop->set_duration(duration);

        target->m_properties.apply(prop);
    }
}

std::vector<std::string> SpellCurse::descr_specific(SpellSkill skill) const
{
    std::vector<std::string> descr;

    const bool is_below_master = skill < SpellSkill::master;

    const prop::PropData& cursed_data = prop::g_data[(size_t)prop::Id::cursed];
    const prop::PropData& doomed_data = prop::g_data[(size_t)prop::Id::doomed];

    const prop::PropData& main_prop_data = is_below_master ? cursed_data : doomed_data;

    descr.emplace_back(
        i18n::get(
            "spells.curse.victims_prefix",
            "The spell's victims are ") +
        text_format::first_to_lower(main_prop_data.name) +
        i18n::get(
            "spells.curse.prop_open_paren",
            " (") +
        main_prop_data.descr +
        i18n::get(
            "spells.curse.close_paren",
            ")"));

    if (is_below_master) {
        descr.emplace_back(
            i18n::get(
                "spells.curse.doom_chance_prefix",
                "With ") +
            std::to_string(pct_chance_doom(skill)) +
            i18n::get(
                "spells.curse.doom_chance_middle",
                "% chance, the victims instead become ") +
            text_format::first_to_lower(doomed_data.name) +
            i18n::get(
                "spells.curse.prop_open_paren",
                " (") +
            doomed_data.descr +
            i18n::get(
                "spells.curse.close_paren",
                ")"));
    }

    descr.emplace_back(not_alerting_mon_descr());

    descr.emplace_back(
        skill == SpellSkill::basic
            ? i18n::get(
                  "spells.target.one_visible_hostile",
                  "Affects one random visible hostile creature.")
            : i18n::get(
                  "spells.target.all_visible_hostile",
                  "Affects all visible hostile creatures."));

    descr.emplace_back(spell_duration_descr(duration_range(skill).str()));

    return descr;
}

bool SpellCurse::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Poison


// -----------------------------------------------------------------------------
// Poison
// -----------------------------------------------------------------------------
int SpellPoison::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 6;
}

std::string SpellPoison::name() const
{
    return i18n::get("spells.poison.name", "Poison");
}

SpellId SpellPoison::id() const
{
    return SpellId::poison;
}

SpellDomain SpellPoison::domain() const
{
    return SpellDomain::corruption;
}

SpellShock SpellPoison::shock_type() const
{
    return SpellShock::mild;
}

int SpellPoison::mon_cooldown() const
{
    return 3;
}

bool SpellPoison::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

Range SpellPoison::duration_range(const SpellSkill skill) const
{
    Range duration_range;
    duration_range.min = 15 * ((int)skill + 1);
    duration_range.max = duration_range.min * 2;

    return duration_range;
}

void SpellPoison::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    const int duration = duration_range(skill).roll();

    if (seen_targets.empty()) {
        if (actor::is_player(caster)) {
            // TODO: There needs to be a message here.
        }

        return;
    }

    // There are targets available

    const std::vector<actor::Actor*> targets =
        (skill == SpellSkill::basic)
        ? std::vector {rnd::element(seen_targets)}
        : seen_targets;

    if (player_aware == PlayerAwareOfCast::yes) {
        if (can_player_see_caster_and_any_target(*caster, targets)) {
            audio::play(audio::SfxId::poison_spell);
        }

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

                // Run effect with the target as caster, and the
                // caster as seen target instead.
                run_effect(target, skill, {caster}, player_aware);
            }

            continue;
        }

        auto id = prop::Id::poisoned;

        prop::Prop* const prop = prop::make(id);

        prop->set_duration(duration);

        target->m_properties.apply(prop);

        if (!actor::is_player(target)) {
            target->become_aware_player(actor::AwareSource::spell_victim);
        }
    }
}

std::vector<std::string> SpellPoison::descr_specific(SpellSkill skill) const
{
    std::vector<std::string> descr;

    const prop::PropData& prop_data = prop::g_data[(size_t)prop::Id::poisoned];

    descr.emplace_back(
        i18n::get(
            "spells.poison.victims_prefix",
            "The spell's victims are ") +
        text_format::first_to_lower(prop_data.name) +
        i18n::get(
            "spells.poison.prop_open_paren",
            " (") +
        prop_data.descr +
        i18n::get(
            "spells.poison.close_paren",
            ")"));

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

bool SpellPoison::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Heal Others


// -----------------------------------------------------------------------------
// Enfeeble
// -----------------------------------------------------------------------------
Range SpellEnfeeble::duration_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {8, 12};
    case SpellSkill::expert:       return {10, 16};
    case SpellSkill::master:       return {12, 20};
    case SpellSkill::transcendent: return {30, 50};
    }

    ASSERT(false);

    return {1, 1};
}

SpellId SpellEnfeeble::id() const
{
    return SpellId::enfeeble;
}

SpellDomain SpellEnfeeble::domain() const
{
    return SpellDomain::corruption;
}

SpellShock SpellEnfeeble::shock_type() const
{
    return SpellShock::mild;
}

bool SpellEnfeeble::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

std::string SpellEnfeeble::name() const
{
    return i18n::get("spells.enfeeble.name", "Enfeeble");
}

int SpellEnfeeble::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 5;
}

int SpellEnfeeble::mon_cooldown() const
{
    return 5;
}

void SpellEnfeeble::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    const int duration = duration_range(skill).roll();

    if (seen_targets.empty()) {
        if (actor::is_player(caster)) {
            msg_log::add(i18n::get(
                "spells.bugs_move_feebly",
                "The bugs on the ground suddenly move very feebly."));
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

                // Run effect with the target as caster, and the
                // caster as seen target instead.
                run_effect(target, skill, {caster}, player_aware);
            }

            continue;
        }

        prop::Prop* const prop = prop::make(prop::Id::weakened);

        prop->set_duration(duration);

        target->m_properties.apply(prop);
    }
}

std::vector<std::string> SpellEnfeeble::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.enfeeble.descr",
            "Physically enfeebles the spell's victims, causing them to "
            "only do half damage in melee combat."));

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

bool SpellEnfeeble::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Temporal Echo

