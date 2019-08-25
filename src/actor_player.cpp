// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "actor_player.hpp"

#include <string>
#include <cmath>

#include "actor_death.hpp"
#include "actor_factory.hpp"
#include "actor_mon.hpp"
#include "actor_move.hpp"
#include "attack.hpp"
#include "audio.hpp"
#include "bot.hpp"
#include "common_text.hpp"
#include "create_character.hpp"
#include "drop.hpp"
#include "explosion.hpp"
#include "terrain_door.hpp"
#include "terrain_mob.hpp"
#include "terrain_trap.hpp"
#include "flood.hpp"
#include "fov.hpp"
#include "game.hpp"
#include "game_commands.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "insanity.hpp"
#include "inventory.hpp"
#include "inventory_handling.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_device.hpp"
#include "item_factory.hpp"
#include "item_potion.hpp"
#include "item_rod.hpp"
#include "item_scroll.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "minimap.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "pact.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "popup.hpp"
#include "property.hpp"
#include "property_handler.hpp"
#include "query.hpp"
#include "reload.hpp"
#include "saving.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static const int s_min_dmg_to_wound = 5;


static const std::vector<std::string> item_feeling_messages_ = {
        "I feel like I should examine this place thoroughly.",
        "I feel like there is something of great interest here.",
        "I sense an object of great power here."
};

static int nr_wounds(const PropHandler& properties)
{
        if (properties.has(PropId::wound))
        {
                const auto* const prop =
                        properties.prop(PropId::wound);

                const PropWound* const wound =
                        static_cast<const PropWound*>(prop);

                return wound->nr_wounds();
        }
        else
        {
                return 0;
        }
}

// -----------------------------------------------------------------------------
// actor
// -----------------------------------------------------------------------------
namespace actor
{

// -----------------------------------------------------------------------------
// Player
// -----------------------------------------------------------------------------
Player::Player() :
        Actor()
{

}

Player::~Player()
{
        delete m_active_explosive;
        delete m_unarmed_wpn;
}

void Player::save() const
{
        m_properties.save();

        saving::put_int(m_ins);
        saving::put_int((int)m_shock);
        saving::put_int(m_hp);
        saving::put_int(m_base_max_hp);
        saving::put_int(m_sp);
        saving::put_int(m_base_max_sp);
        saving::put_int(m_pos.x);
        saving::put_int(m_pos.y);
        saving::put_int(m_nr_turns_until_rspell);

        ASSERT(m_unarmed_wpn);

        saving::put_int((int)m_unarmed_wpn->id());

        for (int i = 0; i < (int)AbilityId::END; ++i)
        {
                const int v = m_data->ability_values.raw_val((AbilityId)i);

                saving::put_int(v);
        }
}

void Player::load()
{
        m_properties.load();

        m_ins = saving::get_int();
        m_shock = double(saving::get_int());
        m_hp = saving::get_int();
        m_base_max_hp = saving::get_int();
        m_sp = saving::get_int();
        m_base_max_sp = saving::get_int();
        m_pos.x = saving::get_int();
        m_pos.y = saving::get_int();
        m_nr_turns_until_rspell = saving::get_int();

        const auto unarmed_wpn_id = (item::Id)saving::get_int();

        ASSERT(unarmed_wpn_id < item::Id::END);

        delete m_unarmed_wpn;
        m_unarmed_wpn = nullptr;

        auto* const unarmed_item = item::make(unarmed_wpn_id);

        ASSERT(unarmed_item);

        m_unarmed_wpn = static_cast<item::Wpn*>(unarmed_item);

        for (int i = 0; i < (int)AbilityId::END; ++i)
        {
                const int v = saving::get_int();

                m_data->ability_values.set_val((AbilityId)i, v);
        }
}

bool Player::can_see_actor(const Actor& other) const
{
        if (this == &other)
        {
                return true;
        }

        if (init::g_is_cheat_vision_enabled)
        {
                return true;
        }

        const Cell& cell = map::g_cells.at(other.m_pos);

        // Dead actors are seen if the cell is seen
        if (!other.is_alive() &&
            cell.is_seen_by_player)
        {
                return true;
        }

        // Player is blind?
        if (!m_properties.allow_see())
        {
                return false;
        }

        const Mon* const mon = static_cast<const Mon*>(&other);

        // LOS blocked hard (e.g. a wall)?
        if (cell.player_los.is_blocked_hard)
        {
                return false;
        }

        const bool can_see_invis = m_properties.has(PropId::see_invis);

        // Monster is invisible, and player cannot see invisible?
        if ((other.m_properties.has(PropId::invis) ||
             other.m_properties.has(PropId::cloaked)) &&
            !can_see_invis)
        {
                return false;
        }

        // Blocked by darkness, and not seeing monster with infravision?
        const bool has_darkvision = m_properties.has(PropId::darkvision);

        const bool can_see_other_in_drk = can_see_invis || has_darkvision;

        if (cell.player_los.is_blocked_by_dark && !can_see_other_in_drk)
        {
                return false;
        }

        if (mon->is_sneaking() && !can_see_invis)
        {
                return false;
        }

        // OK, all checks passed, actor can bee seen!
        return true;
}

std::vector<Actor*> Player::seen_actors() const
{
        std::vector<Actor*> out;

        for (Actor* actor : game_time::g_actors)
        {
                if ((actor != this) &&
                    actor->is_alive())
                {
                        if (can_see_actor(*actor))
                        {
                                out.push_back(actor);
                        }
                }
        }

        return out;
}

std::vector<Actor*> Player::seen_foes() const
{
        std::vector<Actor*> out;

        for (Actor* actor : game_time::g_actors)
        {
                if ((actor != this) &&
                    actor->is_alive() &&
                    map::g_player->can_see_actor(*actor) &&
                    !is_leader_of(actor))
                {
                        out.push_back(actor);
                }
        }

        return out;
}

void Player::on_hit(
        int& dmg,
        const DmgType dmg_type,
        const DmgMethod method,
        const AllowWound allow_wound)
{
        (void)method;

        if (!insanity::has_sympt(InsSymptId::masoch))
        {
                incr_shock(1, ShockSrc::misc);
        }

        const bool is_enough_dmg_for_wound = dmg >= s_min_dmg_to_wound;
        const bool is_physical = dmg_type == DmgType::physical;

        // Ghoul trait Indomitable Fury grants immunity to wounds while frenzied
        const bool is_ghoul_resist_wound =
                player_bon::has_trait(Trait::indomitable_fury) &&
                m_properties.has(PropId::frenzied);

        if ((allow_wound == AllowWound::yes) &&
            (m_hp - dmg) > 0 &&
            is_enough_dmg_for_wound &&
            is_physical &&
            !is_ghoul_resist_wound &&
            !config::is_bot_playing())
        {
                auto* const prop = new PropWound();

                prop->set_indefinite();

                const int nr_wounds_before = nr_wounds(m_properties);

                m_properties.apply(prop);

                const int nr_wounds_after = nr_wounds(m_properties);

                if (nr_wounds_after > nr_wounds_before)
                {
                        if (insanity::has_sympt(InsSymptId::masoch))
                        {
                                game::add_history_event(
                                        "Experienced a very pleasant wound.");

                                msg_log::add("Hehehe...");

                                const double shock_restored = 10.0;

                                m_shock = std::max(
                                        0.0,
                                        m_shock - shock_restored);
                        }
                        else // Not masochistic
                        {
                                game::add_history_event(
                                        "Sustained a severe wound.");
                        }
                }
        }
}

int Player::enc_percent() const
{
        const int total_w = m_inv.total_item_weight();
        const int max_w = carry_weight_lmt();

        return (int)(((double)total_w / (double)max_w) * 100.0);
}

int Player::carry_weight_lmt() const
{
        int carry_weight_mod = 0;

        if (player_bon::has_trait(Trait::strong_backed))
        {
                carry_weight_mod += 50;
        }

        if (m_properties.has(PropId::weakened))
        {
                carry_weight_mod -= 15;
        }

        return (g_player_carry_weight_base * (carry_weight_mod + 100)) / 100;
}

int Player::shock_resistance(const ShockSrc shock_src) const
{
        int res = 0;

        if (player_bon::has_trait(Trait::cool_headed))
        {
                res += 20;
        }

        if (player_bon::has_trait(Trait::courageous))
        {
                res += 20;
        }

        if (player_bon::has_trait(Trait::fearless))
        {
                res += 10;
        }

        switch (shock_src)
        {
        case ShockSrc::use_strange_item:
        case ShockSrc::cast_intr_spell:
                if (player_bon::bg() == Bg::occultist)
                {
                        res += 50;
                }
                break;

        case ShockSrc::see_mon:
                if (player_bon::bg() == Bg::ghoul)
                {
                        res += 50;
                }
                break;

        case ShockSrc::time:
        case ShockSrc::misc:
        case ShockSrc::END:
                break;
        }

        return constr_in_range(0, res, 100);
}

double Player::shock_taken_after_mods(
        const double base_shock,
        const ShockSrc shock_src) const
{
        const double shock_res_db = (double)shock_resistance(shock_src);

        return (base_shock * (100.0 - shock_res_db)) / 100.0;
}

double Player::shock_lvl_to_value(const ShockLvl shock_lvl) const
{
        double shock_value = 0.0;

        switch (shock_lvl)
        {
        case ShockLvl::unsettling:
                shock_value = 2.0;
                break;

        case ShockLvl::frightening:
                shock_value = 4.0;
                break;

        case ShockLvl::terrifying:
                shock_value = 12.0;
                break;

        case ShockLvl::mind_shattering:
                shock_value = 50.0;
                break;

        case ShockLvl::none:
        case ShockLvl::END:
                break;
        }

        return shock_value;
}

void Player::incr_shock(double shock, ShockSrc shock_src)
{
        shock = shock_taken_after_mods(shock, shock_src);

        m_shock += shock;

        m_perm_shock_taken_current_turn += shock;

        m_shock = std::max(0.0, m_shock);
}

void Player::incr_shock(const ShockLvl shock_lvl, ShockSrc shock_src)
{
        double shock_value = shock_lvl_to_value(shock_lvl);

        if (shock_value > 0.0)
        {
                incr_shock(shock_value, shock_src);
        }
}

void Player::restore_shock(
        const int amount_restored,
        const bool is_temp_shock_restored)
{
        m_shock = std::max(0.0, m_shock - amount_restored);

        if (is_temp_shock_restored)
        {
                m_shock_tmp = 0.0;
        }
}

void Player::incr_insanity()
{
        TRACE << "Increasing insanity" << std::endl;

        if (!config::is_bot_playing())
        {
                const int ins_incr = rnd::range(10, 15);

                m_ins += ins_incr;
        }

        if (ins() >= 100)
        {
                const std::string msg =
                        "My mind can no longer withstand what it has grasped. "
                        "I am hopelessly lost.";

                popup::msg(msg, "Insane!", SfxId::insanity_rise);

                kill(
                        *this,
                        IsDestroyed::yes,
                        AllowGore::no,
                        AllowDropItems::no);

                return;
        }

        // This point reached means insanity is below 100%
        insanity::run_sympt();

        restore_shock(999, true);
}

void Player::item_feeling()
{
        if ((player_bon::bg() != Bg::rogue) ||
            !rnd::percent(80))
        {
                return;
        }

        bool print_feeling = false;

        auto is_nice = [](const item::Item& item) {
                return item.data().value == item::Value::supreme_treasure;
        };

        for (auto& cell : map::g_cells)
        {
                // Nice item on the floor, which is not seen by the player?
                if (cell.item &&
                    is_nice(*cell.item) &&
                    !cell.is_seen_by_player)
                {
                        print_feeling = true;

                        break;
                }

                // Nice item in container?
                const auto& cont_items = cell.terrain->m_item_container.m_items;

                for (const auto* const item : cont_items)
                {
                        if (is_nice(*item))
                        {
                                print_feeling = true;

                                break;
                        }
                }

                if (print_feeling)
                {
                        const std::string msg =
                                rnd::element(item_feeling_messages_);

                        msg_log::add(
                                msg,
                                colors::light_cyan(),
                                MsgInterruptPlayer::no,
                                MorePromptOnMsg::yes);

                        return;
                }
        } // map cells loop
}

void Player::on_new_dlvl_reached()
{
        mon_feeling();

        item_feeling();

        for (auto& slot: m_inv.m_slots)
        {
                if (slot.item)
                {
                        slot.item->on_player_reached_new_dlvl();
                }
        }

        pact::on_player_reached_new_dlvl();

        for (auto* const item: m_inv.m_backpack)
        {
                item->on_player_reached_new_dlvl();
        }
}

void Player::mon_feeling()
{
        if (player_bon::bg() != Bg::rogue)
        {
                return;
        }

        bool print_unique_mon_feeling = false;

        for (Actor* actor : game_time::g_actors)
        {
                // Not a hostile, living monster?
                if (actor->is_player() ||
                    map::g_player->is_leader_of(actor) ||
                    !actor->is_alive())
                {
                        continue;
                }

                auto* mon = static_cast<Mon*>(actor);

                // Print monster feeling for monsters spawned during the level?
                // (We do the actual printing once, after the loop, so that we
                // don't print something like "A chill runs down my spine (x2)")
                if (mon->m_data->is_unique &&
                    mon->m_is_player_feeling_msg_allowed)
                {
                        print_unique_mon_feeling = true;

                        mon->m_is_player_feeling_msg_allowed = false;
                }
        }

        if (print_unique_mon_feeling &&
            rnd::percent(80))
        {
                std::vector<std::string> msg_bucket
                {
                        "A chill runs down my spine.",
                                "I sense a great danger.",
                                };

                // This message only makes sense if the player is fearful
                if (!player_bon::has_trait(Trait::fearless) &&
                    !m_properties.has(PropId::frenzied))
                {
                        msg_bucket.push_back("I feel anxious.");
                }

                const auto msg = rnd::element(msg_bucket);

                msg_log::add(msg,
                             colors::msg_note(),
                             MsgInterruptPlayer::no,
                             MorePromptOnMsg::yes);
        }
}

void Player::set_auto_move(const Dir dir)
{
        ASSERT(dir != Dir::END);

        m_auto_move_dir = dir;

        m_has_taken_auto_move_step = false;
}

void Player::act()
{
        if (!is_alive())
        {
                return;
        }

        if (m_properties.on_act() == DidAction::yes)
        {
                return;
        }

        if (m_tgt && (m_tgt->m_state != ActorState::alive))
        {
                m_tgt = nullptr;
        }

        if (m_active_medical_bag)
        {
                m_active_medical_bag->continue_action();

                return;
        }

        if (m_handle_armor_countdown > 0)
        {
                --m_handle_armor_countdown;

                // Done handling armor now?
                if (m_handle_armor_countdown == 0)
                {
                        if (m_armor_putting_on_backpack_idx >= 0)
                        {
                                // Putting on armor
                                m_inv.equip_backpack_item(
                                        m_armor_putting_on_backpack_idx,
                                        SlotId::body);

                                m_armor_putting_on_backpack_idx = -1;
                        }
                        else if (m_is_dropping_armor_from_body_slot)
                        {
                                // Dropping armor
                                item_drop::drop_item_from_inv(
                                        *map::g_player,
                                        InvType::slots,
                                        (size_t)SlotId::body);

                                m_is_dropping_armor_from_body_slot = false;
                        }
                        else
                        {
                                // Taking off armor
                                m_inv.unequip_slot(SlotId::body);
                        }
                }
                else // Not done handling armor yet
                {
                        game_time::tick();
                }

                return;
        }

        if (m_wpn_equipping)
        {
                m_inv.equip_backpack_item(m_wpn_equipping, SlotId::wpn);

                m_wpn_equipping = nullptr;

                game_time::tick();

                return;
        }

        if (wait_turns_left > 0)
        {
                --wait_turns_left;

                move(*this, Dir::center);

                return;
        }

        // Auto move
        if (m_auto_move_dir != Dir::END)
        {
                const P target = m_pos + dir_utils::offset(m_auto_move_dir);

                bool is_target_adj_to_unseen_cell = false;

                for (const P& d : dir_utils::g_dir_list_w_center)
                {
                        const P p_adj(target + d);

                        const Cell& adj_cell = map::g_cells.at(p_adj);

                        if (!adj_cell.is_seen_by_player)
                        {
                                is_target_adj_to_unseen_cell = true;

                                break;
                        }
                }

                const Cell& target_cell = map::g_cells.at(target);

                // If this is not the first step of auto moving, stop before
                // blocking terrains, fire, known traps, etc - otherwise allow
                // bumping terrains as with normal movement
                if (m_has_taken_auto_move_step)
                {
                        bool should_abort = false;

                        if (!target_cell.terrain->can_move(*this))
                        {
                                should_abort = true;
                        }
                        else
                        {
                                const auto target_terrain_id = target_cell.terrain->id();

                                const bool is_target_known_trap =
                                        target_cell.is_seen_by_player &&
                                        (target_terrain_id == terrain::Id::trap) &&
                                        !static_cast<const terrain::Trap*>(target_cell.terrain)->is_hidden();

                                should_abort =
                                        is_target_known_trap ||
                                        (target_terrain_id == terrain::Id::chains) ||
                                        (target_terrain_id == terrain::Id::liquid_shallow) ||
                                        (target_terrain_id == terrain::Id::vines) ||
                                        (target_cell.terrain->m_burn_state == BurnState::burning);
                        }

                        if (should_abort)
                        {
                                m_auto_move_dir = Dir::END;

                                return;
                        }
                }

                auto adj_known_closed_doors = [](const P& p)
                        {
                                std::vector<const terrain::Door*> doors;

                                for (const P& d : dir_utils::g_dir_list_w_center)
                                {
                                        const P p_adj(p + d);

                                        const Cell& adj_cell = map::g_cells.at(p_adj);

                                        if (adj_cell.is_seen_by_player &&
                                            (adj_cell.terrain->id() == terrain::Id::door))
                                        {
                                                const auto* const door =
                                                        static_cast<const terrain::Door*>(adj_cell.terrain);

                                                if (!door->is_hidden() &&
                                                    !door->is_open())
                                                {
                                                        doors.push_back(door);
                                                }
                                        }
                                }

                                return doors;
                        };

                const auto adj_known_closed_doors_before = adj_known_closed_doors(m_pos);

                move(*this, m_auto_move_dir);

                m_has_taken_auto_move_step = true;

                update_fov();

                if (m_auto_move_dir == Dir::END)
                {
                        return;
                }

                const auto adj_known_closed_doors_after =
                        adj_known_closed_doors(m_pos);

                bool is_new_known_adj_closed_door = false;

                for (const auto* const door_after : adj_known_closed_doors_after)
                {
                        is_new_known_adj_closed_door =
                                std::find(begin(adj_known_closed_doors_before),
                                          end(adj_known_closed_doors_before),
                                          door_after) ==
                                end(adj_known_closed_doors_before);

                        if (is_new_known_adj_closed_door)
                        {
                                break;
                        }
                }

                if (is_target_adj_to_unseen_cell || is_new_known_adj_closed_door)
                {
                        m_auto_move_dir = Dir::END;
                }

                return;
        }

        // If this point is reached - read input
        if (config::is_bot_playing())
        {
                bot::act();
        }
        else // Not bot playing
        {
                const auto input = io::get();

                const auto game_cmd = game_commands::to_cmd(input);

                game_commands::handle(game_cmd);
        }
}

bool Player::is_seeing_burning_terrain() const
{
        const R fov_r = fov::fov_rect(m_pos, map::dims());

        bool is_fire_found = false;

        for (int x = fov_r.p0.x; x <= fov_r.p1.x; ++x)
        {
                for (int y = fov_r.p0.y; y <= fov_r.p1.y; ++y)
                {
                        const auto& cell = map::g_cells.at(x, y);

                        if (cell.is_seen_by_player &&
                            (cell.terrain->m_burn_state == BurnState::burning))
                        {
                                is_fire_found = true;

                                break;
                        }
                }

                if (is_fire_found)
                {
                        break;
                }
        }

        return is_fire_found;
}

bool Player::is_busy() const
{
        return
                m_active_medical_bag ||
                (m_handle_armor_countdown > 0) ||
                m_wpn_equipping ||
                (wait_turns_left > 0) ||
                (m_auto_move_dir != Dir::END);
}

void Player::on_actor_turn()
{
        reset_perm_shock_taken_current_turn();

        update_fov();

        update_mon_awareness();

        // Set current temporary shock from darkness etc
        update_tmp_shock();

        if (is_busy() && is_seeing_burning_terrain())
        {
                msg_log::add(
                        common_text::g_fire_prevent_cmd,
                        colors::text(),
                        MsgInterruptPlayer::yes);
        }

        Array2<int> vigilant_flood(map::dims());

        if (player_bon::has_trait(Trait::vigilant))
        {
                Array2<bool> blocks_sound(map::dims());

                const int d = 3;

                const R area(
                        P(std::max(0, m_pos.x - d),
                          std::max(0, m_pos.y - d)),
                        P(std::min(map::w() - 1, m_pos.x + d),
                          std::min(map::h() - 1, m_pos.y + d)));

                map_parsers::BlocksSound()
                        .run(blocks_sound,
                             area,
                             MapParseMode::overwrite);

                vigilant_flood = floodfill(m_pos, blocks_sound, d);
        }

        bool is_old_actor_seen = false;

        Actor* actor_to_warn_about = nullptr;

        for (Actor* actor : game_time::g_actors)
        {
                // Not a hostile, living monster?
                if (actor->is_player() ||
                    map::g_player->is_leader_of(actor) ||
                    !actor->is_alive())
                {
                        continue;
                }

                Mon& mon = *static_cast<Mon*>(actor);

                const bool is_mon_seen = can_see_actor(*actor);

                if (is_mon_seen)
                {
                        mon.m_is_player_feeling_msg_allowed = false;

                        if (mon.m_is_msg_mon_in_view_printed)
                        {
                                is_old_actor_seen = true;
                        }

                        if (is_busy() ||
                            (config::always_warn_new_mon() && !is_old_actor_seen))
                        {
                                actor_to_warn_about = actor;
                        }
                        else
                        {
                                actor_to_warn_about = nullptr;
                        }

                        mon.m_is_msg_mon_in_view_printed = true;
                }
                else // Monster is not seen
                {
                        if (mon.m_player_aware_of_me_counter <= 0)
                        {
                                mon.m_is_msg_mon_in_view_printed = false;
                        }

                        const bool is_cell_seen = map::g_cells.at(mon.m_pos).is_seen_by_player;

                        const bool is_mon_invis =
                                mon.m_properties.has(PropId::invis) ||
                                mon.m_properties.has(PropId::cloaked);

                        const bool can_player_see_invis =
                                m_properties.has(PropId::see_invis);

                        // NOTE: We only run the flodofill within a limited area, so ANY
                        // cell reached by the flood is considered as within distance
                        const bool is_mon_within_vigilant_dist =
                                vigilant_flood.at(mon.m_pos) > 0;

                        // If the monster is invisible (and the player cannot see invisible
                        // monsters), or if the cells is not seen, being Vigilant will make
                        // the player aware of the monster if within a certain distance
                        if (player_bon::has_trait(Trait::vigilant) &&
                            is_mon_within_vigilant_dist &&
                            ((is_mon_invis && !can_player_see_invis) ||
                             !is_cell_seen))
                        {
                                if (mon.m_player_aware_of_me_counter <= 0)
                                {
                                        if (is_cell_seen)
                                        {
                                                // The monster must be invisible
                                                print_aware_invis_mon_msg(mon);
                                        }
                                        else // Became aware of a monster in an unseen cell
                                        {
                                                // Abort quick move
                                                m_auto_move_dir = Dir::END;
                                        }
                                }

                                mon.set_player_aware_of_me();
                        }
                        else if (mon.is_sneaking())
                        {
                                auto sneak_result = ActionResult::success;

                                if (player_bon::has_trait(Trait::vigilant) &&
                                    is_mon_within_vigilant_dist)
                                {
                                        sneak_result = ActionResult::fail;
                                }
                                // Monster cannot be discovered with Vigilant
                                else if (is_cell_seen)
                                {
                                        SneakData sneak_data;

                                        sneak_data.actor_sneaking = &mon;
                                        sneak_data.actor_searching = this;

                                        sneak_result = roll_sneak(sneak_data);
                                }

                                if (sneak_result <= ActionResult::fail)
                                {
                                        mon.set_player_aware_of_me();

                                        const std::string mon_name = mon.name_a();

                                        msg_log::add(
                                                "I spot " + mon_name + "!",
                                                colors::msg_note(),
                                                MsgInterruptPlayer::yes,
                                                MorePromptOnMsg::yes);

                                        mon.m_is_msg_mon_in_view_printed = true;

                                        is_old_actor_seen = true;

                                        actor_to_warn_about = nullptr;
                                }
                        }
                }
        } // actor loop

        if (actor_to_warn_about)
        {
                const std::string name_a =
                        text_format::first_to_upper(
                                actor_to_warn_about->name_a());

                msg_log::add(
                        name_a + " is in my view.",
                        colors::text(),
                        MsgInterruptPlayer::yes,
                        MorePromptOnMsg::yes);
        }

        mon_feeling();

        const auto my_seen_foes = seen_foes();

        for (Actor* actor : my_seen_foes)
        {
                static_cast<Mon*>(actor)->set_player_aware_of_me();

                game::on_mon_seen(*actor);
        }

        add_shock_from_seen_monsters();

        if (m_properties.allow_act())
        {
                // Passive shock taken over time
                double passive_shock_taken = 0.1075;

                if (player_bon::bg() == Bg::rogue)
                {
                        passive_shock_taken *= 0.75;
                }

                incr_shock(passive_shock_taken, ShockSrc::time);
        }

        // Run new turn events on all items
        for (auto* const item : m_inv.m_backpack)
        {
                item->on_actor_turn_in_inv(InvType::backpack);
        }

        for (InvSlot& slot : m_inv.m_slots)
        {
                if (slot.item)
                {
                        slot.item->on_actor_turn_in_inv(InvType::slots);
                }
        }

        pact::on_player_turn();

        if (!is_alive())
        {
                return;
        }

        // Take sanity hit from high shock?
        if (shock_tot() >= 100)
        {
                m_nr_turns_until_ins =
                        (m_nr_turns_until_ins < 0)
                        ? 3
                        : (m_nr_turns_until_ins - 1);

                if (m_nr_turns_until_ins > 0)
                {
                        msg_log::add(
                                "I feel my sanity slipping...",
                                colors::msg_note(),
                                MsgInterruptPlayer::yes,
                                MorePromptOnMsg::yes);
                }
                else // Time to go crazy!
                {
                        m_nr_turns_until_ins = -1;

                        incr_insanity();

                        if (is_alive())
                        {
                                game_time::tick();
                        }

                        return;
                }
        }
        else // Total shock is less than 100%
        {
                m_nr_turns_until_ins = -1;
        }

        insanity::on_new_player_turn(my_seen_foes);
}

void Player::add_shock_from_seen_monsters()
{
        if (!m_properties.allow_see())
        {
                return;
        }

        double val = 0.0;

        for (Actor* actor : game_time::g_actors)
        {
                if (actor->is_player() ||
                    !actor->is_alive() ||
                    (is_leader_of(actor)))
                {
                        continue;
                }

                Mon* mon = static_cast<Mon*>(actor);

                if (mon->m_player_aware_of_me_counter <= 0)
                {
                        continue;
                }

                ShockLvl shock_lvl = ShockLvl::none;

                if (can_see_actor(*mon))
                {
                        shock_lvl = mon->m_data->mon_shock_lvl;
                }
                else // Monster cannot be seen
                {
                        const P mon_p = mon->m_pos;

                        if (map::g_cells.at(mon_p).is_seen_by_player)
                        {
                                // There is an invisible monster here! How scary!
                                shock_lvl = ShockLvl::terrifying;
                        }
                }

                switch (shock_lvl)
                {
                case ShockLvl::unsettling:
                        val += 0.05;
                        break;

                case ShockLvl::frightening:
                        val += 0.3;
                        break;

                case ShockLvl::terrifying:
                        val += 0.6;
                        break;

                case ShockLvl::mind_shattering:
                        val += 1.75;
                        break;

                case ShockLvl::none:
                case ShockLvl::END:
                        break;
                }
        }

        // Dampen the progression (it doesn't seem right that e.g. 8 monsters are
        // twice as scary as 4 monsters).
        val = std::sqrt(val);

        // Cap the value
        const double cap = 5.0;

        val = std::min(cap, val);

        incr_shock(val, ShockSrc::see_mon);
}

void Player::update_tmp_shock()
{
        m_shock_tmp = 0.0;

        const int tot_shock_before = shock_tot();

        // Minimum temporary shock

        // NOTE: In case the total shock is currently at 100, we do NOT want to
        // allow lowering the shock e.g. by turning on the Electric Lantern,
        // since you could interrupt the 3 turns countdown until the insanity
        // event happens just ny turning the lantern on for one turn. Therefore
        // we only allow negative temporary shock while below 100%.
        double shock_tmp_min =
                (tot_shock_before < 100) ?
                -999.0 :
                0.0;

        // "Obessions" raise the minimum temporary shock
        if (insanity::has_sympt(InsSymptId::sadism) ||
            insanity::has_sympt(InsSymptId::masoch))
        {
                m_shock_tmp = std::max(
                        m_shock_tmp,
                        (double)g_shock_from_obsession);

                shock_tmp_min = (double)g_shock_from_obsession;
        }

        if (m_properties.allow_see())
        {
                // Shock reduction from light?
                if (map::g_light.at(m_pos))
                {
                        m_shock_tmp -= 20.0;
                }
                // Not lit - shock from darkness?
                else if (map::g_dark.at(m_pos) && (player_bon::bg() != Bg::ghoul))
                {
                        double shock_value = 20.0;

                        if (insanity::has_sympt(InsSymptId::phobia_dark))
                        {
                                shock_value = 30.0;
                        }

                        m_shock_tmp += shock_taken_after_mods(shock_value, ShockSrc::misc);
                }

                // Temporary shock from seen terrains?
                for (const P& d : dir_utils::g_dir_list_w_center)
                {
                        const P p(m_pos + d);

                        const double terrain_shock_db =
                                (double)map::g_cells.at(p).terrain->shock_when_adj();

                        m_shock_tmp += shock_taken_after_mods(
                                terrain_shock_db,
                                ShockSrc::misc);
                }
        }
        // Is blind
        else if (!m_properties.has(PropId::fainted))
        {
                m_shock_tmp += shock_taken_after_mods(30.0, ShockSrc::misc);
        }

        const double shock_tmp_max = 100.0 - m_shock;

        constr_in_range(
                shock_tmp_min,
                m_shock_tmp,
                shock_tmp_max);
}

int Player::shock_tot() const
{
        double shock_tot_db = m_shock + m_shock_tmp;

        shock_tot_db = std::max(0.0, shock_tot_db);

        shock_tot_db = std::floor(shock_tot_db);

        int result = (int)shock_tot_db;

        result = m_properties.affect_shock(result);

        return result;
}

int Player::ins() const
{
        int result = m_ins;

        result = std::min(100, result);

        return result;
}

void Player::on_std_turn()
{
#ifndef NDEBUG
        // Sanity check: Disease and infection should not be active at the same time
        ASSERT(!m_properties.has(PropId::diseased) ||
               !m_properties.has(PropId::infected));
#endif // NDEBUG

        if (!is_alive())
        {
                return;
        }

        // Spell resistance
        const int spell_shield_turns_base = 125 + rnd::range(0, 25);

        const int spell_shield_turns_bon =
                player_bon::has_trait(Trait::mighty_spirit) ? 100 :
                player_bon::has_trait(Trait::strong_spirit) ? 50 : 0;

        int nr_turns_to_recharge_spell_shield = std::max(
                1,
                spell_shield_turns_base - spell_shield_turns_bon);

        // Halved number of turns due to the Talisman of Reflection?
        if (m_inv.has_item_in_backpack(item::Id::refl_talisman))
        {
                nr_turns_to_recharge_spell_shield /= 2;
        }

        // If we already have spell resistance (e.g. due to the Spell Shield spell),
        // then always (re)set the cooldown to max number of turns
        if (m_properties.has(PropId::r_spell))
        {
                m_nr_turns_until_rspell = nr_turns_to_recharge_spell_shield;
        }
        // Spell resistance not currently active
        else if (player_bon::has_trait(Trait::stout_spirit))
        {
                if (m_nr_turns_until_rspell <= 0)
                {
                        // Cooldown has finished, OR countdown not initialized

                        if (m_nr_turns_until_rspell == 0)
                        {
                                // Cooldown has finished
                                auto prop = new PropRSpell();

                                prop->set_indefinite();

                                m_properties.apply(prop);

                                msg_log::more_prompt();
                        }

                        m_nr_turns_until_rspell = nr_turns_to_recharge_spell_shield;
                }

                if (!m_properties.has(PropId::r_spell) &&
                    (m_nr_turns_until_rspell > 0))
                {
                        // Spell resistance is in cooldown state, decrement number of
                        // remaining turns
                        --m_nr_turns_until_rspell;
                }
        }

        if (m_active_explosive)
        {
                m_active_explosive->on_std_turn_player_hold_ignited();
        }

        // Regenerate Hit Points
        if (!m_properties.has(PropId::poisoned) &&
            player_bon::bg() != Bg::ghoul)
        {
                int nr_turns_per_hp;

                // Rapid Recoverer trait affects hp regen?
                if (player_bon::has_trait(Trait::rapid_recoverer))
                {
                        nr_turns_per_hp = 2;
                }
                else
                {
                        nr_turns_per_hp = 20;
                }

                // Wounds affect hp regen?
                int nr_wounds = 0;

                if (m_properties.has(PropId::wound))
                {
                        Prop* const prop = m_properties.prop(PropId::wound);

                        nr_wounds = static_cast<PropWound*>(prop)->nr_wounds();
                }

                const bool is_survivalist =
                        player_bon::has_trait(Trait::survivalist);

                const int wound_effect_div = is_survivalist ? 2 : 1;

                nr_turns_per_hp +=
                        ((nr_wounds * 4) / wound_effect_div);

                // Items affect hp regen?
                for (const auto& slot : m_inv.m_slots)
                {
                        if (slot.item)
                        {
                                nr_turns_per_hp +=
                                        slot.item->hp_regen_change(
                                                InvType::slots);
                        }
                }

                for (const auto* const item : m_inv.m_backpack)
                {
                        nr_turns_per_hp +=
                                item->hp_regen_change(InvType::backpack);
                }

                nr_turns_per_hp = std::max(1, nr_turns_per_hp);

                const int turn = game_time::turn_nr();
                const int max_hp = actor::max_hp(*this);

                if ((m_hp < max_hp) &&
                    ((turn % nr_turns_per_hp) == 0) &&
                    turn > 1)
                {
                        ++m_hp;
                }
        }

        // Try to spot hidden traps and doors

        // NOTE: Skill value retrieved here is always at least 1
        const int player_search_skill =
                map::g_player->ability(AbilityId::searching, true);

        if (!m_properties.has(PropId::confused) && m_properties.allow_see())
        {
                for (size_t i = 0; i < map::g_cells.length(); ++i)
                {
                        if (!map::g_cells.at(i).is_seen_by_player)
                        {
                                continue;
                        }

                        auto* t = map::g_cells.at(i).terrain;

                        if (!t->is_hidden())
                        {
                                continue;
                        }

                        const int lit_mod = map::g_light.at(i) ? 5 : 0;

                        const int dist = king_dist(m_pos, t->pos());

                        const int dist_mod = -((dist - 1) * 5);

                        int skill_tot =
                                player_search_skill +
                                lit_mod +
                                dist_mod;

                        if (skill_tot > 0)
                        {
                                const bool is_spotted =
                                        ability_roll::roll(skill_tot) >=
                                        ActionResult::success;

                                if (is_spotted)
                                {
                                        t->reveal(Verbose::yes);

                                        t->on_revealed_from_searching();

                                        msg_log::more_prompt();
                                }
                        }
                }
        }
}

void Player::on_log_msg_printed()
{
        // NOTE: There cannot be any calls to msg_log::add() in this function,
        // as that would cause infinite recursion!

        // All messages abort waiting
        wait_turns_left = -1;

        // All messages abort quick move
        m_auto_move_dir = Dir::END;
}

void Player::interrupt_actions()
{
        // Abort healing
        if (m_active_medical_bag)
        {
                m_active_medical_bag->interrupted();
                m_active_medical_bag = nullptr;
        }

        // Abort putting on / taking off armor?
        if (m_handle_armor_countdown > 0)
        {
                bool should_continue = true;

                if (m_properties.has(PropId::burning))
                {
                        should_continue = false;
                }

                if (should_continue)
                {
                        const std::string turns_left_str =
                                std::to_string(m_handle_armor_countdown);

                        std::string msg = "";

                        if (m_armor_putting_on_backpack_idx >= 0)
                        {
                                auto* const item = m_inv.m_backpack[m_armor_putting_on_backpack_idx];

                                const auto armor_name =
                                        item->name(
                                                ItemRefType::a,
                                                ItemRefInf::yes);

                                msg =
                                        "Putting on " + armor_name +
                                        " (" + turns_left_str + " turns left).";
                        }
                        else
                        {
                                // Taking off armor, or dropping from armor slot
                                auto* const item =
                                        m_inv.item_in_slot(SlotId::body);

                                ASSERT(item);

                                const auto armor_name =
                                        item->name(
                                                ItemRefType::a,
                                                ItemRefInf::yes);

                                msg =
                                        "Taking off " + armor_name +
                                        " (" + turns_left_str + " turns left).";
                        }

                        msg_log::add(msg, colors::light_white());

                        msg_log::add(
                                "Continue? " + common_text::g_yes_or_no_hint,
                                colors::light_white(),
                                MsgInterruptPlayer::no,
                                MorePromptOnMsg::no,
                                CopyToMsgHistory::no);

                        should_continue =
                                (query::yes_or_no() == BinaryAnswer::yes);

                        msg_log::clear();
                }

                if (!should_continue)
                {
                        m_handle_armor_countdown = 0;

                        m_armor_putting_on_backpack_idx = -1;

                        m_is_dropping_armor_from_body_slot = false;
                }
        }

        // Abort equipping item?
        if (m_wpn_equipping)
        {
                const auto wpn_name =
                        m_wpn_equipping->name(
                                ItemRefType::a,
                                ItemRefInf::yes);

                const std::string msg =
                        "Continue equipping " +
                        wpn_name +
                        "? " +
                        common_text::g_yes_or_no_hint;

                msg_log::add(
                        msg,
                        colors::light_white(),
                        MsgInterruptPlayer::no,
                        MorePromptOnMsg::no,
                        CopyToMsgHistory::no);

                const bool should_continue =
                        (query::yes_or_no() == BinaryAnswer::yes);

                msg_log::clear();

                if (!should_continue)
                {
                        m_wpn_equipping = nullptr;
                }
        }

        // Abort waiting
        wait_turns_left = -1;

        // Abort quick move
        m_auto_move_dir = Dir::END;
}

void Player::hear_sound(
        const Snd& snd,
        const bool is_origin_seen_by_player,
        const Dir dir_to_origin,
        const int percent_audible_distance)
{
        (void)is_origin_seen_by_player;

        if (m_properties.has(PropId::deaf))
        {
                return;
        }

        const SfxId sfx = snd.sfx();

        const std::string& msg = snd.msg();

        const bool has_snd_msg = !msg.empty() && msg != " ";

        if (has_snd_msg)
        {
                msg_log::add(
                        msg,
                        colors::text(),
                        MsgInterruptPlayer::no,
                        snd.should_add_more_prompt_on_msg());
        }

        // Play audio after message to ensure sync between audio and animation.
        audio::play(sfx, dir_to_origin, percent_audible_distance);

        if (has_snd_msg)
        {
                Actor* const actor_who_made_snd = snd.actor_who_made_sound();

                if (actor_who_made_snd &&
                    (actor_who_made_snd != this))
                {
                        static_cast<Mon*>(actor_who_made_snd)->set_player_aware_of_me();
                }
        }

        snd.on_heard(*this);
}

Color Player::color() const
{
        if (!is_alive())
        {
                return colors::red();
        }

        if (m_active_explosive)
        {
                return colors::yellow();
        }

        Color tmp_color;

        if (m_properties.affect_actor_color(tmp_color))
        {
                return tmp_color;
        }

        const auto* const lantern_item = m_inv.item_in_backpack(item::Id::lantern);

        if (lantern_item)
        {
                const auto* const lantern =
                        static_cast<const device::Lantern*>(lantern_item);

                if (lantern->is_activated)
                {
                        return colors::yellow();
                }
        }

        if (shock_tot() >= 75)
        {
                return colors::magenta();
        }

        if (m_properties.has(PropId::invis) ||
            m_properties.has(PropId::cloaked))
        {
                return colors::gray();
        }

        return m_data->color;
}

SpellSkill Player::spell_skill(const SpellId id) const
{
        return player_spells::spell_skill(id);
}

void Player::auto_melee()
{
        if (m_tgt &&
            m_tgt->m_state == ActorState::alive &&
            is_pos_adj(m_pos, m_tgt->m_pos, false) &&
            can_see_actor(*m_tgt))
        {
                move(*this, dir_utils::dir(m_tgt->m_pos - m_pos));

                return;
        }

        // If this line reached, there is no adjacent cur target.
        for (const P& d : dir_utils::g_dir_list)
        {
                Actor* const actor = map::first_actor_at_pos(m_pos + d);

                if (actor && !is_leader_of(actor) && can_see_actor(*actor))
                {
                        m_tgt = actor;

                        move(*this, dir_utils::dir(d));

                        return;
                }
        }
}

void Player::kick_mon(Actor& defender)
{
        item::Wpn* kick_wpn = nullptr;

        const ActorData& d = *defender.m_data;

        // TODO: This is REALLY hacky, it should be done another way. Why even
        // have a "stomp" attack?? Why not just kick them as well?
        if (d.actor_size == Size::floor &&
            (d.is_spider ||
             d.is_rat ||
             d.is_snake ||
             d.id == Id::worm_mass ||
             d.id == Id::mind_worms))
        {
                kick_wpn = static_cast<item::Wpn*>(
                        item::make(item::Id::player_stomp));
        }
        else
        {
                kick_wpn = static_cast<item::Wpn*>(
                        item::make(item::Id::player_kick));
        }

        attack::melee(this, m_pos, defender, *kick_wpn);

        delete kick_wpn;
}

item::Wpn& Player::unarmed_wpn()
{
        ASSERT(m_unarmed_wpn);

        return *m_unarmed_wpn;
}

void Player::hand_att(Actor& defender)
{
        item::Wpn& wpn = unarmed_wpn();

        attack::melee(this, m_pos, defender, wpn);
}

void Player::add_light_hook(Array2<bool>& light_map) const
{
        LgtSize lgt_size = LgtSize::none;

        if (m_active_explosive)
        {
                if (m_active_explosive->data().id == item::Id::flare)
                {
                        lgt_size = LgtSize::fov;
                }
        }

        if (lgt_size != LgtSize::fov)
        {
                for (auto* const item : m_inv.m_backpack)
                {
                        LgtSize item_lgt_size = item->lgt_size();

                        if ((int)lgt_size < (int)item_lgt_size)
                        {
                                lgt_size = item_lgt_size;
                        }
                }
        }

        switch (lgt_size)
        {
        case LgtSize::fov:
        {
                Array2<bool> hard_blocked(map::dims());

                const R fov_lmt = fov::fov_rect(m_pos, hard_blocked.dims());

                map_parsers::BlocksLos()
                        .run(hard_blocked,
                             fov_lmt,
                             MapParseMode::overwrite);

                FovMap fov_map;
                fov_map.hard_blocked = &hard_blocked;
                fov_map.light = &map::g_light;
                fov_map.dark = &map::g_dark;

                const auto player_fov = fov::run(m_pos, fov_map);

                for (int x = fov_lmt.p0.x; x <= fov_lmt.p1.x; ++x)
                {
                        for (int y = fov_lmt.p0.y; y <= fov_lmt.p1.y; ++y)
                        {
                                if (!player_fov.at(x, y).is_blocked_hard)
                                {
                                        light_map.at(x, y) = true;
                                }
                        }
                }
        }
        break;

        case LgtSize::small:
                for (int y = m_pos.y - 1; y <= m_pos.y + 1; ++y)
                {
                        for (int x = m_pos.x - 1; x <= m_pos.x + 1; ++x)
                        {
                                light_map.at(x, y) = true;
                        }
                }
                break;

        case LgtSize::none:
                break;
        }
}

void Player::update_fov()
{
        for (auto& cell : map::g_cells)
        {
                cell.is_seen_by_player = false;

                cell.player_los.is_blocked_hard = true;

                cell.player_los.is_blocked_by_dark = false;
        }

        const bool has_darkvision = m_properties.has(PropId::darkvision);

        if (m_properties.allow_see())
        {
                Array2<bool> hard_blocked(map::dims());

                const R fov_lmt = fov::fov_rect(m_pos, hard_blocked.dims());

                map_parsers::BlocksLos()
                        .run(hard_blocked,
                             fov_lmt,
                             MapParseMode::overwrite);

                FovMap fov_map;
                fov_map.hard_blocked = &hard_blocked;
                fov_map.light = &map::g_light;
                fov_map.dark = &map::g_dark;

                const auto player_fov = fov::run(m_pos, fov_map);

                for (int x = fov_lmt.p0.x; x <= fov_lmt.p1.x; ++x)
                {
                        for (int y = fov_lmt.p0.y; y <= fov_lmt.p1.y; ++y)
                        {
                                const LosResult& los = player_fov.at(x, y);

                                Cell& cell = map::g_cells.at(x, y);

                                cell.is_seen_by_player =
                                        !los.is_blocked_hard &&
                                        (!los.is_blocked_by_dark || has_darkvision);

                                cell.player_los = los;

#ifndef NDEBUG
                                // Sanity check - if the cell is ONLY blocked by
                                // darkness (i.e. not by a wall or other
                                // blocking terrain), it should NOT be lit
                                if (!los.is_blocked_hard && los.is_blocked_by_dark)
                                {
                                        ASSERT(!map::g_light.at(x, y));
                                }
#endif // NDEBUG
                        }
                }

                fov_hack();
        }

        // The player's current cell is always seen - mostly to update item info
        // while blind (i.e. when you pick up an item you should see it
        // disappear)
        map::g_cells.at(m_pos).is_seen_by_player = true;

        // Cheat vision
        if (init::g_is_cheat_vision_enabled)
        {
                // Show all cells adjacent to cells which can be shot through or
                // seen through
                Array2<bool> reveal(map::dims());

                map_parsers::BlocksProjectiles()
                        .run(reveal, reveal.rect());

                map_parsers::BlocksLos()
                        .run(reveal,
                             reveal.rect(),
                             MapParseMode::append);

                for (auto& reveal_cell : reveal)
                {
                        reveal_cell = !reveal_cell;
                }

                const auto reveal_expanded =
                        map_parsers::expand(reveal, reveal.rect());

                for (size_t i = 0; i < map::g_cells.length(); ++i)
                {
                        if (reveal_expanded.at(i))
                        {
                                map::g_cells.at(i).is_seen_by_player = true;
                        }
                }
        }

        // Explore
        for (int x = 0; x < map::w(); ++x)
        {
                for (int y = 0; y < map::h(); ++y)
                {
                        const P p(x, y);

                        const bool allow_explore =
                                !map::g_dark.at(p) ||
                                map_parsers::BlocksLos().cell(p) ||
                                map_parsers::BlocksWalking(ParseActors::no).cell(p) ||
                                has_darkvision;

                        Cell& cell = map::g_cells.at(p);

                        // Do not explore dark floor cells
                        if (cell.is_seen_by_player && allow_explore)
                        {
                                cell.is_explored = true;
                        }
                }
        }

        minimap::update();
}

void Player::fov_hack()
{
        Array2<bool> blocked_los(map::dims());

        map_parsers::BlocksLos()
                .run(blocked_los, blocked_los.rect());

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::no)
                .run(blocked, blocked.rect());

        const std::vector<terrain::Id> free_terrains = {
                terrain::Id::liquid_deep,
                terrain::Id::chasm
        };

        for (int x = 0; x < blocked.w(); ++x)
        {
                for (int y = 0; y < blocked.h(); ++y)
                {
                        const P p(x, y);

                        if (map_parsers::IsAnyOfTerrains(free_terrains).cell(p))
                        {
                                blocked.at(p) = false;
                        }
                }
        }

        const bool has_darkvision = m_properties.has(PropId::darkvision);

        for (int x = 0; x < map::w(); ++x)
        {
                for (int y = 0; y < map::h(); ++y)
                {
                        if (blocked_los.at(x, y) && blocked.at(x, y))
                        {
                                const P p(x, y);

                                for (const P& d : dir_utils::g_dir_list)
                                {
                                        const P p_adj(p + d);

                                        if (map::is_pos_inside_map(p_adj) &&
                                            map::g_cells.at(p_adj).is_seen_by_player)
                                        {
                                                const bool allow_explore =
                                                        (!map::g_dark.at(p_adj) ||
                                                         map::g_light.at(p_adj) ||
                                                         has_darkvision) &&
                                                        !blocked.at(p_adj);

                                                if (allow_explore)
                                                {
                                                        Cell& cell = map::g_cells.at(x, y);

                                                        cell.is_seen_by_player = true;

                                                        cell.player_los.is_blocked_hard = false;

                                                        break;
                                                }
                                        }
                                }
                        }
                }
        }
}

void Player::update_mon_awareness()
{
        const auto my_seen_actors = seen_actors();

        for (Actor* const actor : my_seen_actors)
        {
                static_cast<Mon*>(actor)->set_player_aware_of_me();
        }
}

bool Player::is_leader_of(const Actor* const actor) const
{
        if (!actor || (actor == this))
        {
                return false;
        }

        // Actor is monster

        return static_cast<const Mon*>(actor)->m_leader == this;
}

bool Player::is_actor_my_leader(const Actor* const actor) const
{
        (void)actor;

        // Should never happen
        ASSERT(false);

        return false;
}

} // actor
