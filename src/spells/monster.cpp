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
// Knockback
// -----------------------------------------------------------------------------
int SpellKnockBack::mon_cooldown() const
{
    return 5;
}

std::string SpellKnockBack::name() const
{
    return i18n::get("spells.knockback.name", "Knockback");
}

SpellId SpellKnockBack::id() const
{
    return SpellId::knockback;
}

SpellDomain SpellKnockBack::domain() const
{
    return SpellDomain::END;
}

SpellShock SpellKnockBack::shock_type() const
{
    return SpellShock::mild;
}

std::vector<std::string> SpellKnockBack::descr_specific(const SpellSkill skill) const
{
    (void)skill;

    return {};
}

int SpellKnockBack::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 8;
}

bool SpellKnockBack::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

void SpellKnockBack::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)skill;

    actor::Actor* target = map::random_closest_actor(caster->m_pos, seen_targets);

    if (!target) {
        ASSERT(false);

        return;
    }

    // Spell resistance?
    if (target->m_properties.has(prop::Id::r_spell)) {
        on_resist(*target);

        // Spell reflection?
        if (target->m_properties.has(prop::Id::spell_reflect)) {
            if (actor::can_player_see_actor(*target)) {
                msg_log::add(spell_reflect_msg());
            }

            // Run effect with the target as caster, and the caster as seen target.
            run_effect(target, skill, {caster}, player_aware);
        }

        return;
    }

    std::string target_str;
    Color msg_clr;

    if (actor::is_player(target)) {
        target_str = i18n::get("spells.me", "me");

        msg_clr = colors::msg_bad();
    }
    else {
        // Target is monster
        target_str = actor::name_the(*target);

        msg_clr = map::g_player->is_leader_of(target) ? colors::white() : colors::msg_good();
    }

    if (actor::can_player_see_actor(*target)) {
        msg_log::add(
            i18n::get("spells.force_pushes_prefix", "A force pushes ") +
                target_str +
                i18n::get("spells.force_pushes_suffix", "!"),
            msg_clr);
    }

    knockback::run(
        *target,
        caster->m_pos,
        knockback::KnockbackSource::other);

    if (!actor::is_player(target)) {
        target->become_aware_player(actor::AwareSource::spell_victim);
    }
}

bool SpellKnockBack::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Curse


// -----------------------------------------------------------------------------
// Heal Others
// -----------------------------------------------------------------------------
int SpellHealOthers::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 6;
}

std::string SpellHealOthers::name() const
{
    return i18n::get("spells.heal_others.name", "Heal Others");
}

SpellId SpellHealOthers::id() const
{
    return SpellId::heal_others;
}

SpellDomain SpellHealOthers::domain() const
{
    return SpellDomain::END;
}

SpellShock SpellHealOthers::shock_type() const
{
    return SpellShock::mild;
}

std::vector<std::string> SpellHealOthers::descr_specific(SpellSkill skill) const
{
    (void)skill;
    return {};
}

bool SpellHealOthers::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellHealOthers::mon_cooldown() const
{
    return 10;
}

std::vector<actor::Actor*> SpellHealOthers::find_possible_actors_to_heal(
    const actor::Actor* const caster) const
{
    const std::vector<actor::Actor*> allies = actor::other_allied_actors(caster);

    Array2<bool> blocks_los(map::dims());

    const R r = fov::fov_rect(caster->m_pos, blocks_los.dims());

    map_parsers::BlocksLos().run(blocks_los, r, MapParseMode::overwrite);

    std::vector<actor::Actor*> actors_to_heal;

    std::copy_if(
        std::begin(allies),
        std::end(allies),
        std::back_inserter(actors_to_heal),
        [caster, blocks_los](const actor::Actor* const actor) {
            const bool can_see = can_mon_see_actor(*caster, *actor, blocks_los);

            const int hp = actor->m_hp;
            const int max_hp = actor::max_hp(*actor);

            const bool is_healing_needed = (hp < ((max_hp * 3) / 4));

            return can_see && is_healing_needed;
        });

    return actors_to_heal;
}

actor::Actor* SpellHealOthers::find_random_actor_to_heal(
    const actor::Actor* caster) const
{
    const std::vector<actor::Actor*> actors = find_possible_actors_to_heal(caster);

    if (actors.empty()) {
        ASSERT(false);

        return nullptr;
    }
    else {
        return rnd::element(actors);
    }
}

void SpellHealOthers::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    const int hp_healed = 8 + (int)skill * 4;

    actor::Actor* const actor_to_heal = find_random_actor_to_heal(caster);

    if (!actor_to_heal) {
        ASSERT(false);

        return;
    }

    actor::restore_hp(*actor_to_heal, hp_healed);
}

bool SpellHealOthers::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)seen_targets;

    return !find_possible_actors_to_heal(&mon).empty();
}

// -----------------------------------------------------------------------------
// Enfeeble


// -----------------------------------------------------------------------------
// Disease
// -----------------------------------------------------------------------------
int SpellDisease::mon_cooldown() const
{
    return 10;
}

std::string SpellDisease::name() const
{
    return i18n::get("spells.disease.name", "Disease");
}

SpellId SpellDisease::id() const
{
    return SpellId::disease;
}

SpellDomain SpellDisease::domain() const
{
    return SpellDomain::END;
}

SpellShock SpellDisease::shock_type() const
{
    return SpellShock::disturbing;
}

std::vector<std::string> SpellDisease::descr_specific(const SpellSkill skill) const
{
    (void)skill;

    return {};
}

int SpellDisease::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

bool SpellDisease::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

void SpellDisease::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    actor::Actor* target = map::random_closest_actor(caster->m_pos, seen_targets);

    if (!target) {
        ASSERT(false);

        return;
    }

    // Spell resistance?
    if (target->m_properties.has(prop::Id::r_spell)) {
        on_resist(*target);

        // Spell reflection?
        if (target->m_properties.has(prop::Id::spell_reflect)) {
            if (actor::can_player_see_actor(*target)) {
                msg_log::add(spell_reflect_msg());
            }

            // Run effect with the target as caster, and the caster as seen target.
            run_effect(target, skill, {caster}, player_aware);
        }

        return;
    }

    if (actor::can_player_see_actor(*target)) {
        const std::string actor_name =
            actor::is_player(target)
            ? i18n::get("spells.me", "me")
            : actor::name_the(*target);

        msg_log::add(
            i18n::get(
                "spells.disease.afflict_prefix",
                "A horrible disease is starting to afflict ") +
            actor_name +
            i18n::get(
                "spells.exclamation",
                "!"));
    }

    target->m_properties.apply(prop::make(prop::Id::diseased));
}

bool SpellDisease::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Blind


// -----------------------------------------------------------------------------
// Blind
// -----------------------------------------------------------------------------
std::string SpellBlind::name() const
{
    return i18n::get("spells.blind.name", "Blind");
}

SpellId SpellBlind::id() const
{
    return SpellId::blind;
}

SpellDomain SpellBlind::domain() const
{
    return SpellDomain::END;
}

SpellShock SpellBlind::shock_type() const
{
    return SpellShock::disturbing;
}

std::vector<std::string> SpellBlind::descr_specific(const SpellSkill skill) const
{
    (void)skill;

    return {};
}

int SpellBlind::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

bool SpellBlind::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellBlind::mon_cooldown() const
{
    return 20;
}

void SpellBlind::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    const std::vector<actor::Actor*> seen_targets_not_blind_resistant =
        find_actors_not_blind_resistant(seen_targets);

    actor::Actor* target =
        map::random_closest_actor(
            caster->m_pos,
            seen_targets_not_blind_resistant);

    if (!target) {
        // TODO: Consider this if spell reflection is updated to work differently.

        // NOTE: This is a legitimate case (e.g. player with spell reflection redirects the
        // spell to a monster with blind resistance).

        return;
    }

    // Spell resistance?
    if (target->m_properties.has(prop::Id::r_spell)) {
        on_resist(*target);

        // Spell reflection?
        if (target->m_properties.has(prop::Id::spell_reflect)) {
            if (actor::can_player_see_actor(*target)) {
                msg_log::add(spell_reflect_msg());
            }

            // Run effect with the target as caster, and the caster as seen target.
            run_effect(target, skill, {caster}, player_aware);
        }

        return;
    }

    if (actor::is_player(target)) {
        msg_log::add(i18n::get(
            "spells.scales_grow_over_my_eyes",
            "Scales grow over my eyes!"));
    }
    else if (actor::can_player_see_actor(*target)) {
        const std::string actor_name = actor::name_the(*target);

        msg_log::add(
            i18n::get(
                "spells.scales_grow_over_eyes_prefix",
                "Scales grow over the eyes of ") +
            actor_name +
            i18n::get("spells.period", "."));
    }

    prop::Prop* prop = prop::make(prop::Id::blind);

    prop->set_duration(3 + (int)skill);

    target->m_properties.apply(prop);
}

bool SpellBlind::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !find_actors_not_blind_resistant(seen_targets).empty();
}

std::vector<actor::Actor*> SpellBlind::find_actors_not_blind_resistant(
    const std::vector<actor::Actor*>& actors) const
{
    std::vector<actor::Actor*> result;

    result.reserve(actors.size());

    std::copy_if(
        std::begin(actors),
        std::end(actors),
        std::back_inserter(result),
        [](const actor::Actor* const actor) {
            return !actor->m_properties.has(prop::Id::r_blind);
        });

    return result;
}

// -----------------------------------------------------------------------------
// Summon spells


// -----------------------------------------------------------------------------
// Summon spells
// -----------------------------------------------------------------------------
int SpellSummon::mon_cooldown() const
{
    return 8;
}

std::string SpellSummon::name() const
{
    return "";
}

SpellId SpellSummon::id() const
{
    return m_impl->id();
}

SpellDomain SpellSummon::domain() const
{
    return SpellDomain::END;
}

SpellShock SpellSummon::shock_type() const
{
    return SpellShock::disturbing;
}

std::vector<std::string> SpellSummon::descr_specific(const SpellSkill skill) const
{
    (void)skill;

    return {};
}

int SpellSummon::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 6;
}

bool SpellSummon::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

void SpellSummon::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    Range mon_lvl_range = get_allowed_mon_lvl_range(skill);

    TRACE
        << "Allowed monster level range: "
        << "'" << mon_lvl_range.str() << "'"
        << "\n";

    std::vector<std::string> summon_bucket = make_summon_bucket(mon_lvl_range);

    if (summon_bucket.empty()) {
        TRACE
            << "No eligible monsters found, trying again with monsters allowed "
               "from depth 0."
            << "\n";

        mon_lvl_range.min = 0;

        TRACE
            << "Allowed monster dungeon level range: "
            << "'" << mon_lvl_range.str() << "'"
            << "\n";

        summon_bucket = make_summon_bucket(mon_lvl_range);
    }

    if (summon_bucket.empty()) {
        TRACE << "No elligible monsters found for spawning" << "\n";

        ASSERT(false);

        return;
    }

    const auto id = rnd::element(summon_bucket);

    summon(id, caster);
}

Range SpellSummon::get_allowed_mon_lvl_range(const SpellSkill skill) const
{
    Range dlvl_range;

    switch (skill) {
    case SpellSkill::basic:
        dlvl_range.min = 0;
        dlvl_range.max = g_dlvl_last_early_game;
        break;

    case SpellSkill::expert:
        dlvl_range.min = 0;
        dlvl_range.max = g_dlvl_last_mid_game;
        break;

    case SpellSkill::master:
    case SpellSkill::transcendent:
        dlvl_range.min = g_dlvl_first_mid_game;
        dlvl_range.max = g_dlvl_last;
        break;
    }

    // Cap min and max to current dungeon level + 2
    const int dlvl = map::g_dlvl + 2;

    dlvl_range.min = std::min(dlvl_range.min, dlvl);
    dlvl_range.max = std::min(dlvl_range.max, dlvl);

    return dlvl_range;
}

std::vector<std::string> SpellSummon::make_summon_bucket(const Range& lvl_range) const
{
    std::vector<std::string> summon_bucket;

    for (auto& it : actor::g_data) {
        const actor::ActorData& data = it.second;

        if (!data.can_be_summoned_by_mon) {
            continue;
        }

        // NOTE: The "min" dungeon level in the monster data is used here as a general
        // "strength" of the monster. The "max" dungeon level is not considered.
        const int mon_lvl = data.spawn_min_dlvl;

        if (!lvl_range.is_in_range(mon_lvl)) {
            continue;
        }

        summon_bucket.push_back(data.id);
    }

    TRACE
        << "Number of monsters allowed before specific filtering: "
        << "'" << summon_bucket.size() << "'"
        << "\n";

    summon_bucket = m_impl->filter_allowed_ids(summon_bucket);

    TRACE
        << "Number of monsters allowed after specific filtering: "
        << "'" << summon_bucket.size() << "'"
        << "\n";

    return summon_bucket;
}

void SpellSummon::summon(const std::string& id, actor::Actor* caster) const
{
    actor::Actor* const caster_leader = caster->m_leader;

    actor::Actor* const leader = caster_leader ? caster_leader : caster;

    const actor::MonSpawnResult summoned =
        actor::spawn(caster->m_pos, {id})
            .make_aware_of_player()
            .set_leader(leader);

    std::for_each(
        std::begin(summoned.monsters),
        std::end(summoned.monsters),
        [](auto* const mon) {
            mon->m_properties.apply(prop::make(prop::Id::summoned));

            prop::Prop* prop_waiting = prop::make(prop::Id::waiting);

            prop_waiting->set_duration(2);

            mon->m_properties.apply(prop_waiting);
        });

    if (summoned.monsters.empty()) {
        return;
    }

    actor::Actor* const mon = summoned.monsters[0];

    if (actor::can_player_see_actor(*mon)) {
        std::string appear_msg = m_impl->appear_msg_override();

        if (appear_msg.empty()) {
            const std::string mon_name_a =
                text_format::first_to_upper(
                    actor::name_a(*mon));

            appear_msg =
                mon_name_a +
                i18n::get("spells.appears_suffix", " appears!");
        }

        msg_log::add(appear_msg);

        actor::make_player_aware_mon(*mon);
    }
}

bool SpellSummon::allow_mon_cast_now(
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

SpellId SummonRandom::id() const
{
    return SpellId::summon_random;
}

int SummonImpl::mon_cooldown() const
{
    return 8;
}

std::string SummonImpl::appear_msg_override() const
{
    return "";
}

std::vector<std::string> SummonRandom::filter_allowed_ids(
    const std::vector<std::string>& summon_bucket) const
{
    // No specific filtering.
    return summon_bucket;
}

SpellId SummonWaterCreature::id() const
{
    return SpellId::summon_water_creature;
}

std::vector<std::string> SummonWaterCreature::filter_allowed_ids(
    const std::vector<std::string>& summon_bucket) const
{
    // Return all creatures with the "water creature" property.
    std::vector<std::string> result;

    std::copy_if(
        std::cbegin(summon_bucket),
        std::cend(summon_bucket),
        std::back_inserter(result),
        [](const std::string& id) {
            const actor::ActorData& data = actor::g_data.at(id);

            return data.natural_props[(size_t)prop::Id::water_creature];
        });

    return result;
}

SpellId SummonTentacles::id() const
{
    return SpellId::summon_tentacles;
}

int SummonTentacles::mon_cooldown() const
{
    return 3;
}

std::vector<std::string> SummonTentacles::filter_allowed_ids(
    const std::vector<std::string>& summon_bucket) const
{
    (void)summon_bucket;

    return {"MON_TENTACLE_CLUSTER"};
}

std::string SummonTentacles::appear_msg_override() const
{
    return i18n::get(
        "spells.summon_tentacles.appear_msg",
        "Monstrous tentacles rise up from the ground!");
}

// -----------------------------------------------------------------------------
// Heal


// -----------------------------------------------------------------------------
// Mi-Go hypnosis
// -----------------------------------------------------------------------------
int SpellMiGoHypno::mon_cooldown() const
{
    return 5;
}

std::string SpellMiGoHypno::name() const
{
    return i18n::get("spells.migo_hypnosis.name", "MiGo Hypnosis");
}

SpellId SpellMiGoHypno::id() const
{
    return SpellId::mi_go_hypno;
}

SpellDomain SpellMiGoHypno::domain() const
{
    return SpellDomain::END;
}

SpellShock SpellMiGoHypno::shock_type() const
{
    return SpellShock::mild;
}

std::vector<std::string> SpellMiGoHypno::descr_specific(const SpellSkill skill) const
{
    (void)skill;

    return {};
}

int SpellMiGoHypno::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

bool SpellMiGoHypno::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

void SpellMiGoHypno::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)skill;
    (void)player_aware;

    actor::Actor* target = map::random_closest_actor(caster->m_pos, seen_targets);

    if (!target) {
        ASSERT(false);

        return;
    }

    // Spell resistance?
    if (target->m_properties.has(prop::Id::r_spell)) {
        on_resist(*target);

        // Spell reflection?
        if (target->m_properties.has(prop::Id::spell_reflect)) {
            if (actor::can_player_see_actor(*target)) {
                msg_log::add(spell_reflect_msg());
            }

            // Run effect with the target as caster, and the caster as seen target.
            run_effect(target, skill, {caster}, player_aware);
        }

        return;
    }

    if (actor::is_player(target)) {
        msg_log::add(i18n::get(
            "spells.sharp_droning",
            "There is a sharp droning in my head!"));
    }

    if (rnd::coin_toss()) {
        spells::run_mi_go_hypno_effect(*target);
    }
    else {
        if (actor::is_player(target)) {
            msg_log::add(i18n::get("spells.feel_dizzy", "I feel dizzy."));
        }
    }
}

bool SpellMiGoHypno::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Immolation


// -----------------------------------------------------------------------------
// Immolation
// -----------------------------------------------------------------------------
int SpellBurn::mon_cooldown() const
{
    return 9;
}

std::string SpellBurn::name() const
{
    return i18n::get("spells.immolation.name", "Immolation");
}

SpellId SpellBurn::id() const
{
    return SpellId::burn;
}

SpellDomain SpellBurn::domain() const
{
    return SpellDomain::END;
}

SpellShock SpellBurn::shock_type() const
{
    return SpellShock::disturbing;
}

std::vector<std::string> SpellBurn::descr_specific(const SpellSkill skill) const
{
    (void)skill;

    return {};
}

int SpellBurn::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

bool SpellBurn::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

void SpellBurn::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)player_aware;

    actor::Actor* target = map::random_closest_actor(caster->m_pos, seen_targets);

    if (!target) {
        ASSERT(false);

        return;
    }

    // Spell resistance?
    if (target->m_properties.has(prop::Id::r_spell)) {
        on_resist(*target);

        // Spell reflection?
        if (target->m_properties.has(prop::Id::spell_reflect)) {
            if (actor::can_player_see_actor(*target)) {
                msg_log::add(spell_reflect_msg());
            }

            // Run effect with the target as caster, and the caster as seen target.
            run_effect(target, skill, {caster}, player_aware);
        }

        return;
    }

    if (actor::can_player_see_actor(*target)) {
        const std::string actor_name =
            actor::is_player(target)
            ? i18n::get("spells.me", "me")
            : actor::name_the(*target);

        msg_log::add(
            i18n::get(
                "spells.flames_rising_prefix",
                "Flames are rising around ") +
            actor_name +
            i18n::get("spells.flames_rising_suffix", "!"));
    }

    prop::Prop* prop = prop::make(prop::Id::burning);

    prop->set_duration(2 + (int)skill);

    target->m_properties.apply(prop);

    if (!actor::is_player(target)) {
        target->become_aware_player(actor::AwareSource::spell_victim);
    }
}

bool SpellBurn::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Deafening


// -----------------------------------------------------------------------------
// Deafening
// -----------------------------------------------------------------------------
int SpellDeafen::mon_cooldown() const
{
    return 5;
}

std::string SpellDeafen::name() const
{
    return i18n::get("spells.deafen.name", "Deafen");
}

SpellId SpellDeafen::id() const
{
    return SpellId::deafen;
}

SpellDomain SpellDeafen::domain() const
{
    return SpellDomain::END;
}

SpellShock SpellDeafen::shock_type() const
{
    return SpellShock::mild;
}

std::vector<std::string> SpellDeafen::descr_specific(const SpellSkill skill) const
{
    (void)skill;

    return {};
}

int SpellDeafen::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 4;
}

bool SpellDeafen::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

void SpellDeafen::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    actor::Actor* target = map::random_closest_actor(caster->m_pos, seen_targets);

    if (!target) {
        ASSERT(false);

        return;
    }

    // Spell resistance?
    if (target->m_properties.has(prop::Id::r_spell)) {
        on_resist(*target);

        // Spell reflection?
        if (target->m_properties.has(prop::Id::spell_reflect)) {
            if (actor::can_player_see_actor(*target)) {
                msg_log::add(spell_reflect_msg());
            }

            // Run effect with the target as caster, and the caster as seen target.
            run_effect(target, skill, {caster}, player_aware);
        }

        return;
    }

    prop::Prop* prop = prop::make(prop::Id::deaf);

    prop->set_duration(75 + (int)skill * 75);

    target->m_properties.apply(prop);
}

bool SpellDeafen::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Transmutation

