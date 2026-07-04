// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SPELLS_CORRUPTION_HPP
#define SPELLS_CORRUPTION_HPP

#include "spells.hpp"

class SpellCurse : public Spell
{
public:
    SpellCurse() = default;

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
    Range duration_range(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    int pct_chance_doom(SpellSkill skill) const;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellPoison : public Spell
{
public:
    SpellPoison() = default;

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
    Range duration_range(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellEnfeeble : public Spell
{
public:
    SpellEnfeeble() = default;

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
    Range duration_range(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SpellAuraOfDecay : public Spell
{
public:
    SpellAuraOfDecay() = default;

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

    Range dmg_range(SpellSkill skill) const;

    Range duration_range(SpellSkill skill) const;
};

class SpellPestilence : public Spell
{
public:
    SpellPestilence() = default;

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
    int nr_rats_summoned(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;

    Range duration_range(SpellSkill skill) const;

    void on_rat_summoned(actor::Actor* mon, SpellSkill skill) const;
};

#endif  // SPELLS_CORRUPTION_HPP
