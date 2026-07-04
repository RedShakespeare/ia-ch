// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SPELLS_CHANNELING_HPP
#define SPELLS_CHANNELING_HPP

#include "spells.hpp"

class BoltImpl
{
public:
    virtual ~BoltImpl() = default;

    virtual SpellShock shock_type() const;

    virtual Range damage(SpellSkill skill) const = 0;

    virtual void on_hit(
        actor::Actor& actor_hit,
        actor::Actor& caster,
        SpellSkill skill) const = 0;

    virtual std::string hit_msg_ending() const = 0;

    virtual audio::SfxId impact_sfx() const;

    virtual int mon_cooldown() const = 0;

    virtual std::string name() const = 0;

    virtual SpellId id() const = 0;

    virtual std::vector<std::string> descr_specific(SpellSkill skill) const = 0;

    virtual int base_max_cost(SpellSkill skill, const actor::Actor* caster) const = 0;

    virtual int nr_projectiles(SpellSkill skill) const
    {
        (void)skill;

        return 1;
    }
};

class ForceBolt : public BoltImpl
{
public:
    ForceBolt() = default;

    Range damage(SpellSkill skill) const override;

    void on_hit(
        actor::Actor& actor_hit,
        actor::Actor& caster,
        SpellSkill skill) const override;

    std::string hit_msg_ending() const override;

    int mon_cooldown() const override;

    std::string name() const override;

    SpellId id() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;
};

class Darkbolt : public BoltImpl
{
public:
    Darkbolt() = default;

    Range damage(SpellSkill skill) const override;

    void on_hit(
        actor::Actor& actor_hit,
        actor::Actor& caster,
        SpellSkill skill) const override;

    std::string hit_msg_ending() const override;

    int mon_cooldown() const override;

    std::string name() const override;

    SpellId id() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;
};

class GnawingTorrent : public BoltImpl
{
public:
    GnawingTorrent() = default;

    SpellShock shock_type() const override;

    Range damage(SpellSkill skill) const override;

    void on_hit(
        actor::Actor& actor_hit,
        actor::Actor& caster,
        SpellSkill skill) const override;

    std::string hit_msg_ending() const override;

    audio::SfxId impact_sfx() const override;

    int mon_cooldown() const override;

    std::string name() const override;

    SpellId id() const override;

    std::vector<std::string> descr_specific(SpellSkill skill) const override;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    int nr_projectiles(SpellSkill skill) const override;
};

class SpellBolt : public Spell
{
public:
    SpellBolt(BoltImpl* impl) :
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

    void draw_projectile_travel(
        const actor::Actor& caster,
        const actor::Actor& target,
        SpellSkill skill) const;

    void run_bolt_on_target(
        actor::Actor& caster,
        actor::Actor& target,
        SpellSkill skill,
        PlayerAwareOfCast player_aware) const;

    std::unique_ptr<BoltImpl> m_impl;
};

class SpellAzaGaze : public Spell
{
public:
    SpellAzaGaze() = default;

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

    Range faint_duration_range(SpellSkill skill) const;

    Range conflict_duration_range(SpellSkill skill) const;

    void run_effect_on_target(
        actor::Actor* caster,
        actor::Actor& target,
        SpellSkill skill,
        PlayerAwareOfCast player_aware) const;

    void do_damage_on_target(
        actor::Actor& target,
        SpellSkill skill,
        actor::Actor* caster) const;

    void apply_properties_on_target(
        actor::Actor& target,
        SpellSkill skill) const;
};

class SpellCataclysm : public Spell
{
public:
    SpellCataclysm() = default;

    bool allow_mon_cast_now(
        const actor::Actor& mon,
        const std::vector<actor::Actor*>& seen_targets) const override;

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
    int destruction_radi(SpellSkill skill) const;

    int nr_explosions(SpellSkill skill) const;

    int nr_destruction_sweeps(SpellSkill skill) const;

    int base_max_cost(SpellSkill skill, const actor::Actor* caster) const override;

    bool is_noisy(SpellSkill skill) const override;
};

#endif  // SPELLS_CHANNELING_HPP
