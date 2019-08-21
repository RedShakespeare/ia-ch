// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <math.h>

#include "actor.hpp"
#include "colors.hpp"
#include "global.hpp"
#include "insanity.hpp"


namespace item
{
class Explosive;
class Item;
class MedicalBag;
class Wpn;
}


enum class Phobia
{
        rat,
        spider,
        dog,
        undead,
        open_place,
        cramped_place,
        deep_places,
        dark,
        END
};

enum class Obsession
{
        sadism,
        masochism,
        END
};

enum class ShockSrc
{
        see_mon,
        use_strange_item,
        cast_intr_spell,
        time,
        misc,
        END
};


namespace actor
{

class Player: public Actor
{
public:
        Player();
        ~Player();

        void save() const;
        void load();

        void update_fov();

        void update_mon_awareness();

        bool can_see_actor(const Actor& other) const;

        std::vector<Actor*> seen_actors() const override;

        std::vector<Actor*> seen_foes() const override;

        bool is_seeing_burning_terrain() const;

        void act() override;

        Color color() const override;

        SpellSkill spell_skill(const SpellId id) const override;

        void on_actor_turn() override;
        void on_std_turn() override;

        void hear_sound(
                const Snd& snd,
                const bool is_origin_seen_by_player,
                const Dir dir_to_origin,
                const int percent_audible_distance);

        void incr_shock(const ShockLvl shock_lvl, ShockSrc shock_src);

        void incr_shock(const double shock, ShockSrc shock_src);

        double shock_lvl_to_value(const ShockLvl shock_lvl) const;

        void restore_shock(
                const int amount_restored,
                const bool is_temp_shock_restored);

        // Used for determining if '!'-marks should be drawn over the player
        double perm_shock_taken_current_turn() const
        {
                return m_perm_shock_taken_current_turn;
        }

        int shock_tot() const;

        int ins() const;

        void auto_melee();

        item::Wpn& unarmed_wpn();

        void set_unarmed_wpn(item::Wpn* wpn)
        {
                m_unarmed_wpn = wpn;
        }

        void kick_mon(Actor& defender);
        void hand_att(Actor& defender);

        void add_light_hook(Array2<bool>& light_map) const override;

        void on_log_msg_printed();  // Aborts e.g. searching and quick move
        void interrupt_actions();   // Aborts e.g. healing

        int enc_percent() const;

        int carry_weight_lmt() const;

        void set_auto_move(const Dir dir);

        bool is_leader_of(const Actor* const actor) const override;
        bool is_actor_my_leader(const Actor* const actor) const override;

        void on_new_dlvl_reached();

        // Randomly prints a message such as "I sense an object of great power
        // here" if there is a major treasure on the map (on the floor or in a
        // container), and the player is a Rogue
        void item_feeling();

        // Randomly prints a message such as "A chill runs down my spine" if
        // there are unique monsters on the map, and the player is a Rogue
        void mon_feeling();

        item::MedicalBag* m_active_medical_bag {nullptr};
        int m_handle_armor_countdown {0};
        int m_armor_putting_on_backpack_idx {-1};
        item::Item* m_wpn_equipping {nullptr};
        bool m_is_dropping_armor_from_body_slot {false};
        item::Explosive* m_active_explosive {nullptr};
        item::Item* m_last_thrown_item {nullptr};
        Actor* m_tgt {nullptr};
        int wait_turns_left {-1};
        int m_ins {0};
        double m_shock {0.0};
        double m_shock_tmp {0.0};
        double m_perm_shock_taken_current_turn {0.0};

private:
        void incr_insanity();

        void reset_perm_shock_taken_current_turn()
        {
                m_perm_shock_taken_current_turn = 0.0;
        }

        int shock_resistance(const ShockSrc shock_src) const;

        double shock_taken_after_mods(const double base_shock,
                                      const ShockSrc shock_src) const;

        void add_shock_from_seen_monsters();

        void update_tmp_shock();

        void on_hit(int& dmg,
                    const DmgType dmg_type,
                    const DmgMethod method,
                    const AllowWound allow_wound) override;

        void fov_hack();

        bool is_busy() const;

        int m_nr_turns_until_ins {-1};
        Dir m_auto_move_dir {Dir::END};
        bool m_has_taken_auto_move_step {false};
        int m_nr_turns_until_rspell {-1};
        item::Wpn* m_unarmed_wpn {nullptr};
};

} // actor

#endif // PLAYER_HPP
