#include "actor_move.hpp"

#include "actor.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "attack.hpp"
#include "common_text.hpp"
#include "feature_mob.hpp"
#include "feature_rigid.hpp"
#include "game_time.hpp"
#include "item.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "query.hpp"
#include "reload.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static BinaryAnswer query_player_attack_mon_with_ranged_wpn(
        const Wpn& wpn,
        const Mon& mon)
{
        const std::string wpn_name = wpn.name(ItemRefType::a);

        const bool can_see_mon = map::player->can_see_actor(mon);

        const std::string mon_name =
                can_see_mon
                ? mon.name_the()
                : "it";

        msg_log::add(
                std::string(
                        "Attack " +
                        mon_name +
                        " with " +
                        wpn_name +
                        "? " +
                        common_text::yes_or_no_hint),
                colors::light_white());

        const auto answer = query::yes_or_no();

        return answer;
}

static void player_bump_known_hostile_mon(Mon& mon)
{
        auto& player = *map::player;

        if (!player.properties.allow_attack_melee(Verbosity::verbose))
        {
                return;
        }

        Item* const wpn_item = player.inv.item_in_slot(SlotId::wpn);

        if (!wpn_item)
        {
                player.hand_att(mon);

                return;
        }

        auto& wpn = static_cast<Wpn&>(*wpn_item);

        // If this is also a ranged weapon, ask if player really
        // intended to use it as melee weapon
        if (wpn.data().ranged.is_ranged_wpn &&
            config::is_ranged_wpn_meleee_prompt())
        {
                const auto answer =
                        query_player_attack_mon_with_ranged_wpn(wpn, mon);

                if (answer == BinaryAnswer::no)
                {
                        msg_log::clear();

                        return;
                }
        }

        attack::melee(
                &player,
                player.pos,
                mon,
                wpn);

        player.tgt_ = &mon;
}

static void player_bump_unkown_hostile_mon(Mon& mon)
{
        mon.set_player_aware_of_me();

        actor::print_aware_invis_mon_msg(mon);
}

static void player_bump_allied_mon(Mon& mon)
{
        if (mon.player_aware_of_me_counter_ > 0)
        {
                std::string mon_name =
                        map::player->can_see_actor(mon)
                        ? mon.name_a()
                        : "it";

                msg_log::add("I displace " + mon_name + ".");
        }

        mon.pos = map::player->pos;
}

static void player_walk_on_item(Item* const item)
{
        if (!item)
        {
                return;
        }

        // Only print the item name if the item will not be "found" by stepping
        // on it, otherwise there would be redundant messages, e.g. "A Muddy
        // Potion." -> "I have found a Muddy Potion!"
        if ((item->data().xp_on_found <= 0) || item->data().is_found)
        {
                std::string item_name =
                        item->name(
                                ItemRefType::plural,
                                ItemRefInf::yes,
                                ItemRefAttInf::wpn_main_att_mode);

                item_name = text_format::first_to_upper(item_name);

                msg_log::add(item_name + ".");
        }

        item->on_player_found();
}

static void print_corpses_at_player_msgs()
{
        for (auto* const actor : game_time::actors)
        {
                if ((actor->pos == map::player->pos) &&
                    (actor->state == ActorState::corpse))
                {
                        const std::string name =
                                text_format::first_to_upper(
                                        actor->data->corpse_name_a);

                        msg_log::add(name + ".");
                }
        }
}

static bool is_player_staggering_from_wounds()
{
        Prop* const wound_prop = map::player->properties.prop(PropId::wound);

        int nr_wounds = 0;

        if (wound_prop)
        {
                nr_wounds = static_cast<PropWound*>(wound_prop)->nr_wounds();
        }

        const int min_nr_wounds_for_stagger = 3;

        return nr_wounds >= min_nr_wounds_for_stagger;
}

static AllowAction pre_bump_features(Actor& actor, const P& tgt)
{
        const auto mobs = game_time::mobs_at_pos(tgt);

        for (auto* mob : mobs)
        {
                const auto result = mob->pre_bump(actor);

                if (result == AllowAction::no)
                {
                        return result;
                }
        }

        const auto result = map::cells.at(tgt).rigid->pre_bump(actor);

        return result;
}

static void bump_features(Actor& actor, const P& tgt)
{
        const auto mobs = game_time::mobs_at_pos(tgt);

        for (auto* mob : mobs)
        {
                mob->bump(actor);
        }

        map::cells.at(tgt).rigid->bump(actor);
}

static void on_player_waiting()
{
        auto did_action = DidAction::no;

        // Ghoul feed on corpses?
        if (player_bon::bg() == Bg::ghoul)
        {
                did_action = map::player->try_eat_corpse();
        }

        if (did_action == DidAction::no)
        {
                // Reorganize pistol magazines?
                const auto seen_foes = map::player->seen_foes();

                if (seen_foes.empty())
                {
                        reload::player_arrange_pistol_mags();
                }
        }
}

static void move_player_non_center_direction(const P& tgt)
{
        auto& player = *map::player;

        const bool is_features_blocking_move =
                map_parsers::BlocksActor(player, ParseActors::no)
                .cell(tgt);

        auto* const mon = static_cast<Mon*>(map::actor_at_pos(tgt));

        const auto is_aware_of_mon =
                mon &&
                (mon->player_aware_of_me_counter_ > 0);

        if (mon &&
            !player.is_leader_of(mon) &&
            is_aware_of_mon)
        {
                player_bump_known_hostile_mon(*mon);

                return;
        }

        const auto pre_move_result = pre_bump_features(player, tgt);

        if (pre_move_result == AllowAction::no)
        {
                return;
        }

        if (mon &&
            !player.is_leader_of(mon) &&
            !is_features_blocking_move &&
            !is_aware_of_mon)
        {
                player_bump_unkown_hostile_mon(*mon);

                return;
        }

        if (!is_features_blocking_move)
        {
                const int enc = player.enc_percent();

                if (enc >= enc_immobile_lvl)
                {
                        // TODO: Currently you can attempt to attack hidden
                        // adjacent monsters "for free" while you are too
                        // encumbered to move (very minor issue, but it's weird)
                        msg_log::add("I am too encumbered to move!");

                        return;
                }
                else if ((enc >= 100) ||
                         is_player_staggering_from_wounds())
                {
                        // TODO: This probably sounds weird when also swimming
                        // or wading
                        msg_log::add("I stagger.", colors::msg_note());

                        player.properties.apply(new PropWaiting());
                }

                if (mon && player.is_leader_of(mon))
                {
                        player_bump_allied_mon(*mon);
                }

                map::cells.at(player.pos).rigid->on_leave(player);

                player.pos = tgt;

                player_walk_on_item(map::cells.at(player.pos).item);

                print_corpses_at_player_msgs();
        }

        bump_features(player, tgt);
}

static void move_player(Dir dir)
{
        auto& player = *map::player;

        if (!player.is_alive())
        {
                return;
        }

        const auto intended_dir = dir;

        player.properties.affect_move_dir(player.pos, dir);

        const auto tgt = player.pos + dir_utils::offset(dir);

        if (intended_dir == Dir::center)
        {
                on_player_waiting();
        }
        else if (dir != Dir::center)
        {
                move_player_non_center_direction(tgt);
        }

        if (player.pos == tgt)
        {
                // We are at the target position, this means that either:
                // * the player moved to a different position, or
                // * the player waited in the current position on purpose, or
                // * the player was stuck (e.g. in a spider web)

                game_time::tick();
        }
}

static void move_mon(Mon& mon, Dir dir)
{
#ifndef NDEBUG
        if (dir == Dir::END)
        {
                TRACE << "Illegal direction parameter" << std::endl;
                ASSERT(false);
        }

        if (!map::is_pos_inside_outer_walls(mon.pos))
        {
                TRACE << "Monster outside map" << std::endl;
                ASSERT(false);
        }
#endif // NDEBUG

        mon.properties.affect_move_dir(mon.pos, dir);

        // Movement direction is stored for AI purposes
        mon.last_dir_moved_ = dir;

        const P target_p(mon.pos + dir_utils::offset(dir));

        // Sanity check - monsters should never attempt to move into a blocked
        // cell (if they do try this, it's probably an AI bug)
#ifndef NDEBUG
        if (target_p != mon.pos)
        {
                Array2<bool> blocked(map::dims());

                const bool is_blocked =
                        map_parsers::BlocksActor(mon, ParseActors::yes)
                        .cell(target_p);

                ASSERT(!is_blocked);
        }
#endif // NDEBUG

        if ((dir != Dir::center) &&
            map::is_pos_inside_outer_walls(target_p))
        {
                // Leave current cell (e.g. stop swimming)
                map::cells.at(mon.pos).rigid->on_leave(mon);

                mon.pos = target_p;

                // Bump features in target cell (e.g. to trigger traps)
                const auto mobs = game_time::mobs_at_pos(mon.pos);

                for (auto* m : mobs)
                {
                        m->bump(mon);
                }

                map::cells.at(mon.pos).rigid->bump(mon);
        }

        game_time::tick();
}

// -----------------------------------------------------------------------------
// actor
// -----------------------------------------------------------------------------
namespace actor
{

void move(Actor& actor, const Dir dir)
{
        if (actor.is_player())
        {
                move_player(dir);
        }
        else
        {
                move_mon(static_cast<Mon&>(actor), dir);
        }
}

} // actor
