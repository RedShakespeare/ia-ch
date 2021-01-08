// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef PROPERTY_HANDLER_HPP
#define PROPERTY_HANDLER_HPP

#include <memory>
#include <string>

#include "ability_values.hpp"
#include "direction.hpp"
#include "global.hpp"
#include "property.hpp"
#include "property_data.hpp"

namespace item
{
class Item;
class Wpn;
}  // namespace item

namespace actor
{
class Actor;
}  // namespace actor

struct P;

enum class PropEndAllowCallEndHook
{
        no,
        yes
};

enum class PropEndAllowMsg
{
        no,
        yes
};

enum class PropEndAllowHistoricMsg
{
        no,
        yes
};

struct PropTextListEntry
{
        ColoredString title {};
        std::string descr {};
        const Prop* prop {nullptr};
};

struct PropEndConfig
{
        PropEndConfig() = default;

        PropEndConfig(
                PropEndAllowCallEndHook end_hook_allowed,
                PropEndAllowMsg msg_allowed,
                PropEndAllowHistoricMsg historic_msg_allowed) :
                allow_end_hook(end_hook_allowed),
                allow_msg(msg_allowed),
                allow_historic_msg(historic_msg_allowed) {}

        const PropEndAllowCallEndHook allow_end_hook {
                PropEndAllowCallEndHook::yes};

        const PropEndAllowMsg allow_msg = {
                PropEndAllowMsg::yes};

        const PropEndAllowHistoricMsg allow_historic_msg {
                PropEndAllowHistoricMsg::yes};
};

// Each actor has an instance of this
class PropHandler
{
public:
        PropHandler(actor::Actor* owner);

        ~PropHandler() = default;

        PropHandler(const PropHandler&) = delete;

        PropHandler& operator=(const PropHandler&) = delete;

        void save() const;

        void load();

        // All properties must be added through this function (can also be done
        // via the other "apply" methods, which will then call "apply")
        void apply(
                Prop* prop,
                PropSrc src = PropSrc::intr,
                bool force_effect = false,
                Verbose verbose = Verbose::yes);

        void apply_natural_props_from_actor_data();

        // The following two methods are supposed to be called by items
        void add_prop_from_equipped_item(
                const item::Item* item,
                Prop* prop,
                Verbose verbose);

        void remove_props_for_item(const item::Item* item);

        // Fast method for checking if a certain property id is applied
        bool has(const PropId id) const
        {
                return m_prop_count_cache[(size_t)id] > 0;
        }

        Prop* prop(PropId id) const;

        bool end_prop(PropId id, const PropEndConfig& config = {});

        std::vector<ColoredString> property_names_short() const;

        std::vector<PropTextListEntry> property_names_and_descr() const;

        std::vector<PropTextListEntry> property_names_temporary_negative();

        bool has_temporary_negative_prop_mon() const;

        //----------------------------------------------------------------------
        // Hooks called from various places
        //----------------------------------------------------------------------
        void affect_move_dir(const P& actor_pos, Dir& dir) const;

        int affect_max_hp(int hp_max) const;
        int affect_max_spi(int spi_max) const;
        int affect_shock(int shock) const;

        bool allow_attack(Verbose verbose) const;
        bool allow_attack_melee(Verbose verbose) const;
        bool allow_attack_ranged(Verbose verbose) const;
        bool allow_see() const;
        bool allow_move() const;
        bool allow_act() const;
        bool allow_speak(Verbose verbose) const;
        bool allow_eat(Verbose verbose) const;  // Also for drinking
        bool allow_pray(Verbose verbose) const;  // Pray over the Holy Symbol

        // NOTE: The allow_*_absolute methods below answer if some action could
        // EVER be performed, and the allow_*_chance methods allows the action
        // with a random chance. For example, blindness never allows the player
        // to read scrolls, and the game won't let the player try. But burning
        // will allow the player to try, with a certain percent chance of
        // success, and the scroll will be wasted on failure. (All plain
        // allow_* methods above are also considered "absolute".)
        bool allow_read_absolute(Verbose verbose) const;
        bool allow_read_chance(Verbose verbose) const;
        bool allow_cast_intr_spell_absolute(Verbose verbose) const;
        bool allow_cast_intr_spell_chance(Verbose verbose) const;

        void on_hit();
        void on_death();
        void on_destroyed_alive();
        void on_destroyed_corpse();

        int ability_mod(AbilityId ability) const;

        bool affect_actor_color(Color& color) const;

        void on_placed();

        void on_new_dlvl();

        // Called when the actors turn begins/ends
        void on_turn_begin();
        void on_turn_end();

        void on_std_turn();

        // Called just before an actor is supposed to do an action (move,
        // attack,...). This may "take over" the actor and do some special
        // behavior instead (e.g. a Zombie rising, or a Vortex pulling),
        // possibly ticking game time - if time is ticked, this method returns
        // 'DidAction::yes' (each property implementing this callback must
        // make sure to do this).
        DidAction on_act();

        bool is_resisting_dmg(
                DmgType dmg_type,
                Verbose verbose) const;

private:
        void print_resist_msg(const Prop& prop);
        void print_start_msg(const Prop& prop);

        bool try_apply_more_on_existing_intr_prop(
                const Prop& new_prop,
                Verbose verbose);

        bool is_temporary_negative_prop(const Prop& prop) const;

        bool is_resisting_prop(PropId id) const;

        // A hook that prints messages, updates FOV, etc, and also calls the
        // on_end() property hook.
        // NOTE: It does NOT remove the property from the vector or decrement
        // the active property info. The caller is responsible for this.
        void on_prop_end(Prop* prop, const PropEndConfig& end_config);

        void incr_prop_count(PropId id);
        void decr_prop_count(PropId id);

        std::vector<std::unique_ptr<Prop>> m_props;

        // This array is only used as an optimization when requesting which
        // properties are currently active (see the "has()" method above).
        int m_prop_count_cache[(size_t)PropId::END];

        actor::Actor* m_owner;
};

#endif  // PROPERTY_HANDLER_HPP
