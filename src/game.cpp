// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "game.hpp"

#include "actor_act.hpp"
#include "actor_items.hpp"
#include "actor_player.hpp"
#include "common_text.hpp"
#include "create_character.hpp"
#include "draw_map.hpp"
#include "game_commands.hpp"
#include "game_over.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_builder.hpp"
#include "map_controller.hpp"
#include "map_mode_gui.hpp"
#include "map_travel.hpp"
#include "minimap.hpp"
#include "msg_log.hpp"
#include "popup.hpp"
#include "postmortem.hpp"
#include "property.hpp"
#include "property_factory.hpp"
#include "query.hpp"
#include "saving.hpp"
#include "sdl_base.hpp"
#include "text_format.hpp"
#include "viewport.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static int s_clvl = 0;
static int s_xp_pct = 0;
static int s_xp_accum = 0;
static TimeData s_start_time;

static std::vector<HistoryEvent> s_history_events;

static const std::string s_intro_msg =
        "I stand at the end of a cobbled forest path, before me lies a shunned "
        "and decrepit old church building. This is the access point to the "
        "domains of the abhorred \"Cult of Starry Wisdom\". "
        "I am determined to enter these sprawling catacombs and rob them of "
        "treasures and knowledge. At the depths of the abyss lies my true "
        "destiny, an artifact of non-human origin called \"The shining "
        "Trapezohedron\" - a window to all the secrets of the universe!";

static const std::vector<std::string> s_win_msg = {
        {"As I approach the crystal, an eerie glow illuminates the area. I "
         "notice a figure observing me from the edge of the light. There is no "
         "doubt concerning the nature of this entity; it is the Faceless God "
         "who dwells in the depths of the earth - Nyarlathotep!"},

        {"I panic. Why is it I find myself here, stumbling around in darkness? "
         "Is this all part of a plan? The being beckons me to gaze into the "
         "stone."},

        {"In the radiance I see visions beyond eternity, visions of unreal "
         "reality, visions of the brightest light of day and the darkest night "
         "of madness. There is only onward now, I have to see, I have to KNOW."},

        {"So I make a pact with the Fiend."},

        {"I now harness the shadows that stride from world to world to sow "
         "death and madness. The destinies of all things on earth, living and "
         "dead, are mine."}};

// -----------------------------------------------------------------------------
// game
// -----------------------------------------------------------------------------
namespace game {

void init()
{
        s_clvl = 0;
        s_xp_pct = 0;
        s_xp_accum = 0;

        s_history_events.clear();
}

void save()
{
        saving::put_int(s_clvl);
        saving::put_int(s_xp_pct);
        saving::put_int(s_xp_accum);
        saving::put_int(s_start_time.year);
        saving::put_int(s_start_time.month);
        saving::put_int(s_start_time.day);
        saving::put_int(s_start_time.hour);
        saving::put_int(s_start_time.minute);
        saving::put_int(s_start_time.second);

        saving::put_int(s_history_events.size());

        for (const HistoryEvent& event : s_history_events) {
                saving::put_str(event.msg);
                saving::put_int(event.turn);
        }
}

void load()
{
        s_clvl = saving::get_int();
        s_xp_pct = saving::get_int();
        s_xp_accum = saving::get_int();
        s_start_time.year = saving::get_int();
        s_start_time.month = saving::get_int();
        s_start_time.day = saving::get_int();
        s_start_time.hour = saving::get_int();
        s_start_time.minute = saving::get_int();
        s_start_time.second = saving::get_int();

        const int nr_events = saving::get_int();

        for (int i = 0; i < nr_events; ++i) {
                const std::string msg = saving::get_str();
                const int turn = saving::get_int();

                s_history_events.emplace_back(msg, turn);
        }
}

int clvl()
{
        return s_clvl;
}

int xp_pct()
{
        return s_xp_pct;
}

int xp_accumulated()
{
        return s_xp_accum;
}

TimeData start_time()
{
        return s_start_time;
}

void incr_player_xp(const int xp_gained, const Verbose verbose)
{
        if (!map::g_player->is_alive()) {
                return;
        }

        if (verbose == Verbose::yes) {
                msg_log::add("(+" + std::to_string(xp_gained) + "% XP)");
        }

        s_xp_pct += xp_gained;

        s_xp_accum += xp_gained;

        while (s_xp_pct >= 100) {
                if (s_clvl < g_player_max_clvl) {
                        ++s_clvl;

                        msg_log::add(
                                std::string(
                                        "Welcome to level " +
                                        std::to_string(s_clvl) +
                                        "!"),
                                colors::green(),
                                MsgInterruptPlayer::no,
                                MorePromptOnMsg::yes);

                        msg_log::more_prompt();

                        {
                                const int hp_gained = 1;

                                map::g_player->change_max_hp(
                                        hp_gained,
                                        Verbose::no);

                                map::g_player->restore_hp(
                                        hp_gained,
                                        false,
                                        Verbose::no);
                        }

                        {
                                const int spi_gained = 1;

                                map::g_player->change_max_sp(
                                        spi_gained,
                                        Verbose::no);

                                map::g_player->restore_sp(
                                        spi_gained,
                                        false,
                                        Verbose::no);
                        }

                        player_bon::on_player_gained_lvl(s_clvl);

                        states::push(std::make_unique<PickTraitState>());
                }

                s_xp_pct -= 100;
        }
}

void decr_player_xp(int xp_lost)
{
        // XP should never be reduced below 0% (if this should happen, it is
        // considered to be a bug)
        ASSERT(xp_lost <= s_xp_pct);

        // If XP lost is greater than the current XP, be nice in release mode
        xp_lost = std::min(xp_lost, s_xp_pct);

        s_xp_pct -= xp_lost;
}

void incr_clvl_number()
{
        ++s_clvl;
}

void on_mon_seen(actor::Actor& actor)
{
        auto& d = *actor.m_data;

        if (!d.has_player_seen) {
                d.has_player_seen = true;

                // Give XP based on monster shock rating
                int xp_gained = 0;

                switch (d.mon_shock_lvl) {
                case ShockLvl::unsettling:
                        xp_gained = 3;
                        break;

                case ShockLvl::frightening:
                        xp_gained = 5;
                        break;

                case ShockLvl::terrifying:
                        xp_gained = 10;
                        break;

                case ShockLvl::mind_shattering:
                        xp_gained = 20;
                        break;

                case ShockLvl::none:
                case ShockLvl::END:
                        break;
                }

                if (xp_gained > 0) {
                        const std::string name = actor.name_a();

                        msg_log::add("I have discovered " + name + "!");

                        incr_player_xp(xp_gained);

                        msg_log::more_prompt();

                        add_history_event("Discovered " + name);

                        // We also cause some shock the first time
                        double shock_value =
                                map::g_player->shock_lvl_to_value(
                                        d.mon_shock_lvl);

                        // Dampen the progression a bit
                        shock_value = pow(shock_value, 0.9);

                        map::g_player->incr_shock(
                                shock_value,
                                ShockSrc::see_mon);
                }
        }
}

void on_mon_killed(actor::Actor& actor)
{
        auto& d = *actor.m_data;

        d.nr_kills += 1;

        const int min_hp_for_sadism_bon = 4;

        if (d.hp >= min_hp_for_sadism_bon &&
            insanity::has_sympt(InsSymptId::sadism)) {
                map::g_player->m_shock =
                        std::max(0.0, map::g_player->m_shock - 3.0);
        }

        if (d.is_unique) {
                const std::string name = actor.name_the();

                add_history_event("Defeated " + name);
        }
}

void add_history_event(const std::string msg)
{
        if (saving::is_loading()) {
                // If we are loading the game, never add historic messages (this
                // allows silently running stuff like equip hooks for items)
                return;
        }

        const int turn_nr = game_time::turn_nr();

        s_history_events.emplace_back(msg, turn_nr);
}

const std::vector<HistoryEvent>& history()
{
        return s_history_events;
}

} // namespace game

// -----------------------------------------------------------------------------
// Game state
// -----------------------------------------------------------------------------
StateId GameState::id()
{
        return StateId::game;
}

void GameState::on_start()
{
        if (m_entry_mode == GameEntryMode::new_game) {
                // Character creation may have affected maximum hp and spi
                // (either positively or negatively), so here we need to (re)set
                // the current hp and spi to the maximum values
                map::g_player->m_hp = actor::max_hp(*map::g_player);
                map::g_player->m_sp = actor::max_sp(*map::g_player);

                map::g_player->m_data->ability_values.reset();

                actor_items::make_for_actor(*map::g_player);

                game::add_history_event("Started journey");

                if (!config::is_intro_lvl_skipped() &&
                    !config::is_intro_popup_skipped()) {
                        io::clear_screen();

                        popup::msg(
                                s_intro_msg,
                                "The story so far...",
                                audio::SfxId::END,
                                14);
                }
        }

        if (config::is_intro_lvl_skipped() ||
            (m_entry_mode == GameEntryMode::load_game)) {
                map_travel::go_to_nxt();
        } else {
                const auto map_builder =
                        map_builder::make(MapType::intro_forest);

                map_builder->build();

                draw_map::clear();

                minimap::clear();

                map::update_vision();

                if (map_control::g_controller) {
                        map_control::g_controller->on_start();
                }
        }

        if (config::is_gj_mode() &&
            (m_entry_mode == GameEntryMode::new_game)) {
                // Start with some disadvantages
                auto* const cursed = property_factory::make(PropId::cursed);
                cursed->set_indefinite();

                auto* const diseased = property_factory::make(PropId::diseased);
                diseased->set_indefinite();

                auto* const burning = property_factory::make(PropId::burning);

                map::g_player->m_properties.apply(cursed);
                map::g_player->m_properties.apply(diseased);
                map::g_player->m_properties.apply(burning);
        }

        s_start_time = current_time();
}

void GameState::draw()
{
        if (map::w() == 0) {
                return;
        }

        if (states::is_current_state(*this)) {
#ifndef NDEBUG
                if (!init::g_is_demo_mapgen) {
#endif // NDEBUG
                        viewport::focus_on(map::g_player->m_pos);
#ifndef NDEBUG
                }
#endif // NDEBUG
        }

        draw_map::run();

        map_mode_gui::draw();

        msg_log::draw();
}

void GameState::update()
{
        // To avoid redrawing the map for each actor, we instead run acting
        // inside a loop here. We exit the loop if the next actor is the player.
        // Then another state cycle will be executed, and rendering performed.
        while (true) {
                // Let the current actor act
                auto* actor = game_time::current_actor();

                const bool allow_act = actor->m_properties.allow_act();

                const bool is_gibbed = actor->m_state == ActorState::destroyed;

                if (allow_act && !is_gibbed) {
                        // Tell actor to "do something". If this is the player,
                        // input is read from either the player or the bot. If
                        // it's a monster, the AI handles it.
                        actor::act(*actor);
                } else {
                        // Actor cannot act

                        if (actor->is_player()) {
                                sdl_base::sleep(g_ms_delay_player_unable_act);
                        }

                        game_time::tick();
                }

                // NOTE: This state may have been popped at this point

                // We have quit the current game, or the player is dead?
                if (!map::g_player ||
                    !states::contains_state(StateId::game) ||
                    !map::g_player->is_alive()) {
                        break;
                }

                // Stop if the next actor is the player (to trigger rendering).
                const auto* next_actor = game_time::current_actor();

                if (next_actor->is_player()) {
                        break;
                }
        }

        // Player is dead?
        if (map::g_player && !map::g_player->is_alive()) {
                TRACE << "Player died" << std::endl;

                audio::play(audio::SfxId::death);

                msg_log::add(
                        "-I AM DEAD!-",
                        colors::msg_bad(),
                        MsgInterruptPlayer::no,
                        MorePromptOnMsg::yes);

                saving::erase_save();

                states::pop();

                on_game_over(IsWin::no);

                return;
        }
}

// -----------------------------------------------------------------------------
// Win game state
// -----------------------------------------------------------------------------
void WinGameState::draw()
{
        const int padding = 9;
        const int x0 = padding;
        const int max_w = panels::w(Panel::screen) - (padding * 2);

        int y = 2;

        for (const std::string& section_msg : s_win_msg) {
                const auto section_lines =
                        text_format::split(section_msg, max_w);

                for (const std::string& line : section_lines) {
                        io::draw_text(
                                line,
                                Panel::screen,
                                P(x0, y),
                                colors::white(),
                                io::DrawBg::no,
                                colors::black());

                        ++y;
                }
                ++y;
        }

        ++y;

        const int screen_w = panels::w(Panel::screen);
        const int screen_h = panels::h(Panel::screen);

        io::draw_text_center(
                common_text::g_confirm_hint,
                Panel::screen,
                P((screen_w - 1) / 2, screen_h - 2),
                colors::menu_dark(),
                io::DrawBg::no,
                colors::black(),
                false); // Do not allow pixel-level adjustment
}

void WinGameState::update()
{
        const auto input = io::get();

        switch (input.key) {
        case SDLK_SPACE:
        case SDLK_ESCAPE:
        case SDLK_RETURN:
                states::pop();
                break;

        default:
                break;
        }
}

StateId WinGameState::id()
{
        return StateId::win_game;
}
