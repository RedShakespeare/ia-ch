// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ACTOR_MON_HPP
#define ACTOR_MON_HPP

#include <memory>
#include <string>
#include <vector>

#include "actor.hpp"
#include "global.hpp"
#include "item.hpp"
#include "sound.hpp"
#include "spells.hpp"

struct AiAttData {
        item::Wpn* wpn = nullptr;
        bool is_melee = false;
};

struct AiAvailAttacksData {
        std::vector<item::Wpn*> weapons = {};
        bool should_reload = false;
        bool is_melee = false;
};

struct MonSpell {
        MonSpell() :
                spell(nullptr),
                skill((SpellSkill)0),
                cooldown(-1) {}

        Spell* spell;
        SpellSkill skill;
        int cooldown;
};

namespace actor {

std::string get_cultist_phrase();

std::string get_cultist_aware_msg_seen(const Actor& actor);

std::string get_cultist_aware_msg_hidden();

class Mon : public Actor {
public:
        Mon();
        virtual ~Mon();

        virtual DidAction on_act()
        {
                return DidAction::no;
        }

        bool can_see_actor(
                const Actor& other,
                const Array2<bool>& hard_blocked_los) const;

        std::vector<Actor*> seen_actors() const override;

        std::vector<Actor*> seen_foes() const override;

        // Actors which are possible to see (i.e. not impossible due to
        // invisibility, etc), but may or may not currently be seen due to
        // (lack of) awareness
        std::vector<Actor*> seeable_foes() const;

        bool is_sneaking() const;

        Color color() const override;

        SpellSkill spell_skill(SpellId id) const override;

        AiAvailAttacksData avail_attacks(Actor& defender) const;

        AiAttData choose_attack(const AiAvailAttacksData& avail_attacks) const;

        DidAction try_attack(Actor& defender);

        void hear_sound(const Snd& snd);

        void become_aware_player(bool is_from_seeing, int factor = 1);

        void become_wary_player();

        void set_player_aware_of_me(int duration_factor = 1);

        void make_leader_aware_silent() const;

        std::vector<Actor*> unseen_foes_aware_of() const;

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

        void speak_phrase(AlertsMon alerts_others);

        bool is_leader_of(const Actor* actor) const override;
        bool is_actor_my_leader(const Actor* actor) const override;

        void add_spell(SpellSkill skill, Spell* spell);

        int m_wary_of_player_counter;
        int m_aware_of_player_counter;
        int m_player_aware_of_me_counter;
        bool m_is_msg_mon_in_view_printed;
        bool m_is_player_feeling_msg_allowed;
        Dir m_last_dir_moved;
        MonRoamingAllowed m_is_roaming_allowed;
        Actor* m_leader;
        Actor* m_target;
        bool m_is_target_seen;
        bool m_waiting;

        std::vector<MonSpell> m_spells;

protected:
        // Return value 'true' means it is possible to see the other actor (i.e.
        // it's not impossible due to invisibility, etc), but the actor may or
        // may not currently be seen due to (lack of) awareness
        bool is_actor_seeable(
                const Actor& other,
                const Array2<bool>& hard_blocked_los) const;

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
                DmgType dmg_type,
                DmgMethod method,
                AllowWound allow_wound) override;

        int nr_mon_in_group() const;
};

class Ape : public Mon {
public:
        Ape() :

                m_frenzy_cooldown(0)
        {}

        ~Ape() = default;

private:
        DidAction on_act() override;

        int m_frenzy_cooldown;
};

class Khephren : public Mon {
public:
        Khephren() :

                m_has_summoned_locusts(false)
        {}
        ~Khephren() = default;

private:
        DidAction on_act() override;

        bool m_has_summoned_locusts;
};

class StrangeColor : public Mon {
public:
        StrangeColor() = default;

        ~StrangeColor() = default;

        Color color() const override;
};

class SpectralWpn : public Mon {
public:
        SpectralWpn();

        ~SpectralWpn() = default;

        void on_death() override;

        std::string name_the() const override;

        std::string name_a() const override;

        char character() const override;

        TileId tile() const override;

        std::string descr() const override;

private:
        std::unique_ptr<item::Item> m_discarded_item {};
};

} // namespace actor

#endif // ACTOR_MON_HPP
