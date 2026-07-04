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
// Projected Strike
// -----------------------------------------------------------------------------
std::string SpellProjectedStrike::name() const
{
    return i18n::get("spells.projected_strike.name", "Projected Strike");
}

SpellId SpellProjectedStrike::id() const
{
    return SpellId::projected_strike;
}

SpellDomain SpellProjectedStrike::domain() const
{
    return SpellDomain::mind;
}

SpellShock SpellProjectedStrike::shock_type() const
{
    return SpellShock::mild;
}

bool SpellProjectedStrike::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellProjectedStrike::max_nr_weapons(const SpellSkill skill) const
{
    if (skill == SpellSkill::transcendent) {
        return -1;
    }
    else {
        return 3 + ((int)skill * 3);
    }
}

int SpellProjectedStrike::hit_chance_bonus(SpellSkill skill) const
{
    return 10 * ((int)skill + 1);
}

int SpellProjectedStrike::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 6;
}

std::vector<const item::Item*> SpellProjectedStrike::get_weapons(SpellSkill skill) const
{
    auto is_melee_wpn = [](const auto* const item) {
        return item && (item->data().type == ItemType::melee_wpn);
    };

    std::vector<const item::Item*> weapons;

    // Assuming the caster is always the player.
    for (const auto& slot : map::g_player->m_inv.m_slots) {
        if (is_melee_wpn(slot.item)) {
            weapons.push_back(slot.item);
        }
    }

    for (const auto& item : map::g_player->m_inv.m_backpack) {
        if (is_melee_wpn(item)) {
            weapons.push_back(item);
        }
    }

    // Cap the number of weapons spawned
    rnd::shuffle(weapons);

    const int nr_max = max_nr_weapons(skill);

    if ((nr_max != -1) && ((size_t)nr_max < weapons.size())) {
        weapons.resize(nr_max);
    }

    return weapons;
}

void SpellProjectedStrike::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    TRACE_FUNC_BEGIN;

    (void)player_aware;

    if (!actor::is_player(caster)) {
        ASSERT(false);

        return;
    }

    std::vector<const item::Item*> weapons = get_weapons(skill);

    if (seen_targets.empty() || weapons.empty()) {
        msg_log::add(i18n::get(
            "spells.weapon_visions",
            "Visions of hacking, crushing and stabbing fill my mind."));

        return;
    }

    std::vector<actor::Actor*> targets = seen_targets;

    rnd::shuffle(weapons);
    rnd::shuffle(targets);

    auto remove_actor = [](std::vector<actor::Actor*>& actors, const actor::Actor* actor) {
        actors.erase(
            std::remove(std::begin(actors), std::end(actors), actor),
            std::end(actors));
    };

    auto remove_dead_actors = [](std::vector<actor::Actor*>& actors) {
        actors.erase(
            std::remove_if(
                std::begin(actors), std::end(actors), [](const actor::Actor* actor) {
                    return !actor::is_alive(*actor);
                }),
            std::end(actors));
    };

    for (size_t i = 0; i < weapons.size(); ++i) {
        const item::Item* const origin_wpn = weapons[i];

        std::unique_ptr<item::Item> new_wpn(item::make(origin_wpn->id()));

        new_wpn->m_melee_hit_chance_mod += hit_chance_bonus(skill);

        actor::Actor* const target = rnd::element(targets);

        // Calculate an origin adjacent to the target creature, for correct knockback direction
        // based on the relative positions of the caster and the target creature.
        const P attack_origin = target->m_pos + (caster->m_pos - target->m_pos).signs();

        attack::melee(
            caster,
            attack_origin,
            target->m_pos,
            *static_cast<item::Wpn*>(new_wpn.get()),
            attack::AttackSource::magical);

        // Each target can only be hit once, remove this target from the list of possible targets.
        remove_actor(targets, target);

        // Remove all dead actors to handle cases like the attacked actor being a creature that
        // explodes on death, killing other actors.
        remove_dead_actors(targets);

        if (targets.empty()) {
            break;
        }

        // Run a sleep if more attacks will happen, to avoid a bunch of sounds playing at exactly
        // the same time.
        if (i < (weapons.size() - 1)) {
            states::draw();
            io::update_screen();
            io::sleep(config::base_delay());
        }
    }

    TRACE_FUNC_END;
}

std::vector<std::string> SpellProjectedStrike::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(i18n::get(
        "spells.projected_strike.descr",
        "Launches a psychic projection of the caster's carried melee weapons."));

    descr.emplace_back(
        i18n::get(
            "spells.projected_strike.attack_prefix",
            "Each projection attacks a visible enemy, using the caster's combat skill with +") +
        std::to_string(hit_chance_bonus(skill)) +
        i18n::get(
            "spells.projected_strike.attack_suffix",
            "% hit chance bonus. "
            "No enemy can be targeted more than once."));

    const int nr_max = max_nr_weapons(skill);

    std::string nr_str;

    if (nr_max == -1) {
        nr_str = i18n::get(
            "spells.projected_strike.unlimited_weapons",
            "An unlimited number of weapons can be used for atacking.");
    }
    else {
        nr_str =
            i18n::get("spells.projected_strike.max_weapons_prefix", "A maximum of ") +
            std::to_string(nr_max) +
            i18n::get("spells.projected_strike.max_weapons_middle", " ");

        if (nr_max == 1) {
            nr_str += i18n::get("spells.projected_strike.weapon_singular", "weapon");
        }
        else {
            nr_str += i18n::get("spells.projected_strike.weapon_plural", "weapons");
        }

        nr_str += i18n::get(
            "spells.projected_strike.max_weapons_suffix",
            " may be used for attacking.");
    }

    descr.push_back(nr_str);

    descr.emplace_back(
        i18n::get(
            "spells.projected_strike.attacker_descr",
            "The caster acts as attacker - all normal conditions that affect "
            "hit chance or damage apply "
            "(e.g. bonus damage from melee traits, or damage penalty from being weakened)."));

    return descr;
}

// -----------------------------------------------------------------------------
// Control Object


// -----------------------------------------------------------------------------
// Control Object
// -----------------------------------------------------------------------------
std::string SpellControlObject::name() const
{
    return i18n::get("spells.control_object.name", "Control Object");
}

SpellId SpellControlObject::id() const
{
    return SpellId::control_object;
}

SpellDomain SpellControlObject::domain() const
{
    return SpellDomain::mind;
}

SpellShock SpellControlObject::shock_type() const
{
    return SpellShock::mild;
}

int SpellControlObject::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)caster;

    if (skill == SpellSkill::transcendent) {
        return 1;
    }
    else {
        return 4;
    }
}

int SpellControlObject::max_dist(const SpellSkill skill) const
{
    int dist = (int)skill + 3;

    dist = std::min(g_fov_radi_int, dist);

    return dist;
}

void SpellControlObject::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    const auto origin = caster->m_pos;

    auto ctrl_obj_state = std::make_unique<CtrlObj>(origin, max_dist(skill), skill);

    // Run the state immediately, so that spell side effects happen AFTER
    // the player has finished casting the spell.
    states::run_until_state_done(std::move(ctrl_obj_state));
}

std::vector<std::string> SpellControlObject::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    std::string control_descr = i18n::get(
        "spells.control_object.descr",
        "Opens doors, chests, tombs, or cabinets. "
        "Closes or jams doors. "
        "Strikes doors, braziers, or statues.");

    if (skill == SpellSkill::transcendent) {
        control_descr += i18n::get(
            "spells.control_object.walls_destroyed",
            " Walls can be destroyed.");
    }

    descr.emplace_back(control_descr);

    descr.emplace_back(
        i18n::get(
            "spells.control_object.max_distance_prefix",
            "Maximum control distance is ") +
        std::to_string(max_dist(skill)) +
        i18n::get("spells.control_object.max_distance_suffix", "."));

    descr.emplace_back(
        i18n::get(
            "spells.control_object.select_descr",
            "When casting the spell, select a seen object to control "
            "within the maximum distance."));

    return descr;
}

bool SpellControlObject::is_noisy(const SpellSkill skill) const
{
    return (skill == SpellSkill::basic);
}

// -----------------------------------------------------------------------------
// Exorcist Cleansing Fire


// -----------------------------------------------------------------------------
// See Invisible
// -----------------------------------------------------------------------------
int SpellSeeInvis::mon_cooldown() const
{
    return 30;
}

std::string SpellSeeInvis::name() const
{
    return i18n::get("spells.see_invisible.name", "See Invisible");
}

SpellId SpellSeeInvis::id() const
{
    return SpellId::see_invis;
}

SpellDomain SpellSeeInvis::domain() const
{
    return SpellDomain::mind;
}

SpellShock SpellSeeInvis::shock_type() const
{
    return SpellShock::mild;
}

bool SpellSeeInvis::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellSeeInvis::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 8;
}

Range SpellSeeInvis::duration_range(SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:  return {15, 30};
    case SpellSkill::expert: return {60, 120};
    case SpellSkill::master: return {250, 500};

    case SpellSkill::transcendent:
        // Unexpected, the spell should be indefinite
        break;
    }

    ASSERT(false);

    return {1, 1};
}

void SpellSeeInvis::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    prop::Prop* prop = prop::make(prop::Id::see_invis);

    if (skill == SpellSkill::transcendent) {
        prop->set_indefinite();
    }
    else {
        prop->set_duration(duration_range(skill).roll());
    }

    caster->m_properties.apply(prop);
}

std::vector<std::string> SpellSeeInvis::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.see_invisible.descr",
            "Grants the caster the ability to see the invisible."));

    if (skill == SpellSkill::transcendent) {
        descr.emplace_back(spell_indefinite_duration_descr());
    }
    else {
        descr.push_back(spell_duration_descr(duration_range(skill).str()));
    }

    return descr;
}

bool SpellSeeInvis::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)seen_targets;

    return (
        !mon.m_properties.has(prop::Id::see_invis) &&
        actor::is_aware_of_player(mon) &&
        rnd::one_in(8));
}

// -----------------------------------------------------------------------------
// Spell Shield


// -----------------------------------------------------------------------------
// Premonition
// -----------------------------------------------------------------------------
std::string SpellPremonition::name() const
{
    return i18n::get("spells.premonition.name", "Premonition");
}

SpellId SpellPremonition::id() const
{
    return SpellId::premonition;
}

SpellDomain SpellPremonition::domain() const
{
    return SpellDomain::mind;
}

SpellShock SpellPremonition::shock_type() const
{
    return SpellShock::mild;
}

bool SpellPremonition::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

Range SpellPremonition::duration_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {4, 8};
    case SpellSkill::expert:       return {8, 16};
    case SpellSkill::master:       return {12, 24};
    case SpellSkill::transcendent: return {20, 40};
    }

    ASSERT(false);

    return {1, 1};
}

int SpellPremonition::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

void SpellPremonition::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    prop::Prop* prop = prop::make(prop::Id::premonition);

    prop->set_duration(duration_range(skill).roll());

    caster->m_properties.apply(prop);
}

std::vector<std::string> SpellPremonition::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.premonition.descr",
            "Grants foresight of attacks against the caster, "
            "making it extremely difficult for assailants to achieve a "
            "succesful hit."));

    descr.emplace_back(spell_duration_descr(duration_range(skill).str()));

    return descr;
}

bool SpellPremonition::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    return (
        !seen_targets.empty() &&
        !mon.m_properties.has(prop::Id::premonition));
}

// -----------------------------------------------------------------------------
// Erudition


// -----------------------------------------------------------------------------
// Erudition
// -----------------------------------------------------------------------------
std::string SpellErudition::name() const
{
    return i18n::get("spells.erudition.name", "Erudition");
}

SpellId SpellErudition::id() const
{
    return SpellId::erudition;
}

SpellDomain SpellErudition::domain() const
{
    return SpellDomain::mind;
}

SpellShock SpellErudition::shock_type() const
{
    return SpellShock::disturbing;
}

bool SpellErudition::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

int SpellErudition::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)caster;

    return 7 - (int)skill;
}

Range SpellErudition::get_duration_range(SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {2, 4};
    case SpellSkill::expert:       return {4, 8};
    case SpellSkill::master:
    case SpellSkill::transcendent: return {6, 12};
    }

    ASSERT(false);

    return {1, 1};
}

void SpellErudition::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    {
        prop::Prop* prop = prop::make(prop::Id::erudition);

        prop->set_duration(get_duration_range(skill).roll());

        caster->m_properties.apply(prop);
    }

    if (skill == SpellSkill::transcendent) {
        auto* const prop =
            caster->m_properties.prop(prop::Id::erudition);

        if (!prop) {
            ASSERT(false);

            return;
        }

        auto* const erudition = static_cast<prop::Erudition*>(prop);

        erudition->disable_end_on_spell_cast();
    }
}

std::vector<std::string> SpellErudition::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.erudition.descr_main",
            "Temporarily bestows the caster with an expanded understanding "
            "of the esoteric mechanisms behind magical practice. "
            "The caster's skill is improved by one level for all spells."));

    std::string duration_descr =
        i18n::get(
            "spells.erudition.duration_prefix",
            "The spell lasts ") +
        get_duration_range(skill).str() +
        i18n::get(
            "spells.erudition.duration_turns",
            " turns");

    if (skill == SpellSkill::transcendent) {
        duration_descr +=
            i18n::get(
                "spells.erudition.duration_transcendent_suffix",
                ". The effect does not end when casting spells, "
                "only when the duration expires.");
    }
    else {
        duration_descr +=
            i18n::get(
                "spells.erudition.duration_normal_suffix",
                ", or until a spell is cast (either from a Manuscript "
                "or from memory).");
    }

    descr.push_back(duration_descr);

    return descr;
}

// -----------------------------------------------------------------------------
// Identify


// -----------------------------------------------------------------------------
// Identify
// -----------------------------------------------------------------------------
std::string SpellIdentify::name() const
{
    return i18n::get("spells.identify.name", "Identify");
}

SpellId SpellIdentify::id() const
{
    return SpellId::identify;
}

SpellDomain SpellIdentify::domain() const
{
    return SpellDomain::mind;
}

bool SpellIdentify::is_tenebrous() const
{
    return true;
}

SpellShock SpellIdentify::shock_type() const
{
    return SpellShock::disturbing;
}

bool SpellIdentify::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

int SpellIdentify::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

void SpellIdentify::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    // TODO: Test with casting from unknown manuscript (with and without something to ID).

    std::vector<ItemType> item_types_allowed;

    if (skill != SpellSkill::master) {
        item_types_allowed.push_back(ItemType::scroll);

        if (skill == SpellSkill::expert) {
            item_types_allowed.push_back(ItemType::potion);
        }
    }

    if (skill == SpellSkill::transcendent) {
        // Immediately identify all items.
        for (item::Item* const item : caster->m_inv.all_items()) {
            item->identify(Verbose::yes);
        }
    }
    else {
        // Run identify selection menu to select one item.
        auto state = std::make_unique<SelectIdentify>(item_types_allowed);

        states::push(std::move(state));
    }

    msg_log::more_prompt();
}

std::vector<std::string> SpellIdentify::descr_specific(
    const SpellSkill skill) const
{
    if (skill == SpellSkill::transcendent) {
        return {i18n::get(
            "spells.identify.descr_all_items",
            "Immediately identifies all carried items.")};
    }

    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.identify.descr_one_item",
            "Identifies one carried item."));

    std::string identifies_str =
        i18n::get(
            "spells.identify.allowed_prefix",
            "The spell can identify ");

    switch (skill) {
    case SpellSkill::basic:
        identifies_str += i18n::get("spells.identify.allowed_basic", "Manuscripts");
        break;
    case SpellSkill::expert:
        identifies_str += i18n::get(
            "spells.identify.allowed_expert",
            "Manuscripts and Potions");
        break;
    case SpellSkill::master:
        identifies_str += i18n::get("spells.identify.allowed_master", "all items");
        break;

    case SpellSkill::transcendent:
        ASSERT(false);
        break;
    }

    identifies_str += i18n::get("spells.identify.allowed_suffix", ".");

    descr.push_back(identifies_str);

    return descr;
}

// -----------------------------------------------------------------------------
// Teleport


// -----------------------------------------------------------------------------
// Transmutation
// -----------------------------------------------------------------------------
std::string SpellTransmut::name() const
{
    return i18n::get("spells.transmutation.name", "Transmutation");
}

SpellId SpellTransmut::id() const
{
    return SpellId::transmut;
}

SpellDomain SpellTransmut::domain() const
{
    return SpellDomain::mind;
}

bool SpellTransmut::is_tenebrous() const
{
    return true;
}

SpellShock SpellTransmut::shock_type() const
{
    return SpellShock::mild;
}

int SpellTransmut::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 4;
}

bool SpellTransmut::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

int SpellTransmut::skill_bon(const SpellSkill skill) const
{
    return 10 * (int)skill;
}

int SpellTransmut::chance_scroll(const SpellSkill skill) const
{
    return skill_bon(skill) + 40;
}

int SpellTransmut::chance_potion(const SpellSkill skill) const
{
    return skill_bon(skill) + 40;
}

int SpellTransmut::chance_weapon(
    const SpellSkill skill,
    int plus) const
{
    plus = std::min(5, plus);

    return skill_bon(skill) + (plus * 10);
}

void SpellTransmut::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)caster;
    (void)seen_targets;
    (void)player_aware;

    const auto& p = map::g_player->m_pos;

    auto* item_before = map::g_items.at(p);

    if (!item_before) {
        msg_log::add(i18n::get(
            "spells.vague_change_in_air",
            "There is a vague change in the air."));

        return;
    }

    // Player is standing on an item

    // Get information on the existing item(s)
    const bool is_stackable_before = item_before->data().is_stackable;

    const int nr_items_before =
        is_stackable_before
        ? item_before->m_nr_items
        : 1;

    const auto item_type_before = item_before->data().type;

    const int melee_plus = item_before->base_melee_dmg().plus();

    const auto id_before = item_before->id();

    std::string item_name_before =
        i18n::get("spells.transmutation.item_before_prefix", "The ");

    if (nr_items_before > 1) {
        item_name_before += item_before->name(ItemNameType::plural);
    }
    else {
        // Single item
        item_name_before += item_before->name(ItemNameType::plain);
    }

    // Remove the existing item(s)
    delete map::g_items.at(p);
    map::g_items.at(p) = nullptr;

    if (map::g_seen.at(p)) {
        std::string disappear_str =
            (nr_items_before == 1)
            ? i18n::get("spells.transmutation.disappears_singular", "disappears")
            : i18n::get("spells.transmutation.disappears_plural", "disappear");

        msg_log::add(
            item_name_before +
                i18n::get("spells.space", " ") +
                disappear_str +
                i18n::get("spells.period", "."),
            colors::text(),
            MsgInterruptPlayer::no,
            MorePromptOnMsg::yes);
    }

    // Determine which item(s) to spawn, if any

    int pct_chance_per_item = 0;

    std::vector<item::Id> id_bucket;

    // Converting a potion?
    if (item_type_before == ItemType::potion) {
        pct_chance_per_item = chance_potion(skill);

        for (size_t item_id = 0;
             (item::Id)item_id != item::Id::END;
             ++item_id) {
            if ((item::Id)item_id == id_before) {
                continue;
            }

            const auto& d = item::g_data[item_id];

            if (d.type == ItemType::potion) {
                id_bucket.push_back((item::Id)item_id);
            }
        }
    }
    // Converting a scroll?
    else if (item_type_before == ItemType::scroll) {
        pct_chance_per_item = chance_scroll(skill);

        for (size_t item_id = 0;
             (item::Id)item_id != item::Id::END;
             ++item_id) {
            if ((item::Id)item_id == id_before) {
                continue;
            }

            const auto& d = item::g_data[item_id];

            if (d.type == ItemType::scroll) {
                id_bucket.push_back((item::Id)item_id);
            }
        }
    }
    // Converting a melee weapon (with at least one "plus")?
    else if ((item_type_before == ItemType::melee_wpn) && (melee_plus >= 1)) {
        pct_chance_per_item = chance_weapon(skill, melee_plus);

        for (size_t item_id = 0;
             (item::Id)item_id != item::Id::END;
             ++item_id) {
            const auto& d = item::g_data[item_id];

            if ((d.type == ItemType::potion) ||
                (d.type == ItemType::scroll)) {
                id_bucket.push_back((item::Id)item_id);
            }
        }
    }

    // Never spawn Transmute scrolls, this is just dumb
    for (auto it = std::begin(id_bucket); it != std::end(id_bucket);) {
        if (*it == item::Id::scroll_transmut) {
            it = id_bucket.erase(it);
        }
        else {
            // Not transmute
            ++it;
        }
    }

    auto id_new = item::Id::END;

    if (!id_bucket.empty()) {
        id_new = rnd::element(id_bucket);
    }

    int nr_items_new = 0;

    // How many items?
    for (int i = 0; i < nr_items_before; ++i) {
        if (rnd::percent(pct_chance_per_item)) {
            ++nr_items_new;
        }
    }

    if ((id_new == item::Id::END) || (nr_items_new < 1)) {
        msg_log::add(i18n::get(
            "spells.nothing_appears",
            "Nothing appears."));

        return;
    }

    // OK, items are good, and player succeeded the rolls etc

    auto* item_new = item::make(id_new, nr_items_new);

    item::randomize_item_properties(*item_new);

    if (item_new->data().is_stackable) {
        item_new->m_nr_items = nr_items_new;
    }

    const std::string item_name_new =
        text_format::first_to_upper(
            item_new->name(ItemNameType::plural));

    if (map::g_seen.at(p)) {
        std::string appear_str =
            (nr_items_new == 1)
            ? i18n::get("spells.transmutation.appears_singular", "appears")
            : i18n::get("spells.transmutation.appears_plural", "appear");

        msg_log::add(
            item_name_new +
            i18n::get("spells.space", " ") +
            appear_str +
            i18n::get("spells.period", "."));
    }

    // NOTE: This will possibly make the player "discover" the item, so it
    // should occur last, after the "appear" message.
    item_drop::drop_item_on_map(map::g_player->m_pos, *item_new);
}

std::vector<std::string> SpellTransmut::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.transmutation.descr_main",
            "Attempts to convert items (stand over an item when casting). "
            "On failure, the item is destroyed."));

    descr.push_back(
        i18n::get(
            "spells.transmutation.potion_chance_prefix",
            "Converts Potions with ") +
        std::to_string(chance_potion(skill)) +
        i18n::get(
            "spells.transmutation.chance_suffix",
            "% chance."));

    descr.push_back(
        i18n::get(
            "spells.transmutation.manuscript_chance_prefix",
            "Converts Manuscripts with ") +
        std::to_string(chance_scroll(skill)) +
        i18n::get(
            "spells.transmutation.chance_suffix",
            "% chance."));

    descr.push_back(
        i18n::get(
            "spells.transmutation.weapon_chance_prefix",
            "Melee weapons with at least +1 damage (not counting any "
            "damage bonus from skills) are converted to a Potion or "
            "Manuscript, with ") +
        std::to_string(chance_weapon(skill, 1)) +
        i18n::get(
            "spells.transmutation.weapon_chance_plus_one",
            "% chance for a +1 weapon, ") +
        std::to_string(chance_weapon(skill, 2)) +
        i18n::get(
            "spells.transmutation.weapon_chance_plus_two",
            "% chance for a +2 weapon, ") +
        std::to_string(chance_weapon(skill, 3)) +
        i18n::get(
            "spells.transmutation.weapon_chance_plus_three",
            "% chance for a +3 weapon, etc."));

    return descr;
}

// -----------------------------------------------------------------------------
// Clairvoyance


// -----------------------------------------------------------------------------
// Clairvoyance
// -----------------------------------------------------------------------------
std::string SpellClairvoyance::name() const
{
    return i18n::get("spells.clairvoyance.name", "Clairvoyance");
}

SpellId SpellClairvoyance::id() const
{
    return SpellId::clairvoyance;
}

SpellDomain SpellClairvoyance::domain() const
{
    return SpellDomain::mind;
}

bool SpellClairvoyance::is_tenebrous() const
{
    return true;
}

bool SpellClairvoyance::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

SpellShock SpellClairvoyance::shock_type() const
{
    return SpellShock::disturbing;
}

Range SpellClairvoyance::duration_range(const SpellSkill skill) const
{
    if (skill == SpellSkill::transcendent) {
        return {400, 800};
    }
    else {
        return {150, 300};
    }
}

int SpellClairvoyance::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

void SpellClairvoyance::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    auto* clairvoyance = static_cast<prop::Clairvoyance*>(prop::make(prop::Id::clairvoyance));

    clairvoyance->set_duration(duration_range(skill).roll());

    if (skill >= SpellSkill::expert) {
        clairvoyance->set_allow_reveal_items();
    }

    if (skill >= SpellSkill::master) {
        clairvoyance->set_allow_reveal_creatures();
    }

    caster->m_properties.apply(clairvoyance);
}

std::vector<std::string> SpellClairvoyance::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.clairvoyance.descr",
            "Reveals the presence of doors, traps, stairs, and other "
            "locations of interest in the surrounding area."));

    if (skill == SpellSkill::expert) {
        descr.emplace_back(
            i18n::get(
                "spells.clairvoyance.reveals_items",
                "Also reveals items."));
    }
    else if (skill >= SpellSkill::master) {
        descr.emplace_back(
            i18n::get(
                "spells.clairvoyance.reveals_items_creatures",
                "Also reveals items and creatures."));
    }

    descr.push_back(spell_duration_descr(duration_range(skill).str()));

    return descr;
}

