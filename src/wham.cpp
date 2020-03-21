// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "wham.hpp"

#include "actor_hit.hpp"
#include "actor_player.hpp"
#include "common_text.hpp"
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
#include "terrain.hpp"
#include "terrain_door.hpp"
#include "text_format.hpp"

namespace wham {

void try_sprain_player()
{
        const bool is_frenzied =
                map::g_player->m_properties.has(PropId::frenzied);

        const bool is_player_ghoul = player_bon::bg() == Bg::ghoul;

        if (is_player_ghoul || is_frenzied) {
                return;
        }

        int sprain_one_in_n;

        if (player_bon::has_trait(Trait::rugged)) {
                sprain_one_in_n = 12;
        } else if (player_bon::has_trait(Trait::tough)) {
                sprain_one_in_n = 8;
        } else {
                sprain_one_in_n = 4;
        }

        if (rnd::one_in(sprain_one_in_n)) {
                msg_log::add("I sprain myself.", colors::msg_bad());

                const int dmg = rnd::range(1, 2);

                actor::hit(*map::g_player, dmg, DmgType::pure);
        }
}

void run()
{
        TRACE_FUNC_BEGIN;

        msg_log::clear();

        // Choose direction
        msg_log::add(
                "Which direction? " + common_text::g_cancel_hint,
                colors::light_white(),
                MsgInterruptPlayer::no,
                MorePromptOnMsg::no,
                CopyToMsgHistory::no);

        const Dir input_dir = query::dir(AllowCenter::yes);

        msg_log::clear();

        if (input_dir == Dir::END) {
                // Invalid direction
                io::update_screen();

                TRACE_FUNC_END;

                return;
        }

        // The chosen direction is valid

        P att_pos(map::g_player->m_pos + dir_utils::offset(input_dir));

        // Kick living actor?
        TRACE << "Checking if player is kicking a living actor" << std::endl;

        if (input_dir != Dir::center) {
                auto* living_actor =
                        map::first_actor_at_pos(att_pos, ActorState::alive);

                if (living_actor) {
                        TRACE << "Actor found at kick pos" << std::endl;

                        const bool melee_allowed =
                                map::g_player->m_properties.allow_attack_melee(
                                        Verbose::yes);

                        if (melee_allowed) {
                                TRACE << "Player is allowed to do melee attack"
                                      << std::endl;

                                Array2<bool> blocked(map::dims());

                                map_parsers::BlocksLos()
                                        .run(blocked, blocked.rect());

                                TRACE << "Player can see actor" << std::endl;

                                map::g_player->kick_mon(*living_actor);

                                try_sprain_player();

                                // Attacking ends cloaking
                                map::g_player->m_properties.end_prop(
                                        PropId::cloaked);

                                game_time::tick();
                        }

                        TRACE_FUNC_END;
                        return;
                }
        }

        // Destroy corpse?
        TRACE << "Checking if player is destroying a corpse" << std::endl;
        actor::Actor* corpse = nullptr;

        // Check all corpses here, stop at any corpse which is prioritized for
        // bashing (Zombies)
        for (auto* const actor : game_time::g_actors) {
                if ((actor->m_pos == att_pos) &&
                    (actor->m_state == ActorState::corpse)) {
                        corpse = actor;

                        if (actor->m_data->prio_corpse_bash) {
                                break;
                        }
                }
        }

        const auto kick_wpn =
                std::unique_ptr<item::Wpn>(
                        static_cast<item::Wpn*>(
                                item::make(item::Id::player_kick)));

        const auto* wpn = map::g_player->m_inv.item_in_slot(SlotId::wpn);

        if (!wpn) {
                wpn = &map::g_player->unarmed_wpn();
        }

        ASSERT(wpn);

        if (corpse) {
                const bool is_seeing_cell =
                        map::g_cells.at(att_pos).is_seen_by_player;

                std::string corpse_name =
                        is_seeing_cell
                        ? corpse->m_data->corpse_name_the
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
                        melee_att_msg + " " +
                        corpse_name + ".";

                msg_log::add(msg);

                const auto dmg_range =
                        wpn_used_att_corpse->melee_dmg(map::g_player);

                const int dmg = dmg_range.total_range().roll();

                actor::hit(
                        *corpse,
                        dmg,
                        DmgType::physical,
                        wpn_used_att_corpse->data().melee.dmg_method);

                if (wpn_used_att_corpse == kick_wpn.get()) {
                        try_sprain_player();
                }

                if (corpse->m_state == ActorState::destroyed) {
                        std::vector<actor::Actor*> corpses_here;

                        for (auto* const actor : game_time::g_actors) {
                                if ((actor->m_pos == att_pos) &&
                                    actor->is_corpse()) {
                                        corpses_here.push_back(actor);
                                }
                        }

                        if (!corpses_here.empty()) {
                                msg_log::more_prompt();
                        }

                        for (auto* const other_corpse : corpses_here) {
                                const std::string name =
                                        text_format::first_to_upper(
                                                other_corpse
                                                        ->m_data
                                                        ->corpse_name_a);

                                msg_log::add(name + ".");
                        }
                }

                // Attacking ends cloaking
                map::g_player->m_properties.end_prop(PropId::cloaked);

                game_time::tick();

                TRACE_FUNC_END;

                return;
        }

        // Attack terrain
        TRACE << "Checking if player is kicking terrain" << std::endl;

        if (input_dir != Dir::center) {
                // Decide if we should kick or use wielded weapon
                auto* const terrain = map::g_cells.at(att_pos).terrain;

                bool allow_wpn_att_terrain = wpn->data().melee.att_terrain;

                const auto wpn_dmg_method = wpn->data().melee.dmg_method;

                if (allow_wpn_att_terrain) {
                        switch (terrain->id()) {
                        case terrain::Id::door: {
                                const auto* const door =
                                        static_cast<const terrain::Door*>(
                                                terrain);

                                const auto door_type = door->type();

                                if (door_type == DoorType::gate) {
                                        // Only allow blunt weapons for gates
                                        // (feels weird to attack a barred gate
                                        // with an axe...)
                                        allow_wpn_att_terrain =
                                                (wpn_dmg_method ==
                                                 DmgMethod::blunt);
                                } else // Not gate (i.e. wooden, metal)
                                {
                                        allow_wpn_att_terrain = true;
                                }
                        } break;

                        case terrain::Id::wall: {
                                allow_wpn_att_terrain = true;
                        } break;

                        default:
                        {
                                allow_wpn_att_terrain = false;
                        } break;
                        }
                }

                const auto* const wpn_used_att_terrain =
                        allow_wpn_att_terrain
                        ? wpn
                        : kick_wpn.get();

                const auto dmg_range =
                        wpn_used_att_terrain->melee_dmg(map::g_player);

                const int dmg = dmg_range.total_range().roll();

                terrain->hit(
                        dmg,
                        DmgType::physical,
                        wpn_used_att_terrain->data().melee.dmg_method,
                        map::g_player);

                // Attacking ends cloaking
                map::g_player->m_properties.end_prop(PropId::cloaked);

                game_time::tick();
        }

        TRACE_FUNC_END;
}

} // namespace wham
