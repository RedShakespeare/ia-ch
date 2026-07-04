// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SPELLS_BLOOD_HPP
#define SPELLS_BLOOD_HPP

#include "spells.hpp"

class SpellBloodTempering : public Spell
{
public:
    SpellBloodTempering() = default;

    std::string name() const override;

    SpellId id() const override;

    SpellDomain domain() const override;

    bool is_tenebrous() const override;

    SpellCostType cost_type() const override;

    SpellShock shock_type() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const override;

private:
    Range duration_range(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellThorns : public Spell
{
public:
    SpellThorns() = default;

    std::string name() const override;

    SpellId id() const override;

    SpellDomain domain() const override;

    SpellCostType cost_type() const override;

    SpellShock shock_type() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const override;

private:
    Range duration_range(SpellSkill skill) const;
    Range dmg_range(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellCrimsonPassage : public Spell
{
public:
    SpellCrimsonPassage() = default;

    std::string name() const override;

    SpellId id() const override;

    SpellDomain domain() const override;

    SpellCostType cost_type() const override;

    SpellShock shock_type() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const override;

private:
    int nr_steps_allowed(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellSacrificeLife : public Spell
{
public:
    SpellSacrificeLife() = default;

    std::string name() const override;

    SpellId id() const override;

    SpellDomain domain() const override;

    bool is_tenebrous() const override;

    SpellShock shock_type() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const override;

private:
    int nr_sp_per_hp(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellShedImpurity : public Spell
{
public:
    SpellShedImpurity() = default;

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
    int get_min_hp_removed_for_bonus_effects() const;

    int get_moribund_hp_limit() const;

    int calc_nr_hp_removed(const actor::Actor* caster) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

#endif  // SPELLS_BLOOD_HPP
