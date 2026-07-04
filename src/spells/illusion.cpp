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
// Mirror Images
// -----------------------------------------------------------------------------
std::string SpellMirrorImages::name() const
{
    return i18n::get("spells.mirror_images.name", "Mirror Images");
}

SpellId SpellMirrorImages::id() const
{
    return SpellId::mirror_images;
}

SpellDomain SpellMirrorImages::domain() const
{
    return SpellDomain::illusion;
}

SpellShock SpellMirrorImages::shock_type() const
{
    return SpellShock::mild;
}

bool SpellMirrorImages::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellMirrorImages::nr_mirror_images_summoned(SpellSkill skill) const
{
    if (skill == SpellSkill::transcendent) {
        return 6;
    }
    else {
        return 2 + (int)skill;
    }
}

Range SpellMirrorImages::duration_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {8, 12};
    case SpellSkill::expert:       return {12, 16};
    case SpellSkill::master:       return {16, 20};
    case SpellSkill::transcendent: return {40, 60};
    }

    ASSERT(false);

    return {1, 1};
}

int SpellMirrorImages::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 5;
}

void SpellMirrorImages::on_mirror_image_summoned(
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
}

void SpellMirrorImages::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    ASSERT(actor::is_player(caster));

    const size_t nr_mon = nr_mirror_images_summoned(skill);

    const std::string id = "MON_MIRROR_IMAGE";

    const actor::MonSpawnResult mon_summoned =
        actor::spawn(
            caster->m_pos,
            {nr_mon, id},
            g_fov_radi_int,
            actor::SpawnScattered::no)
            .make_aware_of_player()
            .set_leader(map::g_player);

    std::for_each(
        std::begin(mon_summoned.monsters),
        std::end(mon_summoned.monsters),
        [skill, this](auto& mon) {
            on_mirror_image_summoned(mon, skill);
        });

    if (mon_summoned.monsters.empty()) {
        return;
    }

    draw_blast_at_seen_actors(mon_summoned.monsters, colors::magenta());

    msg_log::add(i18n::get("spells.images_appear", "Images appear!"));
}

std::vector<std::string> SpellMirrorImages::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.mirror_images.descr",
            "Conjures illusory duplicates of the caster "
            "to mislead enemies and draw their attacks."));

    descr.emplace_back(
        i18n::get(
            "spells.mirror_images.presence_descr",
            "The mirror images project a powerful magical presence, "
            "causing attackers to prefer them over the caster. "
            "As magical apparitions rather than living creatures, "
            "they are extremely difficult to strike with conventional attacks. "
            "They are immune to elemental damage and largely unaffected by physical "
            "or mental afflictions."));

    const size_t nr_mon = nr_mirror_images_summoned(skill);

    const Range duration = duration_range(skill);

    descr.emplace_back(
        i18n::get("spells.mirror_images.creates_prefix", "Creates ") +
        std::to_string(nr_mon) +
        i18n::get(
            "spells.mirror_images.creates_middle",
            " mirror images. They exist for ") +
        duration.str() +
        i18n::get(
            "spells.mirror_images.creates_suffix",
            " turns (their own turns)."));

    return descr;
}

// -----------------------------------------------------------------------------
// Projected Strike


// -----------------------------------------------------------------------------
// Invisibility
// -----------------------------------------------------------------------------
std::string SpellInvis::name() const
{
    return i18n::get("spells.invisibility.name", "Invisibility");
}

SpellId SpellInvis::id() const
{
    return SpellId::invis;
}

SpellDomain SpellInvis::domain() const
{
    return SpellDomain::illusion;
}

bool SpellInvis::is_tenebrous() const
{
    return true;
}

SpellShock SpellInvis::shock_type() const
{
    return SpellShock::mild;
}

int SpellInvis::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 8;
}

Range SpellInvis::duration_range(SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {4, 6};
    case SpellSkill::expert:       return {5, 7};
    case SpellSkill::master:       return {6, 8};
    case SpellSkill::transcendent: return {8, 10};
    }

    ASSERT(false);

    return {1, 1};
}

bool SpellInvis::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

void SpellInvis::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    const prop::Id prop_id = (skill == SpellSkill::basic) ? prop::Id::cloaked : prop::Id::invis;

    prop::Prop* const prop = prop::make(prop_id);

    prop->set_duration(duration_range(skill).roll());

    caster->m_properties.apply(prop);
}

std::vector<std::string> SpellInvis::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.invisibility.descr_main",
            "Makes the caster invisible to normal vision for a "
            "brief time."));

    if (skill == SpellSkill::basic) {
        descr.emplace_back(
            i18n::get(
                "spells.invisibility.descr_basic",
                "Attacking or casting spells reveals the caster."));
    }
    else {
        descr.emplace_back(
            i18n::get(
                "spells.invisibility.descr_advanced",
                "The caster is truly invisible for the duration of "
                "the the spell, and can freely attack or cast "
                "spells without breaking the invisibility."));
    }

    descr.push_back(spell_duration_descr(duration_range(skill).str()));

    return descr;
}

// -----------------------------------------------------------------------------
// See Invisible


// -----------------------------------------------------------------------------
// Terrify
// -----------------------------------------------------------------------------
std::string SpellTerrify::name() const
{
    return i18n::get("spells.terrify.name", "Terrify");
}

SpellId SpellTerrify::id() const
{
    return SpellId::terrify;
}

SpellDomain SpellTerrify::domain() const
{
    return SpellDomain::illusion;
}

SpellShock SpellTerrify::shock_type() const
{
    return SpellShock::disturbing;
}

bool SpellTerrify::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

Range SpellTerrify::duration_range(SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {6, 12};
    case SpellSkill::expert:       return {12, 24};
    case SpellSkill::master:       return {18, 36};
    case SpellSkill::transcendent: return {24, 48};
    }

    ASSERT(false);

    return {1, 1};
}

int SpellTerrify::faint_pct_chance(const SpellSkill skill) const
{
    if (skill == SpellSkill::transcendent) {
        return 100;
    }
    else {
        return 30 + ((int)skill * 20);
    }
}

Range SpellTerrify::faint_duration_range() const
{
    return {2, 4};
}

int SpellTerrify::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 5;
}

int SpellTerrify::mon_cooldown() const
{
    return 5;
}

void SpellTerrify::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    if (seen_targets.empty()) {
        if (actor::is_player(caster)) {
            msg_log::add(i18n::get(
                "spells.bugs_scatter_away",
                "The bugs on the ground suddenly scatter away."));
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

        terrify_target(*target, skill);

        // NOTE: Since this fainting is supposed to be a side effect of the creature becoming
        // terrified by a spell, it would look weird if they "reisted" the sleep due to sleep
        // resistance. Therefore only try to apply the property if they are known to not have such
        // resistance. Any other sources of resisting sleep would probably be fine, but not
        // explicitly sleep resistance.
        if (target->m_properties.has(prop::Id::terrified) &&
            !target->m_properties.has(prop::Id::r_sleep) &&
            rnd::percent(faint_pct_chance(skill))) {
            faint_target(*target);
        }
    }
}

void SpellTerrify::terrify_target(actor::Actor& target, const SpellSkill skill) const
{
    prop::Prop* const terrified = prop::make(prop::Id::terrified);

    terrified->set_duration(duration_range(skill).roll());

    target.m_properties.apply(terrified);
}

void SpellTerrify::faint_target(actor::Actor& target) const
{
    prop::Prop* const fainted = prop::make(prop::Id::fainted);

    fainted->set_duration(faint_duration_range().roll());

    target.m_properties.apply(fainted);
}

std::vector<std::string> SpellTerrify::descr_specific(
    const SpellSkill skill) const
{
    (void)skill;

    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.terrify.descr",
            "Inflicts a nightmare illusion that overwhelms its victims with dread."));

    descr.emplace_back(
        skill == SpellSkill::basic
            ? i18n::get(
                "spells.target.one_visible_hostile",
                "Affects one random visible hostile creature.")
            : i18n::get(
                "spells.target.all_visible_hostile",
                "Affects all visible hostile creatures."));

    descr.push_back(spell_duration_descr(duration_range(skill).str()));

    if (skill == SpellSkill::transcendent) {
        descr.emplace_back(
            i18n::get(
                "spells.terrify.descr_transcendent",
                "Affected creatures also faint."));
    }
    else {
        const std::string creature_str =
            (skill == SpellSkill::basic)
            ? i18n::get("spells.terrify.creature_singular", "creature")
            : i18n::get("spells.terrify.creature_plural", "creatures");

        descr.emplace_back(
            i18n::get(
                "spells.terrify.faint_chance_prefix",
                "Has a ") +
            std::to_string(faint_pct_chance(skill)) +
            i18n::get(
                "spells.terrify.faint_chance_middle",
                "% chance to also make affected ") +
            creature_str +
            i18n::get(
                "spells.terrify.faint_chance_suffix",
                " faint."));
    }

    return descr;
}

bool SpellTerrify::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Threat Projection


// -----------------------------------------------------------------------------
// Threat Projection
// -----------------------------------------------------------------------------
std::string SpellThreatProjection::name() const
{
    return i18n::get("spells.threat_projection.name", "Threat Projection");
}

SpellId SpellThreatProjection::id() const
{
    return SpellId::threat_projection;
}

SpellDomain SpellThreatProjection::domain() const
{
    return SpellDomain::illusion;
}

SpellShock SpellThreatProjection::shock_type() const
{
    return SpellShock::mild;
}

bool SpellThreatProjection::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

Range SpellThreatProjection::duration_range(SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {3, 6};
    case SpellSkill::expert:       return {6, 12};
    // NOTE: Same as the Horn of Malice:
    case SpellSkill::master:       return {8, 24};
    case SpellSkill::transcendent: return {16, 48};
    }

    ASSERT(false);

    return {1, 1};
}

int SpellThreatProjection::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

void SpellThreatProjection::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    if (seen_targets.empty()) {
        if (actor::is_player(caster)) {
            msg_log::add(i18n::get(
                "spells.bugs_attack_each_other",
                "The bugs on the ground all start to attack each other."));
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

        conflict_target(*target, skill);
    }
}

void SpellThreatProjection::conflict_target(actor::Actor& target, const SpellSkill skill) const
{
    prop::Prop* const conflicted = prop::make(prop::Id::conflict);

    conflicted->set_duration(duration_range(skill).roll());

    target.m_properties.apply(conflicted);
}

std::vector<std::string> SpellThreatProjection::descr_specific(
    const SpellSkill skill) const
{
    (void)skill;

    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.threat_projection.descr",
            "Distorts the perception of the spell's victims, causing "
            "all other creatures to be misidentified as enemies."));

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

// -----------------------------------------------------------------------------
// Disease

