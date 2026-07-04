// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SPELLS_MONSTER_HPP
#define SPELLS_MONSTER_HPP

#include "spells.hpp"

class SpellKnockBack : public Spell
{
public:
    SpellKnockBack() = default;

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

class SpellDisease : public Spell
{
public:
    SpellDisease() = default;

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

class SpellBlind : public Spell
{
public:
    SpellBlind() = default;

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
    std::vector<actor::Actor*> find_actors_not_blind_resistant(
        const std::vector<actor::Actor*>& actors) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

class SummonImpl
{
public:
    virtual ~SummonImpl() = default;

    virtual SpellId id() const = 0;

    // NOTE: The input to this function is any summonable monster within a certain level range
    // depending on the monsters skill level.
    virtual std::vector<std::string> filter_allowed_ids(
        const std::vector<std::string>& summon_bucket) const = 0;

    virtual int mon_cooldown() const;

    virtual std::string appear_msg_override() const;
};

class SummonRandom : public SummonImpl
{
public:
    SummonRandom() = default;

    SpellId id() const override;

    std::vector<std::string> filter_allowed_ids(
        const std::vector<std::string>& summon_bucket) const override;
};

class SummonWaterCreature : public SummonImpl
{
public:
    SummonWaterCreature() = default;

    SpellId id() const override;

    std::vector<std::string> filter_allowed_ids(
        const std::vector<std::string>& summon_bucket) const override;
};

class SummonTentacles : public SummonImpl
{
public:
    SummonTentacles() = default;

    SpellId id() const override;

    std::vector<std::string> filter_allowed_ids(
        const std::vector<std::string>& summon_bucket) const override;

    int mon_cooldown() const override;

    std::string appear_msg_override() const override;
};

class SpellSummon : public Spell
{
public:
    SpellSummon(SummonImpl* impl) :
        m_impl(impl) {}

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

    // NOTE: There is no way for the summon implementation classes to control the allowed
    // dungeon level range of the monsters. For spells that should summon a specific monster,
    // make sure that the monster is in range for the summoners spell skill!
    Range get_allowed_mon_lvl_range(SpellSkill skill) const;

    std::vector<std::string> make_summon_bucket(const Range& mon_lvl_range) const;

    void summon(const std::string& id, actor::Actor* caster) const;

    std::unique_ptr<SummonImpl> m_impl;
};

class SpellMiGoHypno : public Spell
{
public:
    SpellMiGoHypno() = default;

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

class SpellBurn : public Spell
{
public:
    SpellBurn() = default;

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

class SpellDeafen : public Spell
{
public:
    SpellDeafen() = default;

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

class SpellHealOthers : public Spell
{
public:
    SpellHealOthers() = default;

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
    std::vector<actor::Actor*> find_possible_actors_to_heal(const actor::Actor* caster) const;

    actor::Actor* find_random_actor_to_heal(const actor::Actor* caster) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

#endif  // SPELLS_MONSTER_HPP
