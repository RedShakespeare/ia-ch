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
static std::vector<ActorSpeed> turn_type_vector_;

// Smallest number divisible by both 2 (200% speed) and 3 (300% speed)
static const int ticks_per_turn_ = 6;

static int current_turn_type_pos_ = 0;

static size_t current_actor_idx_ = 0;

static int turn_nr_ = 0;

static int std_turn_delay_ = ticks_per_turn_;

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
        if (actor.properties.has(PropId::paralyzed))
        {
                return ActorSpeed::normal;
        }

        if (actor.properties.has(PropId::clockwork_hasted))
        {
                return ActorSpeed::very_fast;
        }

        auto speed = actor.data->speed;

        if (actor.properties.has(PropId::slowed))
        {
                speed = decr_speed_category(speed);
        }

        if (actor.properties.has(PropId::hasted))
        {
                speed = incr_speed_category(speed);
        }

        if (actor.properties.has(PropId::frenzied))
        {
                speed = incr_speed_category(speed);
        }

        return speed;
}

static void run_std_turn_events()
{
        if (game_time::is_magic_descend_nxt_std_turn)
        {
                const PropEndConfig prop_end_config(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::no);

                map::player->properties.end_prop(
                        PropId::nailed,
                        prop_end_config);

                map::player->properties.end_prop(
                        PropId::entangled,
                        prop_end_config);

                map::player->properties.end_prop(
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

        ++turn_nr_;

        // NOTE: Iteration must be done by index, since new monsters may be
        // spawned inside the loop when the standard turn hook is called (e.g.
        // from the 'breeding' property)
        for (size_t i = 0; i < game_time::actors.size(); /* No increment */)
        {
                Actor* const actor = game_time::actors[i];

                // Delete destroyed actors
                if (actor->state == ActorState::destroyed)
                {
                        if (actor == map::player)
                        {
                                return;
                        }

                        if (map::player->tgt_ == actor)
                        {
                                map::player->tgt_ = nullptr;
                        }

                        delete actor;

                        game_time::actors.erase(game_time::actors.begin() + i);

                        if (current_actor_idx_ >= game_time::actors.size())
                        {
                                current_actor_idx_ = 0;
                        }
                }
                else  // Actor not destroyed
                {
                        if (!actor->is_player())
                        {
                                // Count down monster awareness
                                Mon* const mon = static_cast<Mon*>(actor);

                                if (mon->player_aware_of_me_counter_ > 0)
                                {
                                        --mon->player_aware_of_me_counter_;
                                }
                        }

                        actor->on_std_turn_common();

                        // NOTE: This may spawn new monsters, see NOTE above.
                        actor->properties.on_std_turn();

                        ++i;
                }
        } // Actor loop

        // Allow already burning features to damage stuff, spread fire, etc
        for (auto& cell : map::cells)
        {
                if (cell.rigid->burn_state_ == BurnState::burning)
                {
                        cell.rigid->started_burning_this_turn_ = false;
                }
        }

        // New turn for rigids
        for (auto& cell : map::cells)
        {
                cell.rigid->on_new_turn();
        }

        // New turn for mobs
        const std::vector<Mob*> mobs_cpy = game_time::mobs;

        for (auto* f : mobs_cpy)
        {
                f->on_new_turn();
        }

        if (map_control::controller)
        {
                map_control::controller->on_std_turn();
        }

        // Run new turn events on all player items
        for (Item* const item : map::player->inv.backpack)
        {
                item->on_std_turn_in_inv(InvType::backpack);
        }

        for (InvSlot& slot : map::player->inv.slots)
        {
                if (slot.item)
                {
                        slot.item->on_std_turn_in_inv(InvType::slots);
                }
        }

        snd_emit::reset_nr_snd_msg_printed_current_turn();

        if ((map::dlvl > 0) && !map::player->properties.has(PropId::deaf))
        {
                const int play_one_in_n = 200;

                audio::try_play_amb(play_one_in_n);
        }
}

static  void run_atomic_turn_events()
{
        // Stop burning for any actor standing in liquid
        for (auto* const actor : game_time::actors)
        {
                const P& p = actor->pos;

                const Rigid* const rigid = map::cells.at(p).rigid;

                if (rigid->data().matl_type == Matl::fluid)
                {
                        actor->properties.end_prop(PropId::burning);
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

std::vector<Actor*> actors;
std::vector<Mob*>   mobs;

bool is_magic_descend_nxt_std_turn;

void init()
{
        current_turn_type_pos_ = 0;
        current_actor_idx_ = 0;
        turn_nr_ = 0;
        std_turn_delay_ = ticks_per_turn_;

        actors.clear();
        mobs  .clear();

        is_magic_descend_nxt_std_turn = false;
}

void cleanup()
{
        for (Actor* a : actors)
        {
                delete a;
        }

        actors.clear();

        for (auto* f : mobs)
        {
                delete f;
        }

        mobs.clear();

        is_magic_descend_nxt_std_turn = false;
}

void save()
{
        saving::put_int(turn_nr_);
}

void load()
{
        turn_nr_ = saving::get_int();
}

int turn_nr()
{
        return turn_nr_;
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
        mobs.push_back(f);
}

void erase_mob(Mob* const f, const bool destroy_object)
{
        for (auto it = mobs.begin(); it != mobs.end(); ++it)
        {
                if (*it == f)
                {
                        if (destroy_object)
                        {
                                delete f;
                        }

                        mobs.erase(it);

                        return;
                }
        }

        ASSERT(false);
}

void erase_all_mobs()
{
        for (auto* m : mobs)
        {
                delete m;
        }

        mobs.clear();
}

void add_actor(Actor* actor)
{
        // Sanity checks
        // ASSERT(map::is_pos_inside_map(actor->pos));

#ifndef NDEBUG
        for (Actor* const existing_actor : actors)
        {
                ASSERT(actor != existing_actor);

                if (actor->is_alive() && existing_actor->is_alive())
                {
                        const P& new_actor_p = actor->pos;
                        const P& existing_actor_p = existing_actor->pos;

                        ASSERT(new_actor_p != existing_actor_p);
                }
        }
#endif // NDEBUG

        actors.push_back(actor);
}

void reset_turn_type_and_actor_counters()
{
        current_turn_type_pos_ = current_actor_idx_ = 0;
}

void tick()
{
        auto* actor = current_actor();

        {
                const auto speed_category = current_actor_speed(*actor);

                const int speed_pct = speed_to_pct(speed_category);

                int delay_to_set = (ticks_per_turn_ * 100) / speed_pct;

                // Make sure the delay is at least 1, to never give an actor
                // infinite number of actions
                delay_to_set = std::max(1, delay_to_set);

                actor->delay = delay_to_set;
        }

        actor->properties.on_turn_end();

        // Find next actor who can act
        while (true)
        {
                if (actors.empty())
                {
                        return;
                }

                ++current_actor_idx_;

                if (current_actor_idx_ == actors.size())
                {
                        // New standard turn?
                        if (std_turn_delay_ == 0)
                        {
                                // Increment the turn counter, and run standard
                                // turn events

                                // NOTE: This will prune destroyed actors, which
                                // will decrease the actor vector size.
                                run_std_turn_events();

                                std_turn_delay_ = ticks_per_turn_;
                        }
                        else
                        {
                                --std_turn_delay_;
                        }

                        current_actor_idx_ = 0;
                }

                actor = current_actor();

                ASSERT(actor->delay >= 0);

                if (actor->delay == 0)
                {
                        // Actor is ready to go
                        break;
                }

                // Actor is still waiting
                --actor->delay;
        }

        run_atomic_turn_events();

        current_actor()->properties.on_turn_begin();

        current_actor()->on_actor_turn();
}

void update_light_map()
{
        Array2<bool> light_tmp(map::dims());

        for (const auto* const a : actors)
        {
                a->add_light(light_tmp);
        }

        for (const auto* const m : mobs)
        {
                m->add_light(light_tmp);
        }

        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                map::cells.at(i).rigid->add_light(light_tmp);
        }

        // Copy the temporary buffer to the real light map
        memcpy(map::light.data(), light_tmp.data(), map::light.length());
}

Actor* current_actor()
{
        ASSERT(current_actor_idx_ < actors.size());

        Actor* const actor = actors[current_actor_idx_];

        ASSERT(map::is_pos_inside_map(actor->pos));

        return actor;
}

} // game_time
