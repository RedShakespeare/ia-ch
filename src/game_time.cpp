// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "game_time.hpp"

#include <vector>

#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "audio.hpp"
#include "feature_mob.hpp"
#include "feature_rigid.hpp"
#include "init.hpp"
#include "inventory.hpp"
#include "inventory_handling.hpp"
#include "io.hpp"
#include "item.hpp"
#include "map.hpp"
#include "map_controller.hpp"
#include "map_parsing.hpp"
#include "map_travel.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "saving.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<ActorSpeed> s_turn_type_vector;

// Smallest number divisible by both 2 (200% speed) and 3 (300% speed)
static const int s_ticks_per_turn = 6;

static int s_current_turn_type_pos = 0;

static size_t s_current_actor_idx = 0;

static int s_turn_nr = 0;

static int s_std_turn_delay = s_ticks_per_turn;

static int speed_to_pct(const ActorSpeed speed)
{
        switch (speed)
        {
        case ActorSpeed::slow:
                return 50;

        case ActorSpeed::normal:
                return 100;

        case ActorSpeed::fast:
                return 200;

        case ActorSpeed::very_fast:
                return 300;
        }

        ASSERT(false);

        return 100;
}

static ActorSpeed incr_speed_category(ActorSpeed speed)
{
        if (speed < ActorSpeed::very_fast)
        {
                speed = (ActorSpeed)((int)speed + 1);
        }

        return speed;
}

static ActorSpeed decr_speed_category(ActorSpeed speed)
{
        if ((int)speed > 0)
        {
                speed = (ActorSpeed)((int)speed - 1);
        }

        return speed;
}

static ActorSpeed current_actor_speed(const Actor& actor)
{
        // Paralyzed actors always act at normal speed (otherwise paralysis will
        // barely affect super fast monsters at all)
        if (actor.m_properties.has(PropId::paralyzed))
        {
                return ActorSpeed::normal;
        }

        if (actor.m_properties.has(PropId::clockwork_hasted))
        {
                return ActorSpeed::very_fast;
        }

        auto speed = actor.m_data->speed;

        if (actor.m_properties.has(PropId::slowed))
        {
                speed = decr_speed_category(speed);
        }

        if (actor.m_properties.has(PropId::hasted))
        {
                speed = incr_speed_category(speed);
        }

        if (actor.m_properties.has(PropId::frenzied))
        {
                speed = incr_speed_category(speed);
        }

        return speed;
}

static void run_std_turn_events()
{
        if (game_time::g_is_magic_descend_nxt_std_turn)
        {
                const PropEndConfig prop_end_config(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::no);

                map::g_player->m_properties.end_prop(
                        PropId::nailed,
                        prop_end_config);

                map::g_player->m_properties.end_prop(
                        PropId::entangled,
                        prop_end_config);

                map::g_player->m_properties.end_prop(
                        PropId::swimming,
                        prop_end_config);

                msg_log::add(
                        "I sink downwards!",
                        colors::white(),
                        false,
                        MorePromptOnMsg::yes);

                map_travel::go_to_nxt();

                return;
        }

        ++s_turn_nr;

        // NOTE: Iteration must be done by index, since new monsters may be
        // spawned inside the loop when the standard turn hook is called (e.g.
        // from the 'breeding' property)
        for (size_t i = 0; i < game_time::g_actors.size(); /* No increment */)
        {
                Actor* const actor = game_time::g_actors[i];

                // Delete destroyed actors
                if (actor->m_state == ActorState::destroyed)
                {
                        if (actor == map::g_player)
                        {
                                return;
                        }

                        if (map::g_player->m_tgt == actor)
                        {
                                map::g_player->m_tgt = nullptr;
                        }

                        delete actor;

                        game_time::g_actors.erase(
                                game_time::g_actors.begin() + i);

                        if (s_current_actor_idx >= game_time::g_actors.size())
                        {
                                s_current_actor_idx = 0;
                        }
                }
                else  // Actor not destroyed
                {
                        if (!actor->is_player())
                        {
                                // Count down monster awareness
                                Mon* const mon = static_cast<Mon*>(actor);

                                if (mon->m_player_aware_of_me_counter > 0)
                                {
                                        --mon->m_player_aware_of_me_counter;
                                }
                        }

                        actor->on_std_turn_common();

                        // NOTE: This may spawn new monsters, see NOTE above.
                        actor->m_properties.on_std_turn();

                        ++i;
                }
        } // Actor loop

        // Allow already burning features to damage stuff, spread fire, etc
        for (auto& cell : map::g_cells)
        {
                if (cell.rigid->m_burn_state == BurnState::burning)
                {
                        cell.rigid->m_started_burning_this_turn = false;
                }
        }

        // New turn for rigids
        for (auto& cell : map::g_cells)
        {
                cell.rigid->on_new_turn();
        }

        // New turn for mobs
        const std::vector<Mob*> mobs_cpy = game_time::g_mobs;

        for (auto* f : mobs_cpy)
        {
                f->on_new_turn();
        }

        if (map_control::g_controller)
        {
                map_control::g_controller->on_std_turn();
        }

        // Run new turn events on all player items
        for (Item* const item : map::g_player->m_inv.m_backpack)
        {
                item->on_std_turn_in_inv(InvType::backpack);
        }

        for (InvSlot& slot : map::g_player->m_inv.m_slots)
        {
                if (slot.item)
                {
                        slot.item->on_std_turn_in_inv(InvType::slots);
                }
        }

        snd_emit::reset_nr_snd_msg_printed_current_turn();

        if ((map::g_dlvl > 0) && !map::g_player->m_properties.has(PropId::deaf))
        {
                const int play_one_in_n = 200;

                audio::try_play_amb(play_one_in_n);
        }
}

static  void run_atomic_turn_events()
{
        // Stop burning for any actor standing in liquid
        for (auto* const actor : game_time::g_actors)
        {
                const P& p = actor->m_pos;

                const Rigid* const rigid = map::g_cells.at(p).rigid;

                if (rigid->data().matl_type == Matl::fluid)
                {
                        actor->m_properties.end_prop(PropId::burning);
                }
        }

        // NOTE: We add light AFTER ending burning for actors in liquid, since
        // those actors shouldn't add light.
        game_time::update_light_map();
}

// -----------------------------------------------------------------------------
// game_time
// -----------------------------------------------------------------------------
namespace game_time
{

std::vector<Actor*> g_actors;
std::vector<Mob*> g_mobs;

bool g_is_magic_descend_nxt_std_turn;


void init()
{
        s_current_turn_type_pos = 0;
        s_current_actor_idx = 0;
        s_turn_nr = 0;
        s_std_turn_delay = s_ticks_per_turn;

        g_actors.clear();

        g_mobs.clear();

        g_is_magic_descend_nxt_std_turn = false;
}

void cleanup()
{
        for (Actor* a : g_actors)
        {
                delete a;
        }

        g_actors.clear();

        for (auto* f : g_mobs)
        {
                delete f;
        }

        g_mobs.clear();

        g_is_magic_descend_nxt_std_turn = false;
}

void save()
{
        saving::put_int(s_turn_nr);
}

void load()
{
        s_turn_nr = saving::get_int();
}

int turn_nr()
{
        return s_turn_nr;
}

std::vector<Mob*> mobs_at_pos(const P& p)
{
        std::vector<Mob*> mobs;

        for (auto* m : mobs)
        {
                if (m->pos() == p)
                {
                        mobs.push_back(m);
                }
        }

        return mobs;
}

void add_mob(Mob* const f)
{
        g_mobs.push_back(f);
}

void erase_mob(Mob* const f, const bool destroy_object)
{
        for (auto it = g_mobs.begin(); it != g_mobs.end(); ++it)
        {
                if (*it == f)
                {
                        if (destroy_object)
                        {
                                delete f;
                        }

                        g_mobs.erase(it);

                        return;
                }
        }

        ASSERT(false);
}

void erase_all_mobs()
{
        for (auto* m : g_mobs)
        {
                delete m;
        }

        g_mobs.clear();
}

void add_actor(Actor* actor)
{
        // Sanity checks
        // ASSERT(map::is_pos_inside_map(actor->m_pos));

#ifndef NDEBUG
        for (Actor* const existing_actor : g_actors)
        {
                ASSERT(actor != existing_actor);

                if (actor->is_alive() && existing_actor->is_alive())
                {
                        const P& new_actor_p = actor->m_pos;
                        const P& existing_actor_p = existing_actor->m_pos;

                        ASSERT(new_actor_p != existing_actor_p);
                }
        }
#endif // NDEBUG

        g_actors.push_back(actor);
}

void reset_turn_type_and_actor_counters()
{
        s_current_turn_type_pos = s_current_actor_idx = 0;
}

void tick()
{
        auto* actor = current_actor();

        {
                const auto speed_category = current_actor_speed(*actor);

                const int speed_pct = speed_to_pct(speed_category);

                int delay_to_set = (s_ticks_per_turn * 100) / speed_pct;

                // Make sure the delay is at least 1, to never give an actor
                // infinite number of actions
                delay_to_set = std::max(1, delay_to_set);

                actor->m_delay = delay_to_set;
        }

        actor->m_properties.on_turn_end();

        // Find next actor who can act
        while (true)
        {
                if (g_actors.empty())
                {
                        return;
                }

                ++s_current_actor_idx;

                if (s_current_actor_idx == g_actors.size())
                {
                        // New standard turn?
                        if (s_std_turn_delay == 0)
                        {
                                // Increment the turn counter, and run standard
                                // turn events

                                // NOTE: This will prune destroyed actors, which
                                // will decrease the actor vector size.
                                run_std_turn_events();

                                s_std_turn_delay = s_ticks_per_turn;
                        }
                        else
                        {
                                --s_std_turn_delay;
                        }

                        s_current_actor_idx = 0;
                }

                actor = current_actor();

                ASSERT(actor->m_delay >= 0);

                if (actor->m_delay == 0)
                {
                        // Actor is ready to go
                        break;
                }

                // Actor is still waiting
                --actor->m_delay;
        }

        run_atomic_turn_events();

        current_actor()->m_properties.on_turn_begin();

        current_actor()->on_actor_turn();
}

void update_light_map()
{
        Array2<bool> light_tmp(map::dims());

        for (const auto* const a : g_actors)
        {
                a->add_light(light_tmp);
        }

        for (const auto* const m : g_mobs)
        {
                m->add_light(light_tmp);
        }

        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                map::g_cells.at(i).rigid->add_light(light_tmp);
        }

        // Copy the temporary buffer to the real light map
        memcpy(map::g_light.data(), light_tmp.data(), map::g_light.length());
}

Actor* current_actor()
{
        ASSERT(s_current_actor_idx < g_actors.size());

        Actor* const actor = g_actors[s_current_actor_idx];

        ASSERT(map::is_pos_inside_map(actor->m_pos));

        return actor;
}

} // game_time
