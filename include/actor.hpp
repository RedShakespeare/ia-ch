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

struct SneakData
{
        const Actor* actor_sneaking {nullptr};
        const Actor* actor_searching {nullptr};
};

// -----------------------------------------------------------------------------
// Actor
// -----------------------------------------------------------------------------
class Actor
{
public:
        virtual ~Actor();

        int ability(
                const AbilityId id,
                const bool is_affected_by_props) const;

        bool restore_hp(
                const int hp_restored,
                const bool is_allowed_above_max = false,
                const Verbosity verbosity = Verbosity::verbose);

        bool restore_sp(
                const int sp_restored,
                const bool is_allowed_above_max = false,
                const Verbosity verbosity = Verbosity::verbose);

        void change_max_hp(
                const int change,
                const Verbosity verbosity = Verbosity::verbose);

        void change_max_sp(
                const int change,
                const Verbosity verbosity = Verbosity::verbose);

        void destroy();

        // Used by Ghoul class and Ghoul monsters
        DidAction try_eat_corpse();

        void on_feed();

        void on_std_turn_common();

        virtual void act() {}

        virtual void on_actor_turn() {}
        virtual void on_std_turn() {}

        virtual TileId tile() const;

        virtual char character() const;

        virtual Color color() const = 0;

        virtual std::vector<Actor*> seen_actors() const = 0;

        virtual std::vector<Actor*> seen_foes() const = 0;

        ActorId id() const
        {
                return data->id;
        }

        int armor_points() const;

        virtual SpellSkill spell_skill(const SpellId id) const = 0;

        virtual std::string name_the() const
        {
                return data->name_the;
        }

        virtual std::string name_a() const
        {
                return data->name_a;
        }

        virtual std::string descr() const
        {
                return data->descr;
        }

        void add_light(Array2<bool>& light_map) const;

        virtual void add_light_hook(Array2<bool>& light) const
        {
                (void)light;
        }

        bool is_alive() const
        {
                return state == ActorState::alive;
        }

        bool is_corpse() const
        {
                return state == ActorState::corpse;
        }

        virtual bool is_leader_of(const Actor* const actor) const = 0;
        virtual bool is_actor_my_leader(const Actor* const actor) const = 0;

        bool is_player() const;

        std::string death_msg() const;

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

        P pos {};
        ActorState state {ActorState::alive};
        int hp {-1};
        int base_max_hp {-1};
        int sp {-1};
        int base_max_sp {-1};
        PropHandler properties {this};
        Inventory inv {this};
        ActorData* data {nullptr};
        int delay {0};
        P lair_pos {};

protected:
        // Damages worn armor, and returns damage after armor absorbs damage
        int hit_armor(int dmg);
};

namespace actor
{

int max_hp(const Actor& actor);

int max_sp(const Actor& actor);

void init_actor(Actor& actor, const P& pos_, ActorData& data);

// This function is not concerned with whether actors are within FOV, or if they
// are actually hidden or not. It merely performs a skill check, taking various
// conditions such as light/dark into concern.
ActionResult roll_sneak(const SneakData& data);

void print_aware_invis_mon_msg(const Mon& mon);

} // actor

#endif // ACTOR_HPP
