// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ACTOR_HPP
#define ACTOR_HPP

#include <string>
#include <vector>

#include "actor_data.hpp"
#include "config.hpp"
#include "gfx.hpp"
#include "global.hpp"
#include "inventory.hpp"
#include "property_handler.hpp"
#include "sound.hpp"


namespace actor
{

class Actor;


struct SneakData
{
        const actor::Actor* actor_sneaking {nullptr};
        const actor::Actor* actor_searching {nullptr};
};


int max_hp(const Actor& actor);

int max_sp(const Actor& actor);

void init_actor(Actor& actor, const P& pos_, ActorData& data);

// This function is not concerned with whether actors are within FOV, or if they
// are actually hidden or not. It merely performs a skill check, taking various
// conditions such as light/dark into concern.
ActionResult roll_sneak(const SneakData& data);

void print_aware_invis_mon_msg(const Mon& mon);


class Actor
{
public:
        virtual ~Actor();

        int ability(
                AbilityId id,
                bool is_affected_by_props) const;

        bool restore_hp(
                int hp_restored,
                bool is_allowed_above_max = false,
                Verbose verbose = Verbose::yes);

        bool restore_sp(
                int spi_restored,
                bool is_allowed_above_max = false,
                Verbose verbose = Verbose::yes);

        void change_max_hp(
                int change,
                Verbose verbose = Verbose::yes);

        void change_max_sp(
                int change,
                Verbose verbose = Verbose::yes);

        // Used by Ghoul class and Ghoul monsters
        DidAction try_eat_corpse();

        void on_feed();

        void on_std_turn_common();

        Id id() const
        {
                return m_data->id;
        }

        int armor_points() const;

        void add_light(Array2<bool>& light_map) const;

        bool is_alive() const
        {
                return m_state == ActorState::alive;
        }

        bool is_corpse() const
        {
                return m_state == ActorState::corpse;
        }

        bool is_player() const;

        std::string death_msg() const;

        virtual void act() {}

        virtual void on_actor_turn() {}

        virtual void on_std_turn() {}

        virtual TileId tile() const;

        virtual char character() const;

        virtual std::string name_the() const
        {
                return m_data->name_the;
        }

        virtual std::string name_a() const
        {
                return m_data->name_a;
        }

        virtual std::string descr() const
        {
                return m_data->descr;
        }

        virtual void add_light_hook(Array2<bool>& light) const
        {
                (void)light;
        }

        virtual void on_hit(
                int& dmg,
                const DmgType dmg_type,
                const DmgMethod method,
                const AllowWound allow_wound)
        {
                (void)dmg;
                (void)dmg_type;
                (void)method;
                (void)allow_wound;
        }

        virtual void on_death() {}

        virtual Color color() const = 0;

        virtual std::vector<Actor*> seen_actors() const = 0;

        virtual std::vector<Actor*> seen_foes() const = 0;

        virtual SpellSkill spell_skill(SpellId id) const = 0;

        virtual bool is_leader_of(const Actor* actor) const = 0;

        virtual bool is_actor_my_leader(const Actor* actor) const = 0;

        P m_pos {};
        ActorState m_state {ActorState::alive};
        int m_hp {-1};
        int m_base_max_hp {-1};
        int m_sp {-1};
        int m_base_max_sp {-1};
        PropHandler m_properties {this};
        Inventory m_inv {this};
        ActorData* m_data {nullptr};
        int m_delay {0};
        P m_lair_pos {};
        P m_opening_door_pos {-1, -1};

protected:
        // Damages worn armor, and returns damage after armor absorbs damage
        int hit_armor(int dmg);
};

} // namespace actor

#endif // ACTOR_HPP
