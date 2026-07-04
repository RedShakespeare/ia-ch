// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "spells.hpp"
#include "spells_internal.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include "actor.hpp"
#include "actor_data.hpp"
#include "actor_hit.hpp"
#include "actor_see.hpp"
#include "audio.hpp"
#include "audio_data.hpp"
#include "colors.hpp"
#include "explosion.hpp"
#include "game_time.hpp"
#include "global.hpp"
#include "i18n.hpp"
#include "inventory.hpp"
#include "item.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "item_weapon.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "pos.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "property_handler.hpp"
#include "random.hpp"
#include "rect.hpp"
#include "sound.hpp"
#include "text_format.hpp"
#include "viewport.hpp"
#include "wpn_dmg.hpp"

// -----------------------------------------------------------------------------
// Blood Tempering
// -----------------------------------------------------------------------------
std::string SpellBloodTempering::name() const
{
    return i18n::get("spells.blood_tempering.name", "Blood Tempering");
}

SpellId SpellBloodTempering::id() const
{
    return SpellId::blood_tempering;
}

SpellDomain SpellBloodTempering::domain() const
{
    return SpellDomain::blood;
}

bool SpellBloodTempering::is_tenebrous() const
{
    return true;
}

SpellCostType SpellBloodTempering::cost_type() const
{
    return SpellCostType::hit_points;
}

SpellShock SpellBloodTempering::shock_type() const
{
    return SpellShock::disturbing;
}

bool SpellBloodTempering::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

Range SpellBloodTempering::duration_range(SpellSkill skill) const
{
    Range duration_range;

    duration_range.min = 4 + ((int)skill * 2);
    duration_range.max = duration_range.min + 4;

    return duration_range;
}

int SpellBloodTempering::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 8;
}

void SpellBloodTempering::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    int nr_turns = duration_range(skill).roll();

    prop::Prop* r_phys = prop::make(prop::Id::r_phys);

    r_phys->set_duration(nr_turns);

    caster->m_properties.apply(r_phys);
}

std::vector<std::string> SpellBloodTempering::descr_specific(
    SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.blood_tempering.descr",
            "Through ardous suffering, the caster tempers their body to "
            "resist physical force (cannot be harmed by normal attacks, "
            "however other forms of damage such as fire is still "
            "harmful)."));

    descr.emplace_back(spell_duration_descr(duration_range(skill).str()));

    return descr;
}

// -----------------------------------------------------------------------------
// Thorns
// -----------------------------------------------------------------------------
std::string SpellThorns::name() const
{
    return i18n::get("spells.thorns.name", "Thorns");
}

SpellId SpellThorns::id() const
{
    return SpellId::thorns;
}

SpellDomain SpellThorns::domain() const
{
    return SpellDomain::blood;
}

SpellCostType SpellThorns::cost_type() const
{
    return SpellCostType::hit_points;
}

SpellShock SpellThorns::shock_type() const
{
    return SpellShock::mild;
}

int SpellThorns::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 4;
}

bool SpellThorns::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

Range SpellThorns::duration_range(const SpellSkill skill) const
{
    Range duration_range;

    duration_range.min = ((int)skill + 1) * 5;
    duration_range.max = duration_range.min * 2;

    return duration_range;
}

Range SpellThorns::dmg_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {2, 4};   // Avg 3.0
    case SpellSkill::expert:       return {3, 6};   // Avg 4.5
    case SpellSkill::master:       return {4, 8};   // Avg 6.0
    case SpellSkill::transcendent: return {5, 10};  // Avg 7.5
    }

    ASSERT(false);

    return {1, 1};
}

void SpellThorns::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    auto* const prop = static_cast<prop::Thorns*>(prop::make(prop::Id::thorns));

    prop->set_duration(duration_range(skill).roll());

    prop->set_dmg(dmg_range(skill).roll());

    caster->m_properties.apply(prop);
}

std::vector<std::string> SpellThorns::descr_specific(SpellSkill skill) const
{
    std::vector<std::string> descr;

    // Re-using the property description as spell description.
    descr.push_back(prop::g_data[(size_t)prop::Id::thorns].descr);

    descr.push_back(
        i18n::get(
            "spells.thorns.return_damage_prefix",
            "The spell returns ") +
        dmg_range(skill).str() +
        i18n::get(
            "spells.thorns.return_damage_suffix",
            " damage to the attacker."));

    descr.emplace_back(spell_duration_descr(duration_range(skill).str()));

    return descr;
}

// -----------------------------------------------------------------------------
// Crimson Passage
// -----------------------------------------------------------------------------
std::string SpellCrimsonPassage::name() const
{
    return i18n::get("spells.crimson_passage.name", "Crimson Passage");
}

SpellId SpellCrimsonPassage::id() const
{
    return SpellId::crimson_passage;
}

SpellDomain SpellCrimsonPassage::domain() const
{
    return SpellDomain::blood;
}

SpellCostType SpellCrimsonPassage::cost_type() const
{
    return SpellCostType::hit_points;
}

bool SpellCrimsonPassage::is_noisy(SpellSkill skill) const
{
    return (skill == SpellSkill::basic);
}

SpellShock SpellCrimsonPassage::shock_type() const
{
    // If the effect is already active, the spell does not cause shock.
    //
    // HACK: Assuming only the player can cast this spell.
    return (
        map::g_player->m_properties.has(prop::Id::crimson_passage)
            ? SpellShock::none
            : SpellShock::disturbing);
}

int SpellCrimsonPassage::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    // If the effect is already active, the spell is free to cast.

    // HACK: Assuming only the player can cast this spell.
    return map::g_player->m_properties.has(prop::Id::crimson_passage) ? 0 : 3;
}

int SpellCrimsonPassage::nr_steps_allowed(const SpellSkill skill) const
{
    if (skill == SpellSkill::transcendent) {
        return -1;
    }
    else {
        return ((int)skill + 1) * 4;
    }
}

void SpellCrimsonPassage::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    if (caster->m_properties.has(prop::Id::crimson_passage)) {
        // Effect already active, cancel it instead.
        caster->m_properties.end_prop(prop::Id::crimson_passage);

        return;
    }

    auto* const prop =
        static_cast<prop::CrimsonPassage*>(
            prop::make(prop::Id::crimson_passage));

    prop->set_indefinite();

    prop->set_nr_steps_allowed(nr_steps_allowed(skill));

    caster->m_properties.apply(prop);
}

std::vector<std::string> SpellCrimsonPassage::descr_specific(
    SpellSkill skill) const
{
    std::vector<std::string> descr;

    // Re-using the property description as spell description.
    descr.push_back(prop::g_data[(size_t)prop::Id::crimson_passage].descr);

    const int nr_steps = nr_steps_allowed(skill);

    if (nr_steps == -1) {
        descr.emplace_back(
            i18n::get(
                "spells.crimson_passage.infinite_steps",
                "An infinite number of steps may be taken, the spell "
                "is only limited by the number of hit points."));
    }
    else {
        descr.emplace_back(
            std::to_string(nr_steps_allowed(skill)) +
            i18n::get(
                "spells.crimson_passage.steps_suffix",
                " steps may be taken before the effect ends."));
    }

    descr.emplace_back(
        i18n::get(
            "spells.crimson_passage.recast_cancels",
            "Casting the spell again while it is already active cancels "
            "the effect (this does not drain hit points or cause shock)."));

    return descr;
}

// -----------------------------------------------------------------------------
// Sacrifice Life
// -----------------------------------------------------------------------------
std::string SpellSacrificeLife::name() const
{
    return i18n::get("spells.sacrifice_life.name", "Sacrifice Life");
}

SpellId SpellSacrificeLife::id() const
{
    return SpellId::sacrifice_life;
}

SpellDomain SpellSacrificeLife::domain() const
{
    return SpellDomain::blood;
}

bool SpellSacrificeLife::is_tenebrous() const
{
    return true;
}

SpellShock SpellSacrificeLife::shock_type() const
{
    return SpellShock::disturbing;
}

int SpellSacrificeLife::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 0;
}

bool SpellSacrificeLife::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellSacrificeLife::nr_sp_per_hp(const SpellSkill skill) const
{
    return 1 + (int)skill;
}

void SpellSacrificeLife::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    const int hp = caster->m_hp;

    if (hp <= 2) {
        // Not enough HP.
        msg_log::add(i18n::get(
            "spells.little_to_offer",
            "I feel like I have very little to offer."));

        return;
    }

    int hp_drained = ((hp - 1) / 2) * 2;

    hp_drained = std::min(8, hp_drained);

    actor::hit(*caster, hp_drained, DmgType::pure, nullptr, AllowWound::no);

    const int sp_gained = hp_drained * nr_sp_per_hp(skill);

    actor::restore_sp(*caster, sp_gained, actor::AllowRestoreAboveMax::yes);
}

std::vector<std::string> SpellSacrificeLife::descr_specific(
    SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.sacrifice_life.descr",
            "Sacrifices the life force of the caster in order to restore "
            "the spirit. The amount restored is proportional to the life "
            "lost. A maximum of 8 hit points may be sacrificed."));

    const int k = nr_sp_per_hp(skill);

    if (k == 1) {
        descr.emplace_back(
            i18n::get(
                "spells.sacrifice_life.spirit_point_prefix",
                "For each hit point sacrificed, ") +
            std::to_string(k) +
            i18n::get(
                "spells.sacrifice_life.spirit_point_singular_suffix",
                " spirit point is gained."));
    }
    else {
        descr.emplace_back(
            i18n::get(
                "spells.sacrifice_life.spirit_point_prefix",
                "For each hit point sacrificed, ") +
            std::to_string(k) +
            i18n::get(
                "spells.sacrifice_life.spirit_point_plural_suffix",
                " spirit points are gained."));
    }

    return descr;
}

// -----------------------------------------------------------------------------
// Shed Impurity
// -----------------------------------------------------------------------------
std::string SpellShedImpurity::name() const
{
    return i18n::get("spells.shed_impurity.name", "Shed Impurity");
}

SpellId SpellShedImpurity::id() const
{
    return SpellId::shed_impurity;
}

SpellDomain SpellShedImpurity::domain() const
{
    return SpellDomain::blood;
}

SpellShock SpellShedImpurity::shock_type() const
{
    return SpellShock::mild;
}

int SpellShedImpurity::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 0;
}

bool SpellShedImpurity::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

int SpellShedImpurity::get_min_hp_removed_for_bonus_effects() const
{
    return 9;
}

int SpellShedImpurity::get_moribund_hp_limit() const
{
    return player_bon::has_trait(TraitId::memento_mori) ? 8 : 6;
}

int SpellShedImpurity::calc_nr_hp_removed(const actor::Actor* const caster) const
{
    return caster->m_hp - get_moribund_hp_limit();
}

void SpellShedImpurity::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;

    const int hp_removed = calc_nr_hp_removed(caster);

    if (hp_removed <= 0) {
        // Not enough HP.
        msg_log::add(i18n::get(
            "spells.nothing_more_to_shed",
            "There is nothing more to shed."));

        return;
    }

    actor::hit(*caster, hp_removed, DmgType::pure, nullptr, AllowWound::no);

    if (hp_removed >= get_min_hp_removed_for_bonus_effects()) {
        caster->m_properties.end_prop(prop::Id::weakened);
        caster->m_properties.end_prop(prop::Id::poisoned);

        if ((int)skill >= (int)SpellSkill::expert) {
            caster->m_properties.end_prop(prop::Id::infected);
            caster->m_properties.end_prop(prop::Id::diseased);
        }

        if (skill >= SpellSkill::master) {
            caster->m_properties.end_prop(prop::Id::slowed);
        }

        if (skill == SpellSkill::transcendent) {
            std::unique_ptr<Spell> bless_spell(spells::make(SpellId::bless));

            bless_spell->run_effect(caster, SpellSkill::basic, {}, player_aware);
        }
    }
}

std::vector<std::string> SpellShedImpurity::descr_specific(
    SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.shed_impurity.descr",
            "Purifies the caster by carving away all that is extraneous, "
            "revealing the essential core of their being."));

    descr.emplace_back(
        i18n::get(
            "spells.shed_impurity.moribund_prefix",
            "Hit points are lowered to the limit where the Moribund effect is activated "
            "(bonuses for having low hit points). "
            "This limit is at ") +
        std::to_string(get_moribund_hp_limit()) +
        i18n::get("spells.shed_impurity.moribund_suffix", " hit points."));

    std::string bonus_effect_descr =
        i18n::get("spells.shed_impurity.bonus_prefix", "If at least ") +
        std::to_string(get_min_hp_removed_for_bonus_effects()) +
        i18n::get(
            "spells.shed_impurity.bonus_middle",
            " hit points are lost, then ");

    switch (skill) {
    case SpellSkill::basic: {
        bonus_effect_descr +=
            i18n::get(
                "spells.shed_impurity.cures_basic",
                "weakening and poisoning are cured.");
    } break;

    case SpellSkill::expert: {
        bonus_effect_descr +=
            i18n::get(
                "spells.shed_impurity.cures_expert",
                "weakening, poisoning, infection and disease are cured.");
    } break;

    case SpellSkill::master: {
        bonus_effect_descr +=
            i18n::get(
                "spells.shed_impurity.cures_master",
                "weakening, poisoning, infection, disease and slowing are cured.");
    } break;

    case SpellSkill::transcendent: {
        std::unique_ptr<SpellBless> bless_spell(
            static_cast<SpellBless*>(spells::make(SpellId::bless)));

        bonus_effect_descr +=
            i18n::get(
                "spells.shed_impurity.cures_transcendent_prefix",
                "weakening, poisoning, infection, disease and slowing are cured. "
                "The caster is also blessed for ") +
            bless_spell->duration_range(SpellSkill::basic).str() +
            i18n::get("spells.shed_impurity.cures_transcendent_suffix", " turns.");
    } break;
    }

    bonus_effect_descr +=
        i18n::get("spells.shed_impurity.current_removed_prefix", " Currently ") +
        std::to_string(calc_nr_hp_removed(map::g_player)) +
        i18n::get(
            "spells.shed_impurity.current_removed_suffix",
            " hit points would be removed.");

    descr.push_back(bonus_effect_descr);

    return descr;
}
