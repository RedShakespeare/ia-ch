// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SPELLS_WARDING_HPP
#define SPELLS_WARDING_HPP

#include "spells.hpp"

class SpellBless : public Spell
{
public:
    SpellBless() = default;

    std::string name() const override;

    SpellId id() const override;

    SpellDomain domain() const override;

    SpellShock shock_type() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const override;

    Range duration_range(SpellSkill skill) const;

private:
    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

enum class CancelledPropIncludeInDescr
{
    no,
    yes,
};

enum class CancelledPropAllowCancelPermanent
{
    no,
    yes,
};

struct CancelledPropData
{
    prop::Id id;

    // Include it in the lits of cancelled effects?
    CancelledPropIncludeInDescr include_in_descr {
        CancelledPropIncludeInDescr::yes};

    CancelledPropAllowCancelPermanent allow_cancel_permanent_effect {
        CancelledPropAllowCancelPermanent::no};
};

class SpellCancellation : public Spell
{
public:
    SpellCancellation() = default;

    std::string name() const override;

    SpellId id() const override;

    SpellDomain domain() const override;

    SpellShock shock_type() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const override;

private:
    int max_dist(SpellSkill skill) const;

    std::vector<CancelledPropData> negative_effect_types_cancelled() const;
    std::vector<CancelledPropData> positive_effect_types_cancelled() const;

    void run_effect_on_actor(actor::Actor& actor, actor::Actor& caster) const;
    void cancel_negative_effects(actor::Actor& actor) const;
    void cancel_positive_effects(actor::Actor& actor) const;
    Range damage_for_vulnerable_creatures() const;
    void do_damage_vulnerable_creature(actor::Actor& actor, actor::Actor& caster) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellInscribeBoundarySigil : public Spell
{
public:
    SpellInscribeBoundarySigil() = default;

    std::string name() const override;

    SpellId id() const override;

    SpellDomain domain() const override;

    SpellShock shock_type() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const override;

private:
    Range nr_actions_prevented(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellLight : public Spell
{
public:
    SpellLight() = default;

    std::string name() const override;

    SpellId id() const override;

    SpellDomain domain() const override;

    SpellShock shock_type() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const override;

private:
    Range light_duration_range(SpellSkill skill) const;

    Range blind_duration_range(SpellSkill skill) const;

    Range burning_duration_range() const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellSpellShield : public Spell
{
public:
    SpellSpellShield() = default;

    bool allow_mon_cast_now(
        const actor::Actor& mon,
        const std::vector<actor::Actor*>& seen_targets) const override;

    int mon_cooldown() const override;

    std::string name() const override;

    SpellId id() const override;

    SpellDomain domain() const override;

    SpellShock shock_type() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const override;

private:
    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellHeal : public Spell
{
public:
    SpellHeal() = default;

    bool allow_mon_cast_now(
        const actor::Actor& mon,
        const std::vector<actor::Actor*>& seen_targets) const override;

    int mon_cooldown() const override;

    std::string name() const override;

    SpellId id() const override;

    SpellDomain domain() const override;

    SpellShock shock_type() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const override;

private:
    int nr_hp_restored(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;

    Range regen_duration() const;
};

#endif  // SPELLS_WARDING_HPP
