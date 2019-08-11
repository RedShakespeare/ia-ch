// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "actor_death.hpp"

#include "actor.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "drop.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "io.hpp"
#include "item_data.hpp"
#include "map.hpp"
#include "map_travel.hpp"
#include "msg_log.hpp"
#include "popup.hpp"
#include "terrain.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static bool try_use_talisman_of_resurrection(actor::Actor& actor)
{
        // Player has the Talisman of Resurrection, and died of physical damage?
        if (!actor.is_player() ||
            !actor.m_inv.has_item_in_backpack(item::Id::resurrect_talisman) ||
            (map::g_player->ins() >= 100) ||
            (actor.m_sp <= 0))
        {
                return false;
        }

        actor.m_inv.decr_item_type_in_backpack(
                item::Id::resurrect_talisman);

        msg_log::more_prompt();

        io::clear_screen();

        io::update_screen();

        const std::string msg =
                "Strange emptiness surrounds me. An eternity passes as I lay "
                "frozen in a world of shadows. Suddenly I awake!";

        popup::msg(msg, "Dead");

        for (auto* const a : game_time::g_actors)
        {
                if (!a->is_player())
                {
                        auto* const mon = static_cast<actor::Mon*>(a);

                        mon->m_aware_of_player_counter = 0;
                        mon->m_wary_of_player_counter = 0;
                }
        }

        actor.restore_hp(
                999,
                false,
                Verbosity::silent);

        actor.m_properties.end_prop(
                PropId::wound,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::no)
                );

        // If player died due to falling down a chasm, go to next level
        if (map::g_cells.at(actor.m_pos).terrain->id() == terrain::Id::chasm)
        {
                map_travel::go_to_nxt();
        }

        msg_log::add("I LIVE AGAIN!");

        game::add_history_event("I was brought back from the dead.");

        map::g_player->incr_shock(
                ShockLvl::mind_shattering,
                ShockSrc::misc);

        return true;
}

static void move_actor_to_pos_can_have_corpse(actor::Actor& actor)
{
        if (map::g_cells.at(actor.m_pos).terrain->can_have_corpse())
        {
                return;
        }

        for (const P& d : dir_utils::g_dir_list_w_center)
        {
                const P p = actor.m_pos + d;

                if (map::g_cells.at(p).terrain->can_have_corpse())
                {
                        actor.m_pos = p;

                        break;
                }
        }
}

static void print_mon_death_msg(actor::Actor& actor)
{
        TRACE_VERBOSE << "Printing death message" << std::endl;

        const std::string msg = actor.death_msg();

        if (!msg.empty())
        {
                msg_log::add(msg);
        }
}

// -----------------------------------------------------------------------------
// actor
// -----------------------------------------------------------------------------
namespace actor
{

// TODO: Split into smaller functions
void kill(
        actor::Actor& actor,
        const IsDestroyed is_destroyed,
        const AllowGore allow_gore,
        const AllowDropItems allow_drop_items)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        // Save player with Talisman of Resurrection?
        const bool did_use_talisman = try_use_talisman_of_resurrection(actor);

        if (did_use_talisman)
        {
                return;
        }

        ASSERT(actor.m_data->can_leave_corpse ||
               (is_destroyed == IsDestroyed::yes));

        unset_actor_as_leader_for_all_mon(actor);

        if (!actor.is_player())
        {
                if (map::g_player->m_tgt == &actor)
                {
                        map::g_player->m_tgt = nullptr;
                }

                if (map::g_player->can_see_actor(actor))
                {
                        print_mon_death_msg(actor);
                }
        }

        if (!actor.is_player() && actor.m_data->is_humanoid)
        {
                TRACE_VERBOSE << "Emitting death sound" << std::endl;

                Snd snd("I hear agonized screaming.",
                        SfxId::END,
                        IgnoreMsgIfOriginSeen::yes,
                        actor.m_pos,
                        &actor,
                        SndVol::high,
                        AlertsMon::no);

                snd.run();
        }

        if (is_destroyed == IsDestroyed::yes)
        {
                actor.m_state = ActorState::destroyed;

                actor.m_properties.on_destroyed_alive();

                if (actor.m_data->can_bleed && (allow_gore == AllowGore::yes))
                {
                        map::make_gore(actor.m_pos);
                        map::make_blood(actor.m_pos);
                }
        }
        else // Not destroyed
        {
                actor.m_state = ActorState::corpse;

                if (!actor.is_player())
                {
                        move_actor_to_pos_can_have_corpse(actor);
                }
        }

        actor.on_death();

        actor.m_properties.on_death();

        if (!actor.is_player())
        {
                if (allow_drop_items == AllowDropItems::yes)
                {
                        actor.m_inv.drop_all_non_intrinsic(actor.m_pos);
                }

                game::on_mon_killed(actor);

                static_cast<Mon&>(actor).m_leader = nullptr;
        }

        TRACE_FUNC_END_VERBOSE;
}

void unset_actor_as_leader_for_all_mon(actor::Actor& actor)
{
        for (Actor* other : game_time::g_actors)
        {
                if ((other != &actor) &&
                    !other->is_player() &&
                    actor.is_leader_of(other))
                {
                        static_cast<Mon*>(other)->m_leader = nullptr;
                }
        }
}

} // actor
