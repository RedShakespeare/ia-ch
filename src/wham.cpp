#include "wham.hpp"

#include "actor_hit.hpp"
#include "actor_player.hpp"
#include "common_text.hpp"
#include "feature_door.hpp"
#include "feature_rigid.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "io.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "query.hpp"
#include "text_format.hpp"

namespace wham
{

void try_sprain_player()
{
        const bool is_frenzied =  map::player->properties.has(PropId::frenzied);

        const bool is_player_ghoul = player_bon::bg() == Bg::ghoul;

        if (is_player_ghoul || is_frenzied)
        {
                return;
        }

        int sprain_one_in_n;

        if (player_bon::has_trait(Trait::rugged))
        {
                sprain_one_in_n = 12;
        }
        else if (player_bon::has_trait(Trait::tough))
        {
                sprain_one_in_n = 8;
        }
        else
        {
                sprain_one_in_n = 4;
        }

        if (rnd::one_in(sprain_one_in_n))
        {
                msg_log::add("I sprain myself.", colors::msg_bad());

                const int dmg = rnd::range(1, 2);

                actor::hit(*map::player, dmg, DmgType::pure);
        }
}

void run()
{
        TRACE_FUNC_BEGIN;

        msg_log::clear();

        // Choose direction
        msg_log::add(
                "Which direction? " + common_text::cancel_hint,
                colors::light_white());

        const Dir input_dir = query::dir(AllowCenter::yes);

        msg_log::clear();

        if (input_dir == Dir::END)
        {
                // Invalid direction
                io::update_screen();

                TRACE_FUNC_END;

                return;
        }

        // The chosen direction is valid

        P att_pos(map::player->pos + dir_utils::offset(input_dir));

        // Kick living actor?
        TRACE << "Checking if player is kicking a living actor" << std::endl;

        if (input_dir != Dir::center)
        {
                Actor* living_actor =
                        map::actor_at_pos(att_pos, ActorState::alive);

                if (living_actor)
                {
                        TRACE << "Actor found at kick pos" << std::endl;

                        const bool melee_allowed =
                                map::player->properties.allow_attack_melee(
                                        Verbosity::verbose);

                        if (melee_allowed)
                        {
                                TRACE << "Player is allowed to do melee attack"
                                      << std::endl;

                                Array2<bool> blocked(map::dims());

                                map_parsers::BlocksLos()
                                        .run(blocked, blocked.rect());

                                TRACE << "Player can see actor" << std::endl;

                                map::player->kick_mon(*living_actor);

                                try_sprain_player();

                                // Attacking ends cloaking
                                map::player->properties.end_prop(
                                        PropId::cloaked);

                                game_time::tick();
                        }

                        TRACE_FUNC_END;
                        return;
                }
        }

        // Destroy corpse?
        TRACE << "Checking if player is destroying a corpse" << std::endl;
        Actor* corpse = nullptr;

        // Check all corpses here, stop at any corpse which is prioritized for
        // bashing (Zombies)
        for (Actor* const actor : game_time::actors)
        {
                if ((actor->pos == att_pos) &&
                    (actor->state == ActorState::corpse))
                {
                        corpse = actor;

                        if (actor->data->prio_corpse_bash)
                        {
                                break;
                        }
                }
        }

        const auto kick_wpn =
                std::unique_ptr<Wpn>(
                        static_cast<Wpn*>(
                                item_factory::make(ItemId::player_kick)));

        const auto* wpn = map::player->inv.item_in_slot(SlotId::wpn);

        if (!wpn)
        {
                wpn = &map::player->unarmed_wpn();
        }

        ASSERT(wpn);

        if (corpse)
        {
                const bool is_seeing_cell =
                        map::cells.at(att_pos).is_seen_by_player;

                std::string corpse_name =
                        is_seeing_cell
                        ? corpse->data->corpse_name_the
                        : "a corpse";

                corpse_name = text_format::first_to_lower(corpse_name);

                // Decide if we should kick or use wielded weapon
                const auto* const wpn_used_att_corpse =
                        wpn->data().melee.att_corpse
                        ? wpn
                        : kick_wpn.get();

                const std::string melee_att_msg =
                        wpn_used_att_corpse->data().melee.att_msgs.player;

                const std::string msg =
                        "I " +
                        melee_att_msg + " "  +
                        corpse_name + ".";

                msg_log::add(msg);

                const Dice dmg_dice =
                        wpn_used_att_corpse->melee_dmg(map::player);

                const int dmg = dmg_dice.roll();

                actor::hit(
                        *corpse,
                        dmg,
                        DmgType::physical,
                        wpn_used_att_corpse->data().melee.dmg_method);

                if (wpn_used_att_corpse == kick_wpn.get())
                {
                        try_sprain_player();
                }

                if (corpse->state == ActorState::destroyed)
                {
                        std::vector<Actor*> corpses_here;

                        for (auto* const actor : game_time::actors)
                        {
                                if ((actor->pos == att_pos) &&
                                    (actor->state == ActorState::corpse))
                                {
                                        corpses_here.push_back(actor);
                                }
                        }

                        if (!corpses_here.empty())
                        {
                                msg_log::more_prompt();
                        }

                        for (auto* const corpse : corpses_here)
                        {
                                const std::string name =
                                        text_format::first_to_upper(
                                                corpse->data->corpse_name_a);

                                msg_log::add(name + ".");
                        }
                }

                // Attacking ends cloaking
                map::player->properties.end_prop(PropId::cloaked);

                game_time::tick();

                TRACE_FUNC_END;

                return;
        }

        // Attack feature
        TRACE << "Checking if player is kicking a feature" << std::endl;

        if (input_dir != Dir::center)
        {
                // Decide if we should kick or use wielded weapon
                auto* const feature = map::cells.at(att_pos).rigid;

                bool allow_wpn_att_rigid = wpn->data().melee.att_rigid;

                const auto wpn_dmg_method = wpn->data().melee.dmg_method;

                if (allow_wpn_att_rigid)
                {
                        switch (feature->id())
                        {
                        case FeatureId::door:
                        {
                                const auto* const door =
                                        static_cast<const Door*>(feature);

                                const auto door_type = door->type();

                                if (door_type == DoorType::gate)
                                {
                                        // Only allow blunt weapons for gates
                                        // (feels weird to attack a barred gate
                                        // with an axe...)
                                        allow_wpn_att_rigid =
                                                (wpn_dmg_method ==
                                                 DmgMethod::blunt);
                                }
                                else // Not gate (i.e. wooden, metal)
                                {
                                        allow_wpn_att_rigid = true;
                                }
                        }
                        break;

                        case FeatureId::wall:
                        {
                                allow_wpn_att_rigid = true;
                        }
                        break;

                        default:
                        {
                                allow_wpn_att_rigid = false;
                        }
                        break;
                        }
                }

                const auto* const wpn_used_att_feature =
                        allow_wpn_att_rigid
                        ? wpn
                        : kick_wpn.get();

                const Dice dmg_dice =
                        wpn_used_att_feature->melee_dmg(map::player);

                const int dmg = dmg_dice.roll();

                feature->hit(
                        dmg,
                        DmgType::physical,
                        wpn_used_att_feature->data().melee.dmg_method,
                        map::player);

                // Attacking ends cloaking
                map::player->properties.end_prop(PropId::cloaked);

                game_time::tick();
        }

        TRACE_FUNC_END;
}

} // wham
