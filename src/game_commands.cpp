#include "game_commands.hpp"

#include <vector>

#include "actor_factory.hpp"
#include "actor_move.hpp"
#include "actor_player.hpp"
#include "character_descr.hpp"
#include "close.hpp"
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
                states::pop();

                init::cleanup_session();
        }
}

// -----------------------------------------------------------------------------
// game_commands
// -----------------------------------------------------------------------------
namespace game_commands
{

GameCmd to_cmd(const InputData& input)
{
        switch (input.key)
        {

        case SDLK_RIGHT:
        case '6':
                return GameCmd::right;

        case SDLK_DOWN:
        case '2':
                return GameCmd::down;

        case SDLK_LEFT:
        case '4':
                return GameCmd::left;

        case SDLK_UP:
        case '8':
                return GameCmd::up;

        case SDLK_PAGEUP:
        case '9':
                return GameCmd::up_right;

        case SDLK_PAGEDOWN:
        case '3':
                return GameCmd::down_right;

        case SDLK_END:
        case '1':
                return GameCmd::down_left;

        case SDLK_HOME:
        case '7':
                return GameCmd::up_left;

        case '5':
        case '.':
                return GameCmd::wait;

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

        case 'e':
                return GameCmd::auto_move;

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

                // Undefined commands
        default:
                break;

        } // switch

        return GameCmd::undefined;

} // get_cmd

void handle(const GameCmd cmd)
{
        msg_log::clear();

        switch (cmd)
        {

        case GameCmd::undefined:
        {

        }
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
                        msg_log::add(msg_fire_prevent_cmd);
                }
                else if (!map::player->seen_foes().empty())
                {
                        msg_log::add(msg_mon_prevent_cmd);
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

                        states::pop();

                        game::win_game();

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

        case GameCmd::auto_move:
        {
                if (map::player->is_seeing_burning_feature())
                {
                        msg_log::add(msg_fire_prevent_cmd);
                }
                else if (!map::player->seen_foes().empty())
                {
                        msg_log::add(msg_mon_prevent_cmd);
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
                else // We are allowed to use auto-walk
                {
                        msg_log::add("Which direction?" + cancel_info_str,
                                     colors::light_white());

                        const Dir input_dir = query::dir(AllowCenter::no);

                        msg_log::clear();

                        if (input_dir == Dir::END || input_dir == Dir::center)
                        {
                                // Invalid direction
                                io::update_screen();
                        }
                        else // Valid direction
                        {
                                map::player->set_quick_move(input_dir);
                        }

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
