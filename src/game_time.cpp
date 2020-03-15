// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "game_time.hpp"

#include <vector>

#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "audio.hpp"
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
#include "terrain.hpp"
#include "terrain_door.hpp"
#include "terrain_mob.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<actor::Speed> s_turn_type_vector;

// Smallest number divisible by both 2 (200% speed) and 3 (300% speed)
static const int s_ticks_per_turn = 6;

static size_t s_current_actor_idx = 0;

static int s_turn_nr = 0;

static int s_std_turn_delay = s_ticks_per_turn;

static int speed_to_pct(const actor::Speed speed)
{
        switch (speed)
        {
        case actor::Speed::slow:
                return 50;

        case actor::Speed::normal:
                return 100;

        case actor::Speed::fast:
                return 200;

        case actor::Speed::very_fast:
                return 300;
        }

        ASSERT(false);

        return 100;
}

static actor::Speed incr_speed_category(actor::Speed speed)
{
        if (speed < actor::Speed::very_fast)
        {
                speed = (actor::Speed)((int)speed + 1);
        }

        return speed;
}

static actor::Speed decr_speed_category(actor::Speed speed)
{
        if ((int)speed > 0)
        {
                speed = (actor::Speed)((int)speed - 1);
        }

        return speed;
}

static actor::Speed current_actor_speed(const actor::Actor& actor)
{
        // Paralyzed actors always act at normal speed (otherwise paralysis will
        // barely affect super fast monsters at all)
        if (actor.m_properties.has(PropId::paralyzed))
        {
                return actor::Speed::normal;
        }

        if (actor.m_properties.has(PropId::clockwork_hasted))
        {
                return actor::Speed::very_fast;
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
                        MsgInterruptPlayer::yes,
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
                auto* const actor = game_time::g_actors[i];

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
                                auto* const mon =
                                        static_cast<actor::Mon*>(actor);

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

        // Allow already burning terrains to damage stuff, spread fire, etc
        for (auto& cell : map::g_cells)
        {
                if (cell.terrain->m_burn_state == BurnState::burning)
                {
                        cell.terrain->m_started_burning_this_turn = false;
                }
        }

        // New turn for terrains
        for (auto& cell : map::g_cells)
        {
                cell.terrain->on_new_turn();
        }

        // New turn for mobs
        const std::vector<terrain::Terrain*> mobs_cpy = game_time::g_mobs;

        for (auto* t : mobs_cpy)
        {
                t->on_new_turn();
        }

        if (map_control::g_controller)
        {
                map_control::g_controller->on_std_turn();
        }

        // Run new turn events on all player items
        for (auto* const item : map::g_player->m_inv.m_backpack)
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

static void run_atomic_turn_events()
{
        // Stop burning for any actor standing in liquid
        for (auto* const actor : game_time::g_actors)
        {
                const P& p = actor->m_pos;

                const auto* const terrain = map::g_cells.at(p).terrain;

                if (terrain->data().matl_type == Matl::fluid)
                {
                        actor->m_properties.end_prop(PropId::burning);
                }
        }

        // NOTE: We add light AFTER ending burning for actors in liquid, since
        // those actors shouldn't add light.
        game_time::update_light_map();
}

static void set_actor_max_delay(actor::Actor& actor)
{
        const auto speed_category = current_actor_speed(actor);

        const int speed_pct = speed_to_pct(speed_category);

        int delay_to_set = (s_ticks_per_turn * 100) / speed_pct;

        // Make sure the delay is at least 1, to never give an actor
        // infinite number of actions
        delay_to_set = std::max(1, delay_to_set);

        actor.m_delay = delay_to_set;
}

// -----------------------------------------------------------------------------
// game_time
// -----------------------------------------------------------------------------
namespace game_time
{

std::vector<actor::Actor*> g_actors;
std::vector<terrain::Terrain*> g_mobs;

bool g_is_magic_descend_nxt_std_turn;


void init()
{
        s_current_actor_idx = 0;
        s_turn_nr = 0;
        s_std_turn_delay = s_ticks_per_turn;

        g_actors.clear();

        g_mobs.clear();

        g_is_magic_descend_nxt_std_turn = false;
}

void cleanup()
{
        for (auto* a : g_actors)
        {
                delete a;
        }

        g_actors.clear();

        for (auto* t : g_mobs)
        {
                delete t;
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

std::vector<terrain::Terrain*> mobs_at_pos(const P& p)
{
        std::vector<terrain::Terrain*> mobs;

        for (auto* m : mobs)
        {
                if (m->pos() == p)
                {
                        mobs.push_back(m);
                }
        }

        return mobs;
}

void add_mob(terrain::Terrain* const t)
{
        g_mobs.push_back(t);

        t->on_placed();
}

void erase_mob(terrain::Terrain* const f, const bool destroy_object)
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

void add_actor(actor::Actor* actor)
{
        // Sanity checks
        // ASSERT(map::is_pos_inside_map(actor->m_pos));

#ifndef NDEBUG
        for (actor::Actor* const existing_actor : g_actors)
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

void reset_current_actor_idx()
{
        s_current_actor_idx = 0;
}

void tick()
{
        auto* actor = current_actor();

        set_actor_max_delay(*actor);

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

                if (actor->m_delay == 0)
                {
                        // Actor is ready to go
                        break;
                }
                else
                {
                        // Actor is still waiting
                        --actor->m_delay;
                }
        }

        run_atomic_turn_events();

        auto* const next_actor = current_actor();

        // Clear flag that this actor is opening a door
        if (map::rect().is_pos_inside(next_actor->m_opening_door_pos))
        {
                auto* const t =
                        map::g_cells.at(next_actor->m_opening_door_pos)
                        .terrain;

                if (t->id() == terrain::Id::door)
                {
                        auto* const door = static_cast<terrain::Door*>(t);

                        if (door->actor_currently_opening() == next_actor)
                        {
                                door->clear_actor_currently_opening();
                        }

                        next_actor->m_opening_door_pos = {-1, -1};
                }
        }

        next_actor->m_properties.on_turn_begin();

        next_actor->on_actor_turn();
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
                map::g_cells.at(i).terrain->add_light(light_tmp);
        }

        // Copy the temporary buffer to the real light map
        memcpy(map::g_light.data(), light_tmp.data(), map::g_light.length());
}

actor::Actor* current_actor()
{
        ASSERT(s_current_actor_idx < g_actors.size());

        auto* const actor = g_actors[s_current_actor_idx];

        ASSERT(actor->m_delay >= 0);

        ASSERT(map::is_pos_inside_map(actor->m_pos));

        return actor;
}

} // namespace game_time
