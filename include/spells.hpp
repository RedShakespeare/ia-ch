#ifndef SPELLS_HPP
#define SPELLS_HPP

#include <vector>
#include <string>
#include <unordered_map>

#include "item.hpp"
#include "player_bon.hpp"

class Actor;
class Mon;

enum class SpellId
{
        // Available for player and monsters
        aura_of_decay,
        darkbolt,
        enfeeble,
        heal,
        pestilence,
        slow,
        slow_time,
        spell_shield,
        summon,
        teleport,
        terrify,

        // Player only
        spectral_wpns, // TODO: Enable for monsters
        aza_wrath,
        bless,
        premonition,
        identify,
        light,
        mayhem,
        opening,
        res,
        searching,
        see_invis,
        transmut,

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

        // Spells from special sources
        pharaoh_staff, // From the Staff of the Pharaohs artifact

        END
};

const std::unordered_map<std::string, SpellId> str_to_spell_id_map =
{
        {"aura_of_decay", SpellId::aura_of_decay},
        {"spectral_wpns", SpellId::spectral_wpns},
        {"aza_wrath", SpellId::aza_wrath},
        {"bless", SpellId::bless},
        {"burn", SpellId::burn},
        {"force_bolt", SpellId::force_bolt},
        {"darkbolt", SpellId::darkbolt},
        {"deafen", SpellId::deafen},
        {"disease", SpellId::disease},
        {"premonition", SpellId::premonition},
        {"enfeeble", SpellId::enfeeble},
        {"frenzy", SpellId::frenzy},
        {"heal", SpellId::heal},
        {"identify", SpellId::identify},
        {"knockback", SpellId::knockback},
        {"light", SpellId::light},
        {"mayhem", SpellId::mayhem},
        {"mi_go_hypno", SpellId::mi_go_hypno},
        {"opening", SpellId::opening},
        {"pestilence", SpellId::pestilence},
        {"pharaoh_staff", SpellId::pharaoh_staff},
        {"res", SpellId::res},
        {"searching", SpellId::searching},
        {"see_invis", SpellId::see_invis},
        {"slow", SpellId::slow},
        {"slow_time", SpellId::slow_time},
        {"spell_shield", SpellId::spell_shield},
        {"summon", SpellId::summon},
        {"summon_tentacles", SpellId::summon_tentacles},
        {"teleport", SpellId::teleport},
        {"terrify", SpellId::terrify},
        {"transmut", SpellId::transmut}
};

enum class SpellSkill
{
        basic,
        expert,
        master
};

const std::unordered_map<std::string, SpellSkill> str_to_spell_skill_map =
{
        {"basic", SpellSkill::basic},
        {"expert", SpellSkill::expert},
        {"master", SpellSkill::master}
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

class Spell;

namespace spell_factory
{

Spell* make_spell_from_id(const SpellId spell_id);

} // spell_factory

class Spell
{
public:
        Spell() {}

        virtual ~Spell() {}

        void cast(Actor* const caster,
                  const SpellSkill skill,
                  const SpellSrc spell_src) const;

        virtual bool allow_mon_cast_now(Mon& mon) const
        {
                (void)mon;
                return false;
        }

        virtual int mon_cooldown() const
        {
                return 3;
        }

        virtual bool mon_can_learn() const = 0;

        virtual bool player_can_learn() const = 0;

        virtual std::string name() const = 0;

        virtual SpellId id() const = 0;

        virtual bool can_be_improved_with_skill() const
        {
                return true;
        }

        std::vector<std::string> descr(const SpellSkill skill,
                                       const SpellSrc spell_src) const;

        Range spi_cost(const SpellSkill skill,
                       Actor* const caster = nullptr) const;

        int shock_value() const;

        virtual SpellShock shock_type() const = 0;

        virtual void run_effect(
            Actor* const caster,
            const SpellSkill skill) const = 0;

protected:
        virtual int max_spi_cost(const SpellSkill skill) const = 0;

        virtual std::vector<std::string> descr_specific(
                const SpellSkill skill) const = 0;

        virtual bool is_noisy(const SpellSkill skill) const = 0;

        void on_resist(Actor& target) const;
};

class SpellEnfeeble: public Spell
{
public:
        SpellEnfeeble() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override;

        std::string name() const override
        {
                return "Enfeeble";
        }

        SpellId id() const override
        {
                return SpellId::enfeeble;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        bool mon_can_learn() const override
        {
                return true;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

protected:
        int max_spi_cost(const SpellSkill skill) const override;

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSlow: public Spell
{
public:
        SpellSlow() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override;

        std::string name() const override
        {
                return "Slow";
        }

        SpellId id() const override
        {
                return SpellId::slow;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        bool mon_can_learn() const override
        {
                return true;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

protected:
        int max_spi_cost(const SpellSkill skill) const override;

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellTerrify: public Spell
{
public:
        SpellTerrify() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override;

        std::string name() const override
        {
                return "Terrify";
        }

        SpellId id() const override
        {
                return SpellId::terrify;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        bool mon_can_learn() const override
        {
                return true;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

protected:
        int max_spi_cost(const SpellSkill skill) const override;

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellAuraOfDecay: public Spell
{
public:
        SpellAuraOfDecay() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override;

        bool mon_can_learn() const override
        {
                return true;
        }

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

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override;

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class BoltImpl
{
public:
        virtual ~BoltImpl() {}

        virtual Range damage(
                const SpellSkill skill,
                const Actor& caster) const = 0;

        virtual void on_hit(Actor& actor_hit, const SpellSkill skill) const = 0;

        virtual std::string hit_msg_ending() const = 0;

        virtual int mon_cooldown() const = 0;

        virtual bool mon_can_learn() const = 0;

        virtual bool player_can_learn() const = 0;

        virtual std::string name() const = 0;

        virtual SpellId id() const = 0;

        virtual std::vector<std::string> descr_specific(
                const SpellSkill skill) const = 0;

        virtual int max_spi_cost(const SpellSkill skill) const = 0;
};

class ForceBolt: public BoltImpl
{
public:
        ForceBolt() : BoltImpl() {}

        Range damage(
                const SpellSkill skill,
                const Actor& caster) const override;

        void on_hit(Actor& actor_hit, const SpellSkill skill) const override
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

        bool mon_can_learn() const override
        {
                return true;
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
                const SpellSkill skill) const override;

        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 2;
        }
};

class Darkbolt: public BoltImpl
{
public:
        Darkbolt() : BoltImpl() {}

        Range damage(
                const SpellSkill skill,
                const Actor& caster) const override;

        void on_hit(Actor& actor_hit, const SpellSkill skill) const override;

        std::string hit_msg_ending() const override
        {
                return "struck by a blast!";
        }

        int mon_cooldown() const override
        {
                return 5;
        }

        bool mon_can_learn() const override
        {
                return true;
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
                const SpellSkill skill) const override;

        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 4;
        }
};

class SpellBolt: public Spell
{
public:
        SpellBolt(BoltImpl* impl) :
                Spell(),
                impl_(impl) {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return impl_->mon_cooldown();
        }

        bool mon_can_learn() const override
        {
                return impl_->mon_can_learn();
        }

        bool player_can_learn() const override
        {
                return impl_->player_can_learn();
        }

        std::string name() const override
        {
                return impl_->name();
        }

        SpellId id() const override
        {
                return impl_->id();
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override
        {
                return impl_->descr_specific(skill);
        }

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                return impl_->max_spi_cost(skill);
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }

        std::unique_ptr<BoltImpl> impl_;
};

class SpellAzaWrath: public Spell
{
public:
        SpellAzaWrath() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 6;
        }

        bool mon_can_learn() const override
        {
                return false;
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

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 8;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellMayhem: public Spell
{
public:
        SpellMayhem() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        bool mon_can_learn() const override
        {
                return false;
        }

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

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override;

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellPestilence: public Spell
{
public:
        SpellPestilence() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 21;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSpectralWpns: public Spell
{
public:
        SpellSpectralWpns() : Spell() {}

        bool mon_can_learn() const override
        {
                return false;
        }

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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellPharaohStaff : public Spell
{
public:
        SpellPharaohStaff() {}
        ~SpellPharaohStaff() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        virtual bool mon_can_learn() const override
        {
                return false;
        }

        virtual bool player_can_learn() const override
        {
                return false;
        }

        virtual std::string name() const override
        {
                return "Summon Mummy Servant";
        }

        virtual SpellId id() const override
        {
                return SpellId::pharaoh_staff;
        }

        bool can_be_improved_with_skill() const override
        {
                return false;
        }

        virtual SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

protected:
        virtual int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return false;
        }
};

class SpellSearching: public Spell
{
public:
        SpellSearching() : Spell() {}

        bool mon_can_learn() const override
        {
                return false;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Searching";
        }

        SpellId id() const override
        {
                return SpellId::searching;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override;

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return false;
        }
};

class SpellOpening: public Spell
{
public:
        SpellOpening() : Spell() {}

        bool mon_can_learn() const override
        {
                return false;
        }

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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 4;
        }

        bool is_noisy(const SpellSkill skill) const override;
};

class SpellFrenzy: public Spell
{
public:
        SpellFrenzy() : Spell() {}

        bool mon_can_learn() const override
        {
                return false;
        }

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

        bool can_be_improved_with_skill() const override
        {
                return false;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 3;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return false;
        }
};

class SpellBless: public Spell
{
public:
        SpellBless() : Spell() {}

        bool mon_can_learn() const override
        {
                return false;
        }

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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 8;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return false;
        }
};

class SpellTransmut: public Spell
{
public:
        SpellTransmut() : Spell() {}

        bool mon_can_learn() const override
        {
                return false;
        }

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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 4;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return false;
        }
};

class SpellLight: public Spell
{
public:
        SpellLight() : Spell() {}

        bool mon_can_learn() const override
        {
                return false;
        }
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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 4;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return false;
        }
};

class SpellKnockBack: public Spell
{
public:
        SpellKnockBack() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 5;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override
        {
                (void)skill;

                return {""};
        }

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 8;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellTeleport: public Spell
{
public:
        SpellTeleport() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 30;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 10;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSeeInvis: public Spell
{
public:
        SpellSeeInvis() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 30;
        }

        bool mon_can_learn() const override
        {
                return false;
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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 8;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSpellShield: public Spell
{
public:
        SpellSpellShield() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 3;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override;

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSlowTime: public Spell
{
public:
        SpellSlowTime() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override;

        bool mon_can_learn() const override
        {
                return true;
        }

        bool player_can_learn() const override
        {
                return true;
        }

        std::string name() const override
        {
                return "Slow Time";
        }

        SpellId id() const override
        {
                return SpellId::slow_time;
        }

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override;

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return false;
        }
};

class SpellPremonition: public Spell
{
public:
        SpellPremonition() : Spell() {}

        bool mon_can_learn() const override
        {
                return false;
        }

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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override;

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return false;
        }
};

class SpellIdentify: public Spell
{
public:
        SpellIdentify() : Spell() {}

        bool mon_can_learn() const override
        {
                return false;
        }

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

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override;

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return false;
        }
};

class SpellRes: public Spell
{
public:
        SpellRes() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 20;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 7;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellDisease: public Spell
{
public:
        SpellDisease() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 10;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override
        {
                (void)skill;

                return {""};
        }

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 7;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSummonMon: public Spell
{
public:
        SpellSummonMon() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 8;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellSummonTentacles: public Spell
{
public:
        SpellSummonTentacles() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 5;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override
        {
                (void)skill;

                return {};
        }

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return false;
        }
};

class SpellHeal: public Spell
{
public:
        SpellHeal() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 6;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override;

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 6;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellMiGoHypno: public Spell
{
public:
        SpellMiGoHypno() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 5;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override
        {
                (void)skill;

                return {""};
        }

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 7;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellBurn: public Spell
{
public:
        SpellBurn() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 9;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::disturbing;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override
        {
                (void)skill;

                return {""};
        }

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 7;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

class SpellDeafen: public Spell
{
public:
        SpellDeafen() : Spell() {}

        bool allow_mon_cast_now(Mon& mon) const override;

        int mon_cooldown() const override
        {
                return 5;
        }

        bool mon_can_learn() const override
        {
                return true;
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

        SpellShock shock_type() const override
        {
                return SpellShock::mild;
        }

        std::vector<std::string> descr_specific(
                const SpellSkill skill) const override
        {
                (void)skill;

                return {""};
        }

        void run_effect(
            Actor* const caster,
            const SpellSkill skill) const override;

private:
        int max_spi_cost(const SpellSkill skill) const override
        {
                (void)skill;

                return 4;
        }

        bool is_noisy(const SpellSkill skill) const override
        {
                (void)skill;

                return true;
        }
};

#endif // SPELLS_HPP
