// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "game_commands.hpp"

#include <vector>

#include "actor_factory.hpp"
#include "actor_move.hpp"
#include "actor_player.hpp"
#include "character_descr.hpp"
#include "close.hpp"
#include "common_text.hpp"
#include "config.hpp"
#include "disarm.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "item_factory.hpp"
#include "manual.hpp"
#include "map.hpp"
#include "map_travel.hpp"
#include "marker.hpp"
#include "minimap.hpp"
#include "msg_log.hpp"
#include "pickup.hpp"
#include "player_spells.hpp"
#include "popup.hpp"
#include "postmortem.hpp"
#include "query.hpp"
#include "reload.hpp"
#include "saving.hpp"
#include "state.hpp"
#include "teleport.hpp"
#include "wham.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static void query_quit()
{
        const auto quit_choices = std::vector<std::string> {
                "Yes",
                "No"
        };

        const int quit_choice =
                popup::menu(
                        "Save and highscore are not kept.",
                        quit_choices,
                        "Quit the current game?");

        if (quit_choice == 0)
        {
                // Choosing to quit the game deletes the save
                saving::erase_save();

                states::pop();

                init::cleanup_session();
        }
}

static GameCmd to_cmd_default(const InputData& input)
{
        // When running on windows, with numlock enabled, each numpad key press
        // yields two input events - one for the keypad key, and one as a number
        // key. This seems to be due to a bug in SDL (or maybe it's just how it
        // works on windows). As a workaround, we skip numerical keys here.
        if (input.key >= '0' && input.key <= '9')
        {
                return GameCmd::none;
        }

        switch (input.key)
        {
        case SDLK_KP_6:
                if (input.is_shift_held)
                {
                        return GameCmd::auto_move_right;
                }
                else
                {
                        return GameCmd::right;
                }

        case SDLK_KP_4:
                if (input.is_shift_held)
                {
                        return GameCmd::auto_move_left;
                }
                else
                {
                        return GameCmd::left;
                }

        case SDLK_KP_2:
                if (input.is_shift_held)
                {
                        return GameCmd::auto_move_down;
                }
                else
                {
                        return GameCmd::down;
                }

        case SDLK_KP_8:
                if (input.is_shift_held)
                {
                        return GameCmd::auto_move_up;
                }
                else
                {
                        return GameCmd::up;
                }

        case SDLK_KP_9:
                if (input.is_shift_held)
                {
                        return GameCmd::auto_move_up_right;
                }
                else
                {
                        return GameCmd::up_right;
                }

        case SDLK_KP_3:
                if (input.is_shift_held)
                {
                        return GameCmd::auto_move_down_right;
                }
                else
                {
                        return GameCmd::down_right;
                }

        case SDLK_KP_1:
                if (input.is_shift_held)
                {
                        return GameCmd::auto_move_down_left;
                }
                else
                {
                        return GameCmd::down_left;
                }

        case SDLK_KP_7:
                if (input.is_shift_held)
                {
                        return GameCmd::auto_move_up_left;
                }
                else
                {
                        return GameCmd::up_left;
                }

        case SDLK_KP_5:
        case '.':
                return GameCmd::wait;

        case SDLK_RIGHT:
                if (input.is_shift_held)
                {
                        return GameCmd::up_right;
                }
                else if (input.is_ctrl_held)
                {
                        return GameCmd::down_right;
                }
                else
                {
                        return GameCmd::right;
                }

        case SDLK_LEFT:
                if (input.is_shift_held)
                {
                        return GameCmd::up_left;
                }
                else if (input.is_ctrl_held)
                {
                        return GameCmd::down_left;
                }
                else
                {
                        return GameCmd::left;
                }

        case SDLK_DOWN:
                return GameCmd::down;

        case SDLK_UP:
                return GameCmd::up;

        case 's':
                return GameCmd::wait_long;

        case '?':
        case SDLK_F1:
                return GameCmd::manual;

        case '=':
                return GameCmd::options;

        case 'r':
                return GameCmd::reload;

        case 'k':
                return GameCmd::kick;

        case 'c':
                return GameCmd::close;

        case 'u':
                return GameCmd::unload;

        case 'f':
                return GameCmd::fire;

        case 'g':
                return GameCmd::get;

        case 'i':
                return GameCmd::inventory;

        case 'a':
                return GameCmd::apply_item;

        case 'd':
                return GameCmd::drop_item;

        case 'z':
                return GameCmd::swap_weapon;

        case 't':
                return GameCmd::throw_item;

        case 'l':
                return GameCmd::look;

        case SDLK_TAB:
                return GameCmd::auto_melee;

        case 'x':
                return GameCmd::cast_spell;

        case '@':
                return GameCmd::char_descr;

        case 'm':
                return GameCmd::minimap;

        case 'h':
                return GameCmd::msg_history;

        case 'n':
                return GameCmd::make_noise;

        case 'p':
                return GameCmd::disarm;

        case SDLK_ESCAPE:
                return GameCmd::game_menu;

        case 'Q':
                return GameCmd::quit;

                // Some cheat commands enabled in debug builds
#ifndef NDEBUG
        case SDLK_F2:
                return GameCmd::debug_f2;

        case SDLK_F3:
                return GameCmd::debug_f3;

        case SDLK_F4:
                return GameCmd::debug_f4;

        case SDLK_F5:
                return GameCmd::debug_f5;

        case SDLK_F6:
                return GameCmd::debug_f6;

        case SDLK_F7:
                return GameCmd::debug_f7;

        case SDLK_F8:
                return GameCmd::debug_f8;

        case SDLK_F9:
                return GameCmd::debug_f9;
#endif // NDEBUG

        default:
                // Undefined command
                break;

        } // switch

        return GameCmd::undefined;
} // to_cmd_default

static GameCmd to_cmd_vi(const InputData& input)
{
        // Overriden keys for vi-mode
        switch (input.key)
        {
        case 'l':
                return GameCmd::right;

        case 'L':
                return GameCmd::auto_move_right;

        case 'j':
                return GameCmd::down;

        case 'J':
                return GameCmd::auto_move_down;

        case 'h':
                return GameCmd::left;

        case 'H':
                return GameCmd::auto_move_left;

        case 'k':
                return GameCmd::up;

        case 'K':
                return GameCmd::auto_move_up;

        case 'u':
                return GameCmd::up_right;

        case 'U':
                return GameCmd::auto_move_up_right;

        case 'n':
                return GameCmd::down_right;

        case 'N':
                return GameCmd::auto_move_down_right;

        case 'b':
                return GameCmd::down_left;

        case 'B':
                return GameCmd::auto_move_down_left;

        case 'y':
                return GameCmd::up_left;

        case 'Y':
                return GameCmd::auto_move_up_left;

        case 'M':
                return GameCmd::msg_history;

        case 'w':
                return GameCmd::kick;

        case 'v':
                return GameCmd::look;

        case 'G':
                return GameCmd::unload;

        case 'o':
                return GameCmd::make_noise;

        default:
                break;
        }

        // Input not overriden, delegate to default keys
        return to_cmd_default(input);

} // to_cmd_vi

// -----------------------------------------------------------------------------
// game_commands
// -----------------------------------------------------------------------------
namespace game_commands
{

GameCmd to_cmd(const InputData& input)
{
        switch (config::input_mode())
        {
        case InputMode::standard:
                return to_cmd_default(input);

        case InputMode::vi_keys:
                return to_cmd_vi(input);

        case InputMode::END:
                break;
        }

        PANIC;

        return (GameCmd)0;
}

void handle(const GameCmd cmd)
{
        if (cmd != GameCmd::none)
        {
                msg_log::clear();
        }

        switch (cmd)
        {
        case GameCmd::undefined:
        {
                msg_log::add("Press [?] for help.");
        }
        break;

        case GameCmd::none:
                break;

        case GameCmd::right:
        {
                actor::move(*map::player, Dir::right);
        }
        break;

        case GameCmd::down:
        {
                actor::move(*map::player, Dir::down);
        }
        break;

        case GameCmd::left:
        {
                actor::move(*map::player, Dir::left);
        }
        break;

        case GameCmd::up:
        {
                actor::move(*map::player, Dir::up);
        }
        break;

        case GameCmd::up_right:
        {
                actor::move(*map::player, Dir::up_right);
        }
        break;

        case GameCmd::down_right:
        {
                actor::move(*map::player, Dir::down_right);
        }
        break;

        case GameCmd::down_left:
        {
                actor::move(*map::player, Dir::down_left);
        }
        break;

        case GameCmd::up_left:
        {
                actor::move(*map::player, Dir::up_left);
        }
        break;

        case GameCmd::wait:
        {
                if (player_bon::has_trait(Trait::steady_aimer))
                {
                        Prop* aiming = new PropAiming();

                        aiming->set_duration(1);

                        map::player->properties.apply(aiming);
                }

                actor::move(*map::player, Dir::center);
        }
        break;

        case GameCmd::wait_long:
        {
                if (map::player->is_seeing_burning_feature())
                {
                        msg_log::add(common_text::fire_prevent_cmd);
                }
                else if (!map::player->seen_foes().empty())
                {
                        msg_log::add(common_text::mon_prevent_cmd);
                }
                else // We are allowed to wait
                {
                        // NOTE: We should not print any "wait" message here,
                        // since it would look weird in some cases - e.g. when
                        // the waiting is immediately interrupted by a message
                        // from rearranging pistol magazines.

                        // NOTE: A 'long wait' merely performs "move" into the
                        // center position a number of turns (i.e. the same as
                        // pressing 'wait')
                        const int turns_to_apply = 5;

                        map::player->wait_turns_left = turns_to_apply - 1;

                        game_time::tick();
                }
        }
        break;

        case GameCmd::manual:
        {
                states::push(std::make_unique<BrowseManual>());
        }
        break;

        case GameCmd::options:
        {
                states::push(std::make_unique<ConfigState>());
        }
        break;

        case GameCmd::reload:
        {
                Item* const wpn = map::player->inv.item_in_slot(SlotId::wpn);

                reload::try_reload(*map::player, wpn);
        }
        break;

        case GameCmd::kick:
        {
                wham::run();
        }
        break;

        case GameCmd::close:
        {
                close_door::player_try_close_or_jam();
        }
        break;

        case GameCmd::unload:
        {
                item_pickup::try_unload_wpn_or_pickup_ammo();
        }
        break;

        case GameCmd::fire:
        {
                const bool is_allowed =
                        map::player->properties
                        .allow_attack_ranged(Verbosity::verbose);

                if (is_allowed)
                {
                        auto* const item =
                                map::player->inv.item_in_slot(SlotId::wpn);

                        if (item)
                        {
                                const ItemData& item_data = item->data();

                                if (item_data.ranged.is_ranged_wpn)
                                {
                                        auto* wpn = static_cast<Wpn*>(item);

                                        if ((wpn->ammo_loaded_ >= 1) ||
                                            item_data.ranged.has_infinite_ammo)
                                        {
                                                // Not enough health for Mi-go gun?

                                                // TODO: This doesn't belong here - refactor
                                                if (wpn->data().id == ItemId::mi_go_gun)
                                                {
                                                        if (map::player->hp <= mi_go_gun_hp_drained)
                                                        {
                                                                msg_log::add(
                                                                        "I don't have enough health to fire it.");

                                                                return;
                                                        }
                                                }

                                                states::push(
                                                        std::make_unique<Aiming>(
                                                                map::player->pos, *wpn));
                                        }
                                        // Not enough ammo loaded - auto reload?
                                        else if (config::is_ranged_wpn_auto_reload())
                                        {
                                                reload::try_reload(*map::player, item);
                                        }
                                        else // Not enough ammo loaded, and auto reloading disabled
                                        {
                                                msg_log::add("There is no ammo loaded.");
                                        }
                                }
                                else // Wielded item is not a ranged weapon
                                {
                                        msg_log::add("I am not wielding a firearm.");
                                }
                        }
                        else // Not wielding any item
                        {
                                msg_log::add("I am not wielding a weapon.");
                        }
                }
        }
        break;

        case GameCmd::get:
        {
                const P& p = map::player->pos;

                Item* const item_at_player = map::cells.at(p).item;

                if (item_at_player &&
                    item_at_player->data().id == ItemId::trapez)
                {
                        game::add_history_event(
                                "Beheld The Shining Trapezohedron!");

                        game::win_game();

                        saving::erase_save();

                        states::pop();

                        states::push(
                                std::make_unique<PostmortemMenu>(
                                        IsWin::yes));

                        return;
                }

                item_pickup::try_pick();
        }
        break;

        case GameCmd::inventory:
        {
                states::push(std::make_unique<BrowseInv>());
        }
        break;

        case GameCmd::apply_item:
        {
                states::push(std::make_unique<Apply>());
        }
        break;

        case GameCmd::drop_item:
        {
                states::push(std::make_unique<Drop>());
        }
        break;

        case GameCmd::swap_weapon:
        {
                Item* const wielded = map::player->inv.item_in_slot(SlotId::wpn);

                Item* const alt = map::player->inv.item_in_slot(SlotId::wpn_alt);

                if (wielded || alt)
                {
                        const std::string alt_name_a =
                                alt
                                ? alt->name(ItemRefType::a)
                                : "";

                        // War veteran swaps instantly
                        const bool is_instant = player_bon::bg() == Bg::war_vet;

                        const std::string swift_str =
                                is_instant
                                ? "swiftly "
                                : "";

                        if (wielded)
                        {
                                if (alt)
                                {
                                        msg_log::add(
                                                "I " +
                                                swift_str +
                                                "swap to " +
                                                alt_name_a +
                                                ".");
                                }
                                else // No current alt weapon
                                {
                                        const std::string name =
                                                wielded->name(ItemRefType::plain);

                                        msg_log::add(
                                                "I " +
                                                swift_str +
                                                "put away my " +
                                                name +
                                                ".");
                                }
                        }
                        else // No current wielded item
                        {
                                msg_log::add("I " +
                                             swift_str +
                                             "wield " +
                                             alt_name_a +
                                             ".");
                        }

                        map::player->inv.swap_wielded_and_prepared();

                        if (!is_instant)
                        {
                                game_time::tick();
                        }
                }
                else // No wielded weapon and no alt weapon
                {
                        msg_log::add("I have neither a wielded nor a prepared weapon.");
                }
        }
        break;

        case GameCmd::auto_move_right:
        case GameCmd::auto_move_down:
        case GameCmd::auto_move_left:
        case GameCmd::auto_move_up:
        case GameCmd::auto_move_up_right:
        case GameCmd::auto_move_down_right:
        case GameCmd::auto_move_down_left:
        case GameCmd::auto_move_up_left:
        {
                if (map::player->is_seeing_burning_feature())
                {
                        msg_log::add(common_text::fire_prevent_cmd);
                }
                else if (!map::player->seen_foes().empty())
                {
                        msg_log::add(common_text::mon_prevent_cmd);
                }
                else if (!map::player->properties.allow_see())
                {
                        msg_log::add("Not while blind.");
                }
                else if (map::player->properties.has(PropId::poisoned))
                {
                        msg_log::add("Not while poisoned.");
                }
                else if (map::player->properties.has(PropId::confused))
                {
                        msg_log::add("Not while confused.");
                }
                else // We are allowed to use auto-move
                {
                        auto dir = (Dir)0;

                        switch (cmd)
                        {
                        case GameCmd::auto_move_right:
                                dir = Dir::right;
                                break;

                        case GameCmd::auto_move_down:
                                dir = Dir::down;
                                break;

                        case GameCmd::auto_move_left:
                                dir = Dir::left;
                                break;

                        case GameCmd::auto_move_up:
                                dir = Dir::up;
                                break;

                        case GameCmd::auto_move_up_right:
                                dir = Dir::up_right;
                                break;

                        case GameCmd::auto_move_down_right:
                                dir = Dir::down_right;
                                break;

                        case GameCmd::auto_move_down_left:
                                dir = Dir::down_left;
                                break;

                        case GameCmd::auto_move_up_left:
                                dir = Dir::up_left;
                                break;

                        default:
                                PANIC;
                                break;
                        }

                        map::player->set_auto_move(dir);
                }
        }
        break;

        case GameCmd::throw_item:
        {
                const Item* explosive = map::player->active_explosive_;

                if (explosive)
                {
                        states::push(
                                std::make_unique<ThrowingExplosive>(
                                        map::player->pos, *explosive));
                }
                else // Not holding explosive - run throwing attack instead
                {
                        const bool is_allowed =
                                map::player->properties
                                .allow_attack_ranged(Verbosity::verbose);

                        if (is_allowed)
                        {
                                states::push(std::make_unique<SelectThrow>());
                        }
                }
        }
        break;

        case GameCmd::look:
        {
                if (map::player->properties.allow_see())
                {
                        states::push(std::make_unique<Viewing>(map::player->pos));
                }
                else // Cannot see
                {
                        msg_log::add("I cannot see.");
                }
        }
        break;

        case GameCmd::auto_melee:
        {
                map::player->auto_melee();
        }
        break;

        case GameCmd::cast_spell:
        {
                states::push(std::make_unique<BrowseSpell>());
        }
        break;

        case GameCmd::char_descr:
        {
                states::push(std::make_unique<CharacterDescr>());
        }
        break;

        case GameCmd::minimap:
        {
                states::push(std::make_unique<ViewMinimap>());
        }
        break;

        case GameCmd::msg_history:
        {
                states::push(std::make_unique<MsgHistoryState>());
        }
        break;

        case GameCmd::make_noise:
        {
                if (player_bon::bg() == Bg::ghoul)
                {
                        msg_log::add("I let out a chilling howl.");
                }
                else // Not ghoul
                {
                        msg_log::add("I make some noise.");
                }


                Snd snd("",
                        SfxId::END,
                        IgnoreMsgIfOriginSeen::yes,
                        map::player->pos,
                        map::player,
                        SndVol::low,
                        AlertsMon::yes);

                snd_emit::run(snd);

                game_time::tick();
        }
        break;

        case GameCmd::disarm:
        {
                disarm::player_disarm();
        }
        break;

        case GameCmd::game_menu:
        {
                const auto choices = std::vector<std::string>{
                        "Options",
                        "Tome of Wisdom",
                        "Quit",
                        "Cancel",
                };

                const int choice = popup::menu("", choices);

                if (choice == 0)
                {
                        // Options
                        states::push(std::make_unique<ConfigState>());
                }
                else if (choice == 1)
                {
                        // Manual
                        states::push(std::make_unique<BrowseManual>());
                }
                else if (choice == 2)
                {
                        // Quit
                        query_quit();
                }
        }
        break;

        case GameCmd::quit:
        {
                query_quit();
        }
        break;

        // Some cheat commands enabled in debug builds
#ifndef NDEBUG
        case GameCmd::debug_f2:
        {
                map_travel::go_to_nxt();
        }
        break;

        case GameCmd::debug_f3:
        {
                game::incr_player_xp(100, Verbosity::silent);
        }
        break;

        case GameCmd::debug_f4:
        {
                if (init::is_cheat_vision_enabled)
                {
                        for (auto& cell : map::cells)
                        {
                                cell.is_seen_by_player = false;
                                cell.is_explored = false;
                        }

                        init::is_cheat_vision_enabled = false;
                }
                else // Cheat vision was not enabled
                {
                        init::is_cheat_vision_enabled = true;
                }

                map::player->update_fov();
        }
        break;

        case GameCmd::debug_f5:
        {
                map::player->incr_shock(50, ShockSrc::misc);
        }
        break;

        case GameCmd::debug_f6:
        {
                item_factory::make_item_on_floor(ItemId::gas_mask, map::player->pos);

                for (size_t i = 0; i < (size_t)ItemId::END; ++i)
                {
                        const auto& item_data = item_data::data[i];

                        if (item_data.value != ItemValue::normal && item_data.allow_spawn)
                        {
                                item_factory::make_item_on_floor(ItemId(i), map::player->pos);
                        }
                }
        }
        break;

        case GameCmd::debug_f7:
        {
                teleport(*map::player);
        }
        break;

        case GameCmd::debug_f8:
        {
                map::player->properties.apply(new PropInfected());
        }
        break;

        case GameCmd::debug_f9:
        {
                const std::string query_str = "Summon monster id:";

                io::draw_text(query_str, Panel::screen, P(0, 0), colors::yellow());

                const int idx =
                        query::number(
                                P(query_str.size(), 0),
                                colors::light_white(),
                                0,
                                (int)ActorId::END,
                                0,
                                false);

                const ActorId mon_id = ActorId(idx);

                actor_factory::spawn(map::player->pos, {mon_id}, map::rect());
        }
        break;
#endif // NDEBUG

        } // switch

} // handle

} // game_commands
