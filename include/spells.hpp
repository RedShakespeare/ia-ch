// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SPELLS_HPP
#define SPELLS_HPP

#include <string>
#include <vector>

#include "item.hpp"
#include "player_bon.hpp"

namespace actor
{
class Actor;
class Mon;
}  // namespace actor

enum class SpellId
{
        // Available for player and monsters
        aura_of_decay,
        darkbolt,
        enfeeble,
        heal,
        pestilence,
        slow,
        haste,
        spell_shield,
        summon,
        teleport,
        terrify,

        // Player only
        spectral_wpns,  // TODO: Enable for monsters
        aza_wrath,
        bless,
        premonition,
        identify,
        light,
        mayhem,
        opening,
        res,
        see_invis,
        transmut,

        // Exorcist background
        cleansing_fire,
        sanctuary,
        purge,

        // Ghoul background
        frenzy,

        // Monsters only
        force_bolt,
        burn,
        deafen,
        disease,
        knockback,
        mi_go_hypno,
        summon_tentacles,

        END
};

enum class SpellSkill
{
        basic,
        expert,
        master
};

enum class SpellSrc
{
        learned,
        manuscript,
        item
};

enum class SpellShock
{
        mild,
        disturbing,
        severe
};

namespace spells
{
Spell* make( SpellId spell_id );

SpellId str_to_spell_id( const std::string& str );

SpellSkill str_to_spell_skill_id( const std::string& str );

std::string skill_to_str( SpellSkill skill );

}  // namespace spells

class Spell
{
public:
        Spell() = default;

        virtual ~Spell() = default;

        void cast(
                actor::Actor* caster,
                SpellSkill skill,
                SpellSrc spell_src ) const;

        virtual bool allow_mon_cast_now( actor::Mon& mon ) const
        {
                (void)mon;
                return false;
        }

        virtual int mon_cooldown() const
        {
                return 3;
        }

        virtual bool player_can_learn() const = 0;

        virtual std::string name() const = 0;

        virtual SpellId id() const = 0;

        virtual OccultistDomain domain() const = 0;

        virtual bool can_be_improved_with_skill() const
        {
                return true;
        }

        std::vector<std::string> descr(
                SpellSkill skill,
                SpellSrc spell_src ) const;

        std::string domain_descr() const;

        virtual std::vector<std::string> descr_specific(
                SpellSkill skill ) const = 0;

        Range spi_cost( SpellSkill skill ) const;

        int shock_value() const;

        virtual SpellShock shock_type() const = 0;

        virtual void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const = 0;

protected:
        virtual int max_spi_cost( SpellSkill skill ) const = 0;

        virtual bool is_noisy( SpellSkill skill ) const = 0;

        void on_resist( actor::Actor& target ) const;
};

class SpellEnfeeble : public Spell
{
public:
        SpellEnfeeble() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override;

        std::string name() const override
        {
                return "Enfeeble";
        }

        SpellId id() const override
        {
                return SpellId::enfeeble;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::enchanter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

protected:
        int max_spi_cost( SpellSkill skill ) const override;

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSlow : public Spell
{
public:
        SpellSlow() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override;

        std::string name() const override
        {
                return "Slow";
        }

        SpellId id() const override
        {
                return SpellId::slow;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::enchanter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

protected:
        int max_spi_cost( SpellSkill skill ) const override;

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellTerrify : public Spell
{
public:
        SpellTerrify() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override;

        std::string name() const override
        {
                return "Terrify";
        }

        SpellId id() const override
        {
                return SpellId::terrify;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::enchanter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

protected:
        int max_spi_cost( SpellSkill skill ) const override;

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }

        Range duration_range( SpellSkill skill ) const;
};

class SpellAuraOfDecay : public Spell
{
public:
        SpellAuraOfDecay() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Aura of Decay";
        }

        SpellId id() const override
        {
                return SpellId::aura_of_decay;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::invoker;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( SpellSkill skill ) const override;

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class BoltImpl
{
public:
        virtual ~BoltImpl() = default;

        virtual Range damage(
                SpellSkill skill,
                const actor::Actor& caster ) const = 0;

        virtual void on_hit(
                actor::Actor& actor_hit,
                SpellSkill skill ) const = 0;

        virtual std::string hit_msg_ending() const = 0;

        virtual int mon_cooldown() const = 0;

        virtual bool player_can_learn() const = 0;

        virtual std::string name() const = 0;

        virtual SpellId id() const = 0;

        virtual std::vector<std::string> descr_specific(
                SpellSkill skill ) const = 0;

        virtual int max_spi_cost( SpellSkill skill ) const = 0;
};

class ForceBolt : public BoltImpl
{
public:
        ForceBolt() = default;

        Range damage(
                SpellSkill skill,
                const actor::Actor& caster ) const override;

        void on_hit(
                actor::Actor& actor_hit,
                const SpellSkill skill ) const override
        {
                (void)actor_hit;
                (void)skill;
        }

        std::string hit_msg_ending() const override
        {
                return "struck by a bolt!";
        }

        int mon_cooldown() const override
        {
                return 3;
        }

        bool player_can_learn() const override
        {
                return false;
        }

        std::string name() const override
        {
                return "Force Bolt";
        }

        SpellId id() const override
        {
                return SpellId::force_bolt;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 2;
        }
};

class Darkbolt : public BoltImpl
{
public:
        Darkbolt() = default;

        Range damage(
                SpellSkill skill,
                const actor::Actor& caster ) const override;

        void on_hit(
                actor::Actor& actor_hit,
                SpellSkill skill ) const override;

        std::string hit_msg_ending() const override
        {
                return "struck by a blast!";
        }

        int mon_cooldown() const override
        {
                return 5;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Darkbolt";
        }

        SpellId id() const override
        {
                return SpellId::darkbolt;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 4;
        }
};

class SpellBolt : public Spell
{
public:
        SpellBolt( BoltImpl* impl ) :

                m_impl( impl )
        {}

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return m_impl->mon_cooldown();
        }

        bool player_can_learn() const override
        {
                return m_impl->player_can_learn();
        }

        std::string name() const override
        {
                return m_impl->name();
        }

        SpellId id() const override
        {
                return m_impl->id();
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::invoker;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill ) const override
        {
                return m_impl->descr_specific( skill );
        }

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                return m_impl->max_spi_cost( skill );
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }

        std::unique_ptr<BoltImpl> m_impl;
};

class SpellAzaWrath : public Spell
{
public:
        SpellAzaWrath() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 6;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Azathoth's Wrath";
        }

        SpellId id() const override
        {
                return SpellId::aza_wrath;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::invoker;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 8;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellMayhem : public Spell
{
public:
        SpellMayhem() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Mayhem";
        }

        SpellId id() const override
        {
                return SpellId::mayhem;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::invoker;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( SpellSkill skill ) const override;

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellPestilence : public Spell
{
public:
        SpellPestilence() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 21;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Pestilence";
        }

        SpellId id() const override
        {
                return SpellId::pestilence;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSpectralWpns : public Spell
{
public:
        SpellSpectralWpns() = default;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Spectral Weapons";
        }

        SpellId id() const override
        {
                return SpellId::spectral_wpns;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellOpening : public Spell
{
public:
        SpellOpening() = default;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Opening";
        }

        SpellId id() const override
        {
                return SpellId::opening;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::transmuter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 4;
        }

        bool is_noisy( SpellSkill skill ) const override;
};

class SpellCleansingFire : public Spell
{
public:
        SpellCleansingFire() = default;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Cleansing Fire";
        }

        SpellId id() const override
        {
                return SpellId::cleansing_fire;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        bool can_be_improved_with_skill() const override
        {
                return true;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 7;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }

        Range burn_duration_range() const;
};

class SpellSanctuary : public Spell
{
public:
        SpellSanctuary() = default;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Sanctuary";
        }

        SpellId id() const override
        {
                return SpellId::sanctuary;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        bool can_be_improved_with_skill() const override
        {
                return true;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 5;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return false;
        }

        Range duration( SpellSkill skill ) const;
};

class SpellPurge : public Spell
{
public:
        SpellPurge() = default;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Purge";
        }

        SpellId id() const override
        {
                return SpellId::purge;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        bool can_be_improved_with_skill() const override
        {
                return false;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 4;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }

        Range dmg_range() const;

        Range fear_duration_range() const;
};

class SpellFrenzy : public Spell
{
public:
        SpellFrenzy() = default;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Incite Frenzy";
        }

        SpellId id() const override
        {
                return SpellId::frenzy;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        bool can_be_improved_with_skill() const override
        {
                return false;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 0;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return false;
        }
};

class SpellBless : public Spell
{
public:
        SpellBless() = default;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Bless";
        }

        SpellId id() const override
        {
                return SpellId::bless;
        }

        OccultistDomain domain() const override
        {
                // NOTE: This could perhaps be considered an enchantment spell,
                // but the way the spell description is phrased, it sounds a lot
                // more like alteration
                return OccultistDomain::transmuter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 8;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return false;
        }
};

class SpellTransmut : public Spell
{
public:
        SpellTransmut() = default;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Transmutation";
        }

        SpellId id() const override
        {
                return SpellId::transmut;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::transmuter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 4;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return false;
        }
};

class SpellLight : public Spell
{
public:
        SpellLight() = default;
        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Light";
        }

        SpellId id() const override
        {
                return SpellId::light;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::transmuter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 4;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return false;
        }
};

class SpellKnockBack : public Spell
{
public:
        SpellKnockBack() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 5;
        }

        bool player_can_learn() const override
        {
                return false;
        }

        std::string name() const override
        {
                return "Knockback";
        }

        SpellId id() const override
        {
                return SpellId::knockback;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill ) const override
        {
                (void)skill;

                return { "" };
        }

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 8;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellTeleport : public Spell
{
public:
        SpellTeleport() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 20;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Teleport";
        }

        SpellId id() const override
        {
                return SpellId::teleport;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::transmuter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 8;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }

        int max_dist( SpellSkill skill ) const;
};

class SpellSeeInvis : public Spell
{
public:
        SpellSeeInvis() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 30;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "See Invisible";
        }

        SpellId id() const override
        {
                return SpellId::see_invis;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::clairvoyant;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 8;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSpellShield : public Spell
{
public:
        SpellSpellShield() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 3;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Spell Shield";
        }

        SpellId id() const override
        {
                return SpellId::spell_shield;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::enchanter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( SpellSkill skill ) const override;

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellHaste : public Spell
{
public:
        SpellHaste() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Haste";
        }

        SpellId id() const override
        {
                return SpellId::haste;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::transmuter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( SpellSkill skill ) const override;

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return false;
        }
};

class SpellPremonition : public Spell
{
public:
        SpellPremonition() = default;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Premonition";
        }

        SpellId id() const override
        {
                return SpellId::premonition;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::clairvoyant;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( SpellSkill skill ) const override;

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return false;
        }
};

class SpellIdentify : public Spell
{
public:
        SpellIdentify() = default;

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Identify";
        }

        SpellId id() const override
        {
                return SpellId::identify;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::clairvoyant;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( SpellSkill skill ) const override;

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return false;
        }
};

class SpellRes : public Spell
{
public:
        SpellRes() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 20;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Resistance";
        }

        SpellId id() const override
        {
                return SpellId::res;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::enchanter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 7;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellDisease : public Spell
{
public:
        SpellDisease() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 10;
        }

        bool player_can_learn() const override
        {
                return false;
        }

        std::string name() const override
        {
                return "Disease";
        }

        SpellId id() const override
        {
                return SpellId::disease;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill ) const override
        {
                (void)skill;

                return { "" };
        }

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 7;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSummonMon : public Spell
{
public:
        SpellSummonMon() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 8;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Summon Creature";
        }

        SpellId id() const override
        {
                return SpellId::summon;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSummonTentacles : public Spell
{
public:
        SpellSummonTentacles() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 3;
        }

        bool player_can_learn() const override
        {
                return false;
        }

        std::string name() const override
        {
                return "";
        }

        SpellId id() const override
        {
                return SpellId::summon_tentacles;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill ) const override
        {
                (void)skill;

                return {};
        }

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return false;
        }
};

class SpellHeal : public Spell
{
public:
        SpellHeal() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 6;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Healing";
        }

        SpellId id() const override
        {
                return SpellId::heal;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::enchanter;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                SpellSkill skill ) const override;

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellMiGoHypno : public Spell
{
public:
        SpellMiGoHypno() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 5;
        }

        bool player_can_learn() const override
        {
                return false;
        }

        std::string name() const override
        {
                return "MiGo Hypnosis";
        }

        SpellId id() const override
        {
                return SpellId::mi_go_hypno;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill ) const override
        {
                (void)skill;

                return { "" };
        }

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 7;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellBurn : public Spell
{
public:
        SpellBurn() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 9;
        }

        bool player_can_learn() const override
        {
                return false;
        }

        std::string name() const override
        {
                return "Immolation";
        }

        SpellId id() const override
        {
                return SpellId::burn;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill ) const override
        {
                (void)skill;

                return { "" };
        }

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 7;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

class SpellDeafen : public Spell
{
public:
        SpellDeafen() = default;

        bool allow_mon_cast_now( actor::Mon& mon ) const override;

        int mon_cooldown() const override
        {
                return 5;
        }

        bool player_can_learn() const override
        {
                return false;
        }

        std::string name() const override
        {
                return "Deafen";
        }

        SpellId id() const override
        {
                return SpellId::deafen;
        }

        OccultistDomain domain() const override
        {
                return OccultistDomain::END;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill ) const override
        {
                (void)skill;

                return { "" };
        }

        void run_effect(
                actor::Actor* caster,
                SpellSkill skill ) const override;

private:
        int max_spi_cost( const SpellSkill skill ) const override
        {
                (void)skill;

                return 4;
        }

        bool is_noisy( const SpellSkill skill ) const override
        {
                (void)skill;

                return true;
        }
};

#endif  // SPELLS_HPP
