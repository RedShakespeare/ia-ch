// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SPELLS_ILLUSION_HPP
#define SPELLS_ILLUSION_HPP

#include "spells.hpp"

class SpellTerrify : public Spell
{
public:
    SpellTerrify() = default;

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

    Range duration_range(SpellSkill skill) const;

    int faint_pct_chance(SpellSkill skill) const;
    Range faint_duration_range() const;

    void terrify_target(actor::Actor& target, SpellSkill skill) const;
    void faint_target(actor::Actor& target) const;
};

class SpellThreatProjection : public Spell
{
public:
    SpellThreatProjection() = default;

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

    Range duration_range(SpellSkill skill) const;

    void conflict_target(actor::Actor& target, SpellSkill skill) const;
};

class SpellMirrorImages : public Spell
{
public:
    SpellMirrorImages() = default;

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
    int nr_mirror_images_summoned(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;

    Range duration_range(SpellSkill skill) const;

    void on_mirror_image_summoned(actor::Actor* mon, SpellSkill skill) const;
};

class SpellInvis : public Spell
{
public:
    SpellInvis() = default;

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
    Range duration_range(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

#endif  // SPELLS_ILLUSION_HPP
