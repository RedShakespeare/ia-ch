// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef PROPERTY_HPP
#define PROPERTY_HPP

#include <string>

#include "ability_values.hpp"
#include "colors.hpp"
#include "debug.hpp"
#include "direction.hpp"
#include "global.hpp"
#include "property_data.hpp"

namespace item {
class Item;
} // namespace item

struct P;

// -----------------------------------------------------------------------------
// Support types
// -----------------------------------------------------------------------------
enum class PropSrc {
        // Properties applied by potions, spells, etc, or "natural" properties
        // for monsters (e.g. flying), or player properties gained by traits
        intr,

        // Properties applied by items carried in inventory
        inv,

        END
};

enum class PropDurationMode {
        standard,
        specific,
        indefinite
};

struct DmgResistData {
        DmgResistData() :
                is_resisted(false)
        {}

        bool is_resisted;
        std::string msg_resist_player;
        // Not including monster name, e.g. " seems unaffected"
        std::string msg_resist_mon;
};

enum class PropEnded {
        no,
        yes
};

struct PropActResult {
        PropActResult() :
                did_action(DidAction::no),
                prop_ended(PropEnded::no) {}

        DidAction did_action;
        PropEnded prop_ended;
};

// -----------------------------------------------------------------------------
// Property base class
// -----------------------------------------------------------------------------
class Prop {
public:
        Prop(PropId id);

        virtual ~Prop() = default;

        PropId id() const
        {
                return m_id;
        }

        virtual void save() const {}

        virtual void load() {}

        int nr_turns_left() const
        {
                return m_nr_turns_left;
        }

        void set_duration(const int nr_turns)
        {
                ASSERT(nr_turns > 0);

                m_duration_mode = PropDurationMode::specific;

                m_nr_turns_left = nr_turns;
        }

        void set_indefinite()
        {
                m_duration_mode = PropDurationMode::indefinite;

                m_nr_turns_left = -1;
        }

        PropDurationMode duration_mode() const
        {
                return m_duration_mode;
        }

        PropSrc src() const
        {
                return m_src;
        }

        virtual bool is_finished() const
        {
                return m_nr_turns_left == 0;
        }

        virtual PropAlignment alignment() const
        {
                return m_data.alignment;
        }

        virtual std::optional<Color> color_override() const
        {
                return {};
        }

        virtual bool allow_display_turns() const
        {
                return m_data.allow_display_turns;
        }

        virtual std::string name() const
        {
                return m_data.name;
        }

        virtual std::string name_short() const
        {
                return m_data.name_short;
        }

        std::string descr() const
        {
                return m_data.descr;
        }

        virtual std::string msg_end_player() const
        {
                return m_data.msg_end_player;
        }

        virtual bool should_update_vision_on_toggled() const
        {
                return m_data.update_vision_on_toggled;
        }

        virtual bool allow_see() const
        {
                return true;
        }

        virtual bool allow_move() const
        {
                return true;
        }

        virtual bool allow_act() const
        {
                return true;
        }

        virtual void on_hit() {}

        virtual void on_placed() {}

        virtual PropEnded on_tick()
        {
                return PropEnded::no;
        }

        virtual void on_std_turn() {}

        virtual PropActResult on_act()
        {
                return {};
        }

        virtual void on_applied() {}

        virtual void on_end() {}

        virtual void on_more(const Prop& new_prop)
        {
                (void)new_prop;
        }

        virtual PropEnded on_death()
        {
                return PropEnded::no;
        }

        virtual void on_destroyed_alive() {}

        virtual void on_destroyed_corpse() {}

        virtual int affect_max_hp(const int hp_max) const
        {
                return hp_max;
        }

        virtual int affect_max_spi(const int spi_max) const
        {
                return spi_max;
        }

        virtual int affect_shock(const int shock) const
        {
                return shock;
        }

        virtual bool affect_actor_color(Color& color) const
        {
                (void)color;
                return false;
        }

        virtual bool allow_attack_melee(const Verbose verbose) const
        {
                (void)verbose;
                return true;
        }

        virtual bool allow_attack_ranged(const Verbose verbose) const
        {
                (void)verbose;
                return true;
        }

        virtual bool allow_speak(const Verbose verbose) const
        {
                (void)verbose;
                return true;
        }

        virtual bool allow_eat(const Verbose verbose) const
        {
                (void)verbose;
                return true;
        }

        virtual bool allow_read_absolute(const Verbose verbose) const
        {
                (void)verbose;
                return true;
        }

        virtual bool allow_read_chance(const Verbose verbose) const
        {
                (void)verbose;
                return true;
        }

        virtual bool allow_cast_intr_spell_absolute(
                const Verbose verbose) const
        {
                (void)verbose;
                return true;
        }

        virtual bool allow_cast_intr_spell_chance(
                const Verbose verbose) const
        {
                (void)verbose;
                return true;
        }

        virtual int ability_mod(const AbilityId ability) const
        {
                (void)ability;
                return 0;
        }

        virtual PropEnded affect_move_dir(const P& actor_pos, Dir& dir)
        {
                (void)actor_pos;
                (void)dir;

                return PropEnded::no;
        }

        virtual bool is_resisting_other_prop(const PropId prop_id) const
        {
                (void)prop_id;
                return false;
        }

        virtual DmgResistData is_resisting_dmg(const DmgType dmg_type) const
        {
                (void)dmg_type;

                return DmgResistData();
        }

protected:
        friend class PropHandler;

        const PropId m_id;
        const PropData& m_data;

        int m_nr_turns_left;

        PropDurationMode m_duration_mode;

        actor::Actor* m_owner;
        PropSrc m_src;
        const item::Item* m_item_applying;
};

// -----------------------------------------------------------------------------
// Specific properties
// -----------------------------------------------------------------------------
class PropTerrified : public Prop {
public:
        PropTerrified() :
                Prop(PropId::terrified) {}

        int ability_mod(const AbilityId ability) const override
        {
                switch (ability) {
                case AbilityId::dodging:
                        return 20;

                case AbilityId::ranged:
                        return -20;

                default:
                        return 0;
                }
        }

        bool allow_attack_melee(Verbose verbose) const override;

        bool allow_attack_ranged(Verbose verbose) const override;

        void on_applied() override;
};

class PropInfected : public Prop {
public:
        PropInfected() :
                Prop(PropId::infected) {}

        std::optional<Color> color_override() const override
        {
                return colors::orange();
        }

        PropEnded on_tick() override;

        void on_applied() override;

private:
        bool has_warned {false};
};

class PropDiseased : public Prop {
public:
        PropDiseased() :
                Prop(PropId::diseased) {}

        int affect_max_hp(int hp_max) const override;

        bool is_resisting_other_prop(PropId prop_id) const override;

        void on_applied() override;
};

class PropDescend : public Prop {
public:
        PropDescend() :
                Prop(PropId::descend) {}

        PropEnded on_tick() override;
};

class PropBurrowing : public Prop {
public:
        PropBurrowing() :
                Prop(PropId::burrowing) {}

        PropEnded on_tick() override;
};

class PropZuulPossessPriest : public Prop {
public:
        PropZuulPossessPriest() :
                Prop(PropId::zuul_possess_priest) {}

        void on_placed() override;
};

class PropPossessedByZuul : public Prop {
public:
        PropPossessedByZuul() :
                Prop(PropId::possessed_by_zuul) {}

        PropEnded on_death() override;

        int affect_max_hp(const int hp_max) const override
        {
                return hp_max * 2;
        }
};

class PropShapeshifts : public Prop {
public:
        PropShapeshifts() :
                Prop(PropId::shapeshifts) {}

        void on_placed() override;

        void on_std_turn() override;

        PropEnded on_death() override;

private:
        void shapeshift(Verbose verbose) const;

        int m_countdown {-1};
};

class PropPoisoned : public Prop {
public:
        PropPoisoned() :
                Prop(PropId::poisoned) {}

        PropEnded on_tick() override;
};

class PropAiming : public Prop {
public:
        PropAiming() :
                Prop(PropId::aiming) {}

        int ability_mod(const AbilityId ability) const override
        {
                if (ability == AbilityId::ranged) {
                        return 10;
                } else {
                        return 0;
                }
        }

        void on_hit() override;
};

class PropBlind : public Prop {
public:
        PropBlind() :
                Prop(PropId::blind) {}

        bool should_update_vision_on_toggled() const override;

        bool allow_read_absolute(Verbose verbose) const override;

        bool allow_see() const override
        {
                return false;
        }

        int ability_mod(const AbilityId ability) const override
        {
                switch (ability) {
                case AbilityId::searching:
                        return -9999;

                case AbilityId::ranged:
                        return -20;

                case AbilityId::melee:
                        return -20;

                case AbilityId::dodging:
                        return -50;

                default:
                        return 0;
                }
        }
};

class PropRecloaks : public Prop {
public:
        PropRecloaks() :
                Prop(PropId::recloaks) {}

        PropActResult on_act() override;
};

class PropSeeInvis : public Prop {
public:
        PropSeeInvis() :
                Prop(PropId::see_invis) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;
};

class PropBlessed : public Prop {
public:
        PropBlessed() :
                Prop(PropId::blessed) {}

        void on_applied() override;

        void on_more(const Prop& new_prop) override;

        int ability_mod(AbilityId ability) const override;

private:
        void bless_adjacent() const;
};

class PropCursed : public Prop {
public:
        PropCursed() :
                Prop(PropId::cursed) {}

        int ability_mod(AbilityId ability) const override;

        void on_applied() override;

        void on_more(const Prop& new_prop) override;

private:
        void curse_adjacent() const;
};

class PropPremonition : public Prop {
public:
        PropPremonition() :
                Prop(PropId::premonition) {}

        int ability_mod(const AbilityId ability) const override
        {
                (void)ability;

                if (ability == AbilityId::dodging) {
                        return 75;
                } else {
                        return 0;
                }
        }
};

class PropMagicSearching : public Prop {
public:
        PropMagicSearching() :
                Prop(PropId::magic_searching),
                m_range(1),
                m_allow_reveal_items(false),
                m_allow_reveal_creatures(false) {}

        void save() const override;

        void load() override;

        PropEnded on_tick() override;

        void set_range(const int range)
        {
                m_range = range;
        }

        void set_allow_reveal_items()
        {
                m_allow_reveal_items = true;
        }

        void set_allow_reveal_creatures()
        {
                m_allow_reveal_creatures = true;
        }

private:
        int m_range;

        bool m_allow_reveal_items;
        bool m_allow_reveal_creatures;
};

class PropEntangled : public Prop {
public:
        PropEntangled() :
                Prop(PropId::entangled) {}

        PropEnded on_tick() override;

        void on_applied() override;

        PropEnded affect_move_dir(const P& actor_pos, Dir& dir) override;

private:
        bool try_player_end_with_machete();
};

class PropBurning : public Prop {
public:
        PropBurning() :
                Prop(PropId::burning) {}

        bool allow_read_chance(Verbose verbose) const override;

        bool allow_cast_intr_spell_chance(
                Verbose verbose) const override;

        int ability_mod(const AbilityId ability) const override
        {
                (void)ability;
                return -30;
        }

        bool affect_actor_color(Color& color) const override
        {
                color = colors::light_red();

                return true;
        }

        bool allow_attack_ranged(Verbose verbose) const override;

        PropEnded on_tick() override;
};

class PropFlared : public Prop {
public:
        PropFlared() :
                Prop(PropId::flared) {}

        PropEnded on_tick() override;
};

class PropConfused : public Prop {
public:
        PropConfused() :
                Prop(PropId::confused) {}

        PropEnded affect_move_dir(const P& actor_pos, Dir& dir) override;

        bool allow_attack_melee(Verbose verbose) const override;
        bool allow_attack_ranged(Verbose verbose) const override;
        bool allow_read_absolute(Verbose verbose) const override;
        bool allow_cast_intr_spell_absolute(
                Verbose verbose) const override;
};

class PropNailed : public Prop {
public:
        PropNailed() :
                Prop(PropId::nailed),
                m_nr_spikes(1) {}

        std::string name_short() const override
        {
                return "Nailed(" + std::to_string(m_nr_spikes) + ")";
        }

        PropEnded affect_move_dir(const P& actor_pos, Dir& dir) override;

        void on_more(const Prop& new_prop) override
        {
                (void)new_prop;

                ++m_nr_spikes;
        }

        bool is_finished() const override
        {
                return m_nr_spikes <= 0;
        }

private:
        int m_nr_spikes;
};

class PropWound : public Prop {
public:
        PropWound() :
                Prop(PropId::wound),
                m_nr_wounds(1) {}

        void save() const override;

        void load() override;

        std::string msg_end_player() const override
        {
                const auto str =
                        (m_nr_wounds > 1)
                        ? "All my wounds are healed!"
                        : "A wound is healed!";

                return str;
        }

        std::string name_short() const override
        {
                return "Wounded(" + std::to_string(m_nr_wounds) + ")";
        }

        int ability_mod(AbilityId ability) const override;

        void on_more(const Prop& new_prop) override;

        bool is_finished() const override
        {
                return m_nr_wounds <= 0;
        }

        int affect_max_hp(int hp_max) const override;

        int nr_wounds() const
        {
                return m_nr_wounds;
        }

        void heal_one_wound();

private:
        int m_nr_wounds;
};

class PropHpSap : public Prop {
public:
        PropHpSap();

        void save() const override;

        void load() override;

        std::string name_short() const override
        {
                return "Life Sapped(" + std::to_string(m_nr_drained) + ")";
        }

        void on_more(const Prop& new_prop) override;

        int affect_max_hp(int hp_max) const override;

private:
        int m_nr_drained;
};

class PropSpiSap : public Prop {
public:
        PropSpiSap();

        void save() const override;

        void load() override;

        std::string name_short() const override
        {
                return "Spirit Sapped(" + std::to_string(m_nr_drained) + ")";
        }

        void on_more(const Prop& new_prop) override;

        int affect_max_spi(int spi_max) const override;

private:
        int m_nr_drained;
};

class PropMindSap : public Prop {
public:
        PropMindSap();

        void save() const override;

        void load() override;

        std::string name_short() const override
        {
                return "Mind Sapped(" + std::to_string(m_nr_drained) + "%)";
        }

        void on_more(const Prop& new_prop) override;

        int affect_shock(int shock) const override;

private:
        int m_nr_drained;
};

class PropWaiting : public Prop {
public:
        PropWaiting() :
                Prop(PropId::waiting) {}

        bool allow_move() const override
        {
                return false;
        }

        bool allow_act() const override
        {
                return false;
        }

        bool allow_attack_melee(const Verbose verbose) const override
        {
                (void)verbose;
                return false;
        }

        bool allow_attack_ranged(const Verbose verbose) const override
        {
                (void)verbose;
                return false;
        }
};

class PropDisabledAttack : public Prop {
public:
        PropDisabledAttack() :
                Prop(PropId::disabled_attack) {}

        bool allow_attack_ranged(const Verbose verbose) const override
        {
                (void)verbose;
                return false;
        }

        bool allow_attack_melee(const Verbose verbose) const override
        {
                (void)verbose;
                return false;
        }
};

class PropDisabledMelee : public Prop {
public:
        PropDisabledMelee() :
                Prop(PropId::disabled_melee) {}

        bool allow_attack_melee(const Verbose verbose) const override
        {
                (void)verbose;
                return false;
        }
};

class PropDisabledRanged : public Prop {
public:
        PropDisabledRanged() :
                Prop(PropId::disabled_ranged) {}

        bool allow_attack_ranged(const Verbose verbose) const override
        {
                (void)verbose;
                return false;
        }
};

class PropParalyzed : public Prop {
public:
        PropParalyzed() :
                Prop(PropId::paralyzed) {}

        PropEnded on_tick() override;

        void on_applied() override;

        int ability_mod(const AbilityId ability) const override
        {
                if (ability == AbilityId::dodging) {
                        return -999;
                } else {
                        return 0;
                }
        }

        bool allow_act() const override
        {
                return false;
        }

        bool allow_attack_ranged(const Verbose verbose) const override
        {
                (void)verbose;
                return false;
        }

        bool allow_attack_melee(const Verbose verbose) const override
        {
                (void)verbose;
                return false;
        }
};

class PropFainted : public Prop {
public:
        PropFainted() :
                Prop(PropId::fainted) {}

        bool should_update_vision_on_toggled() const override;

        int ability_mod(const AbilityId ability) const override
        {
                if (ability == AbilityId::dodging) {
                        return -999;
                } else {
                        return 0;
                }
        }

        bool allow_act() const override
        {
                return false;
        }

        bool allow_see() const override
        {
                return false;
        }

        bool allow_attack_ranged(const Verbose verbose) const override
        {
                (void)verbose;
                return false;
        }

        bool allow_attack_melee(const Verbose verbose) const override
        {
                (void)verbose;
                return false;
        }

        void on_hit() override
        {
                m_nr_turns_left = 0;
        }
};

class PropSlowed : public Prop {
public:
        PropSlowed() :
                Prop(PropId::slowed) {}

        void on_applied() override;
};

class PropHasted : public Prop {
public:
        PropHasted() :
                Prop(PropId::hasted) {}

        void on_applied() override;
};

class PropClockworkHasted : public Prop {
public:
        PropClockworkHasted() :
                Prop(PropId::clockwork_hasted) {}

        void on_applied() override;
};

class PropSummoned : public Prop {
public:
        PropSummoned() :
                Prop(PropId::summoned) {}

        void on_end() override;
};

class PropFrenzied : public Prop {
public:
        PropFrenzied() :
                Prop(PropId::frenzied) {}

        void on_applied() override;
        void on_end() override;

        PropEnded affect_move_dir(const P& actor_pos, Dir& dir) override;

        bool allow_read_absolute(Verbose verbose) const override;
        bool allow_cast_intr_spell_absolute(
                Verbose verbose) const override;

        bool is_resisting_other_prop(PropId prop_id) const override;

        int ability_mod(const AbilityId ability) const override
        {
                if (ability == AbilityId::melee) {
                        return 10;
                } else {
                        return 0;
                }
        }
};

class PropRAcid : public Prop {
public:
        PropRAcid() :
                Prop(PropId::r_acid) {}

        DmgResistData is_resisting_dmg(DmgType dmg_type) const override;
};

class PropRConf : public Prop {
public:
        PropRConf() :
                Prop(PropId::r_conf) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;
};

class PropRElec : public Prop {
public:
        PropRElec() :
                Prop(PropId::r_elec) {}

        DmgResistData is_resisting_dmg(DmgType dmg_type) const override;
};

class PropRFear : public Prop {
public:
        PropRFear() :
                Prop(PropId::r_fear) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;
};

class PropRSlow : public Prop {
public:
        PropRSlow() :
                Prop(PropId::r_slow) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;
};

class PropRPhys : public Prop {
public:
        PropRPhys() :
                Prop(PropId::r_phys) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;

        DmgResistData is_resisting_dmg(DmgType dmg_type) const override;
};

class PropRFire : public Prop {
public:
        PropRFire() :
                Prop(PropId::r_fire) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;

        DmgResistData is_resisting_dmg(DmgType dmg_type) const override;
};

class PropRPoison : public Prop {
public:
        PropRPoison() :
                Prop(PropId::r_poison) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;
};

class PropRSleep : public Prop {
public:
        PropRSleep() :
                Prop(PropId::r_sleep) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;
};

class PropRDisease : public Prop {
public:
        PropRDisease() :
                Prop(PropId::r_disease) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;
};

class PropRBlind : public Prop {
public:
        PropRBlind() :
                Prop(PropId::r_blind) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;
};

class PropRPara : public Prop {
public:
        PropRPara() :
                Prop(PropId::r_para) {}

        void on_applied() override;

        bool is_resisting_other_prop(PropId prop_id) const override;
};

class PropRBreath : public Prop {
public:
        PropRBreath() :
                Prop(PropId::r_breath) {}
};

class PropLgtSens : public Prop {
public:
        PropLgtSens() :
                Prop(PropId::light_sensitive) {}
};

class PropVortex : public Prop {
public:
        PropVortex() :
                Prop(PropId::vortex),
                pull_cooldown(0) {}

        PropActResult on_act() override;

private:
        int pull_cooldown;
};

class PropExplodesOnDeath : public Prop {
public:
        PropExplodesOnDeath() :
                Prop(PropId::explodes_on_death) {}

        PropEnded on_death() override;
};

class PropSplitsOnDeath : public Prop {
public:
        PropSplitsOnDeath() :
                Prop(PropId::splits_on_death) {}

        PropEnded on_death() override;
};

class PropCorpseEater : public Prop {
public:
        PropCorpseEater() :
                Prop(PropId::corpse_eater) {}

        PropActResult on_act() override;
};

class PropTeleports : public Prop {
public:
        PropTeleports() :
                Prop(PropId::teleports) {}

        PropActResult on_act() override;
};

class PropCorruptsEnvColor : public Prop {
public:
        PropCorruptsEnvColor() :
                Prop(PropId::corrupts_env_color) {}

        PropActResult on_act() override;
};

class PropAltersEnv : public Prop {
public:
        PropAltersEnv() :
                Prop(PropId::alters_env) {}

        void on_std_turn() override;
};

class PropRegenerates : public Prop {
public:
        PropRegenerates() :
                Prop(PropId::regenerates) {}

        void on_std_turn() override;
};

class PropCorpseRises : public Prop {
public:
        PropCorpseRises() :
                Prop(PropId::corpse_rises),
                m_has_risen(false),
                m_nr_turns_until_allow_rise(2) {}

        PropActResult on_act() override;

        PropEnded on_death() override;

private:
        bool m_has_risen;
        int m_nr_turns_until_allow_rise;
};

class PropSpawnsZombiePartsOnDestroyed : public Prop {
public:
        PropSpawnsZombiePartsOnDestroyed() :
                Prop(PropId::spawns_zombie_parts_on_destroyed) {}

        void on_destroyed_alive() override;
        void on_destroyed_corpse() override;

private:
        void try_spawn_zombie_parts() const;

        void try_spawn_zombie_dust() const;

        bool is_allowed_to_spawn_parts_here() const;
};

class PropBreeds : public Prop {
public:
        PropBreeds() :
                Prop(PropId::breeds) {}

        void on_std_turn() override;
};

class PropConfusesAdjacent : public Prop {
public:
        PropConfusesAdjacent() :
                Prop(PropId::confuses_adjacent) {}

        void on_std_turn() override;
};

class PropSpeaksCurses : public Prop {
public:
        PropSpeaksCurses() :
                Prop(PropId::speaks_curses) {}

        PropActResult on_act() override;
};

class PropAuraOfDecay : public Prop {
public:
        PropAuraOfDecay() :
                Prop(PropId::aura_of_decay) {}

        void save() const override;

        void load() override;

        void on_std_turn() override;

        void set_max_dmg(const int dmg)
        {
                m_max_dmg = dmg;
        }

private:
        int range() const;

        void run_effect_on_actors() const;

        void run_effect_on_env() const;

        void print_msg_actor_hit(const actor::Actor& actor) const;

        int m_max_dmg {1};
};

class PropMajorClaphamSummon : public Prop {
public:
        PropMajorClaphamSummon() :
                Prop(PropId::major_clapham_summon) {}

        PropActResult on_act() override;
};

class PropSwimming : public Prop {
public:
        PropSwimming() :
                Prop(PropId::swimming) {}

        bool allow_read_absolute(Verbose verbose) const override;

        bool affect_actor_color(Color& color) const override
        {
                color = colors::light_blue();

                return true;
        }

        bool allow_attack_ranged(Verbose verbose) const override;

        int ability_mod(const AbilityId ability) const override
        {
                switch (ability) {
                case AbilityId::melee:
                        return -10;

                case AbilityId::dodging:
                        return -10;

                default:
                        return 0;
                }
        }
};

class PropHitChancePenaltyCurse : public Prop {
public:
        PropHitChancePenaltyCurse() :
                Prop(PropId::hit_chance_penalty_curse) {}

        int ability_mod(const AbilityId ability) const override
        {
                switch (ability) {
                case AbilityId::melee:
                case AbilityId::ranged:
                        return -10;

                default:
                        return 0;
                }
        }
};

class PropIncreasedShockCurse : public Prop {
public:
        PropIncreasedShockCurse() :
                Prop(PropId::increased_shock_curse) {}

        int affect_shock(const int shock) const override
        {
                return shock + 10;
        }
};

class PropCannotReadCurse : public Prop {
public:
        PropCannotReadCurse() :
                Prop(PropId::cannot_read_curse) {}

        bool allow_read_absolute(Verbose verbose) const override;
};

#endif // PROPERTY_HPP
