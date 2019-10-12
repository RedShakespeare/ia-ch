// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MON_HPP
#define MON_HPP

#include <memory>
#include <string>
#include <vector>

#include "actor.hpp"
#include "global.hpp"
#include "item.hpp"
#include "sound.hpp"
#include "spells.hpp"

struct AiAttData
{
        item::Wpn* wpn = nullptr;
        bool is_melee = false;
};

struct AiAvailAttacksData
{
        std::vector<item::Wpn*> weapons = {};
        bool should_reload = false;
        bool is_melee = false;
};

struct MonSpell
{
        MonSpell() :
                
                skill((SpellSkill)0)
                {}

        Spell* spell{nullptr};
        SpellSkill skill;
        int cooldown{-1};
};


namespace actor
{

std::string get_cultist_phrase();

std::string get_cultist_aware_msg_seen(const Actor& actor);

std::string get_cultist_aware_msg_hidden();


class Mon: public Actor
{
public:
        Mon();
        ~Mon() override;

        bool can_see_actor(const Actor& other,
                           const Array2<bool>& hard_blocked_los) const;

        std::vector<Actor*> seen_actors() const override;

        std::vector<Actor*> seen_foes() const override;

        // Actors which are possible to see (i.e. not impossible due to
        // invisibility, etc), but may or may not currently be seen due to
        // (lack of) awareness
        std::vector<Actor*> seeable_foes() const;

        bool is_sneaking() const;

        void act() override;

        Color color() const override;

        SpellSkill spell_skill(const SpellId id) const override;

        AiAvailAttacksData avail_attacks(Actor& defender) const;

        AiAttData choose_attack(const AiAvailAttacksData& avail_attacks) const;

        DidAction try_attack(Actor& defender);

        void hear_sound(const Snd& snd);

        void become_aware_player(const bool is_from_seeing,
                                 const int factor = 1);

        void become_wary_player();

        void set_player_aware_of_me(int duration_factor = 1);

        void on_actor_turn() override;

        void on_std_turn() final;

        std::string aware_msg_mon_seen() const;

        std::string aware_msg_mon_hidden() const;

        virtual SfxId aware_sfx_mon_seen() const
        {
                return m_data->aware_sfx_mon_seen;
        }

        virtual SfxId aware_sfx_mon_hidden() const
        {
                return m_data->aware_sfx_mon_hidden;
        }

        void speak_phrase(const AlertsMon alerts_others);

        bool is_leader_of(const Actor* const actor) const override;
        bool is_actor_my_leader(const Actor* const actor) const override;

        void add_spell(SpellSkill skill, Spell* const spell);

        int m_wary_of_player_counter{0};
        int m_aware_of_player_counter{0};
        int m_player_aware_of_me_counter{0};
        bool m_is_msg_mon_in_view_printed{false};
        bool m_is_player_feeling_msg_allowed{true};
        Dir m_last_dir_moved{Dir::center};
        MonRoamingAllowed m_is_roaming_allowed{MonRoamingAllowed::yes};
        Actor* m_leader{nullptr};
        Actor* m_target{nullptr};
        bool m_is_target_seen{false};
        bool m_waiting{false};

        std::vector<MonSpell> m_spells;

protected:
        std::vector<Actor*> unseen_foes_aware_of() const;

        // Return value 'true' means it is possible to see the other actor (i.e.
        // it's not impossible due to invisibility, etc), but the actor may or
        // may not currently be seen due to (lack of) awareness
        bool is_actor_seeable(
                const Actor& other,
                const Array2<bool>& hard_blocked_los) const;

        void make_leader_aware_silent() const;

        void print_player_see_mon_become_aware_msg() const;

        void print_player_see_mon_become_wary_msg() const;

        bool is_friend_blocking_ranged_attack(const P& target_pos) const;

        item::Wpn* avail_wielded_melee() const;
        item::Wpn* avail_wielded_ranged() const;
        std::vector<item::Wpn*> avail_intr_melee() const;
        std::vector<item::Wpn*> avail_intr_ranged() const;

        bool should_reload(const item::Wpn& wpn) const;

        void on_hit(
                int& dmg,
                const DmgType dmg_type,
                const DmgMethod method,
                const AllowWound allow_wound) override;

        virtual DidAction on_act()
        {
                return DidAction::no;
        }

        // TODO: This will be removed
        virtual void on_std_turn_hook() {}

        int nr_mon_in_group() const;
};

class Ape: public Mon
{
public:
        Ape() :
                Mon()
                {}

        ~Ape() override = default;

private:
        DidAction on_act() override;

        int m_frenzy_cooldown{0};
};

class Khephren: public Mon
{
public:
        Khephren() :
                Mon()
                {}
        ~Khephren() override = default;

private:
        DidAction on_act() override;

        bool m_has_summoned_locusts{false};
};

class StrangeColor: public Mon
{
public:
        StrangeColor() : Mon() {}

        ~StrangeColor() override = default;

        Color color() const override;
};

class SpectralWpn: public Mon
{
public:
        SpectralWpn();

        ~SpectralWpn() override = default;

        void on_death() override;

        std::string name_the() const override;

        std::string name_a() const override;

        char character() const override;

        TileId tile() const override;

        std::string descr() const override;

private:
        std::unique_ptr<item::Item> m_discarded_item {};
};

}  // namespace actor

#endif // MON_HPP
