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
#include "feature_rigid.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_travel.hpp"
#include "msg_log.hpp"
#include "popup.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static bool try_use_talisman_of_resurrection(Actor& actor)
{
        // Player has the Talisman of Resurrection, and died of physical damage?
        if (!actor.is_player() ||
            !actor.inv.has_item_in_backpack(ItemId::resurrect_talisman) ||
            (map::player->ins() >= 100) ||
            (actor.sp <= 0))
        {
                return false;
        }

        actor.inv.decr_item_type_in_backpack(ItemId::resurrect_talisman);

        msg_log::more_prompt();

        io::clear_screen();

        io::update_screen();

        const std::string msg =
                "Strange emptiness surrounds me. An eternity passes as  I lay "
                "frozen in a world of shadows. Suddenly I awake!";

        popup::msg(msg, "Dead");

        actor.restore_hp(
                999,
                false,
                Verbosity::silent);

        actor.properties.end_prop(
                PropId::wound,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::no)
                );

        // If player died due to falling down a chasm, go to next level
        if (map::cells.at(actor.pos).rigid->id() == FeatureId::chasm)
        {
                map_travel::go_to_nxt();
        }

        msg_log::add("I LIVE AGAIN!");

        game::add_history_event("I was brought back from the dead.");

        map::player->incr_shock(
                ShockLvl::mind_shattering,
                ShockSrc::misc);

        return true;
}

static void move_actor_to_pos_can_have_corpse(Actor& actor)
{
        if (map::cells.at(actor.pos).rigid->can_have_corpse())
        {
                return;
        }

        for (const P& d : dir_utils::dir_list_w_center)
        {
                const P p = actor.pos + d;

                if (map::cells.at(p).rigid->can_have_corpse())
                {
                        actor.pos = p;

                        break;
                }
        }
}

static void print_mon_death_msg(Actor& actor)
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
        Actor& actor,
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

        ASSERT(actor.data->can_leave_corpse ||
               (is_destroyed == IsDestroyed::yes));

        unset_actor_as_leader_for_all_mon(actor);

        if (!actor.is_player())
        {
                if (map::player->tgt_ == &actor)
                {
                        map::player->tgt_ = nullptr;
                }

                if (map::player->can_see_actor(actor))
                {
                        print_mon_death_msg(actor);
                }
        }

        if (is_destroyed == IsDestroyed::yes)
        {
                actor.destroy();
        }
        else // Not destroyed
        {
                actor.state = ActorState::corpse;
        }

        if (!actor.is_player())
        {
                // This is a monster

                if (actor.data->is_humanoid)
                {
                        TRACE_VERBOSE << "Emitting death sound" << std::endl;

                        Snd snd("I hear agonized screaming.",
                                SfxId::END,
                                IgnoreMsgIfOriginSeen::yes,
                                actor.pos,
                                &actor,
                                SndVol::high,
                                AlertsMon::no);

                        snd.run();
                }
        }

        if (is_destroyed == IsDestroyed::yes)
        {
                if (actor.data->can_bleed && (allow_gore == AllowGore::yes))
                {
                        map::make_gore(actor.pos);
                        map::make_blood(actor.pos);
                }
        }
        else // Not destroyed
        {
                if (!actor.is_player())
                {
                        move_actor_to_pos_can_have_corpse(actor);
                }
        }

        actor.on_death();

        actor.properties.on_death();

        if (!actor.is_player())
        {
                if (allow_drop_items == AllowDropItems::yes)
                {
                        actor.inv.drop_all_non_intrinsic(actor.pos);
                }

                game::on_mon_killed(actor);

                static_cast<Mon&>(actor).leader_ = nullptr;
        }

        TRACE_FUNC_END_VERBOSE;
}

void unset_actor_as_leader_for_all_mon(Actor& actor)
{
        for (Actor* other : game_time::actors)
        {
                if ((other != &actor) &&
                    !other->is_player() &&
                    actor.is_leader_of(other))
                {
                        static_cast<Mon*>(other)->leader_ = nullptr;
                }
        }
}

} // actor
