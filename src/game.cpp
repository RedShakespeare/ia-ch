#include "game.hpp"

#include "actor_items.hpp"
#include "actor_player.hpp"
#include "create_character.hpp"
#include "draw_map.hpp"
#include "init.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_builder.hpp"
#include "map_controller.hpp"
#include "map_travel.hpp"
#include "minimap.hpp"
#include "msg_log.hpp"
#include "popup.hpp"
#include "postmortem.hpp"
#include "query.hpp"
#include "saving.hpp"
#include "sdl_base.hpp"
#include "status_lines.hpp"
#include "text_format.hpp"
#include "viewport.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static int clvl_ = 0;
static int xp_pct_ = 0;
static int xp_accum_ = 0;
static TimeData start_time_;

static std::vector<HistoryEvent> history_events_;

// -----------------------------------------------------------------------------
// game
// -----------------------------------------------------------------------------
namespace game
{

void init()
{
        clvl_ = 1;
        xp_pct_ = 0;
        xp_accum_ = 0;

        history_events_.clear();
}

void save()
{
        saving::put_int(clvl_);
        saving::put_int(xp_pct_);
        saving::put_int(xp_accum_);
        saving::put_int(start_time_.year_);
        saving::put_int(start_time_.month_);
        saving::put_int(start_time_.day_);
        saving::put_int(start_time_.hour_);
        saving::put_int(start_time_.minute_);
        saving::put_int(start_time_.second_);

        saving::put_int(history_events_.size());

        for (const HistoryEvent& event : history_events_)
        {
                saving::put_str(event.msg);
                saving::put_int(event.turn);
        }
}

void load()
{
        clvl_ = saving::get_int();
        xp_pct_ = saving::get_int();
        xp_accum_ = saving::get_int();
        start_time_.year_ = saving::get_int();
        start_time_.month_ = saving::get_int();
        start_time_.day_ = saving::get_int();
        start_time_.hour_ = saving::get_int();
        start_time_.minute_ = saving::get_int();
        start_time_.second_ = saving::get_int();

        const int nr_events = saving::get_int();

        for (int i = 0; i < nr_events; ++i)
        {
                const std::string msg = saving::get_str();
                const int turn = saving::get_int();

                history_events_.push_back({msg, turn});
        }
}

int clvl()
{
        return clvl_;
}

int xp_pct()
{
        return xp_pct_;
}

int xp_accumulated()
{
        return xp_accum_;
}

TimeData start_time()
{
        return start_time_;
}

void incr_player_xp(const int xp_gained, const Verbosity verbosity)
{
        if (!map::player->is_alive())
        {
                return;
        }

        if (verbosity == Verbosity::verbose)
        {
                msg_log::add("(+" + std::to_string(xp_gained) + "% XP)");
        }

        xp_pct_ += xp_gained;

        xp_accum_ += xp_gained;

        while (xp_pct_ >= 100)
        {
                if (clvl_ < player_max_clvl)
                {
                        ++clvl_;

                        msg_log::add(
                                std::string(
                                        "Welcome to level " +
                                        std::to_string(clvl_) +
                                        "!"),
                                colors::green(),
                                false,
                                MorePromptOnMsg::yes);

                        msg_log::more_prompt();

                        {
                                const int hp_gained = 1;

                                map::player->change_max_hp(
                                        hp_gained,
                                        Verbosity::silent);

                                map::player->restore_hp(
                                        hp_gained,
                                        false,
                                        Verbosity::silent);
                        }

                        {
                                const int spi_gained = 1;

                                map::player->change_max_sp(
                                        spi_gained,
                                        Verbosity::silent);

                                map::player->restore_sp(
                                        spi_gained,
                                        false,
                                        Verbosity::silent);
                        }

                        player_bon::on_player_gained_lvl(clvl_);

                        states::push(std::make_unique<PickTraitState>());
                }

                xp_pct_ -= 100;
        }
}

void incr_clvl()
{
        ++clvl_;
}

void win_game()
{
        io::cover_panel(Panel::screen);
        io::update_screen();

        const std::vector<std::string> win_msg = {
                "As I approach the crystal, an eerie glow illuminates "
                "the area. I notice a figure observing me from the "
                "edge of the light. There is no doubt concerning the "
                "nature of this entity; it is the Faceless God who "
                "dwells in the depths of the earth - Nyarlathotep!",

                "I panic. Why is it I find myself here, stumbling "
                "around in darkness? Is this all part of a plan? The "
                "being beckons me to gaze into the stone.",

                "In the radiance I see visions beyond eternity, "
                "visions of unreal reality, visions of the brightest "
                "light of day and the darkest night of madness. There "
                "is only onward now, I have to see, I have to KNOW.",

                "So I make a pact with the Fiend.",

                "I now harness the shadows that stride from world to "
                "world to sow death and madness. The destinies of all "
                "things on earth, living and dead, are mine."
        };

        const int padding = 9;

        const int x0 = padding;

        const int max_w = panels::w(Panel::screen) - (padding * 2);

        const int line_delay = 50;

        int y = 2;

        for (const std::string& section_msg : win_msg)
        {
                const auto section_lines =
                        text_format::split(section_msg, max_w);

                for (const std::string& line : section_lines)
                {
                        io::draw_text(
                                line,
                                Panel::screen,
                                P(x0, y),
                                colors::white(),
                                false, // Do not draw background color
                                colors::black());

                        io::update_screen();

                        sdl_base::sleep(line_delay);

                        ++y;
                }
                ++y;
        }

        ++y;

        const int screen_w = panels::w(Panel::screen);
        const int screen_h = panels::h(Panel::screen);

        io::draw_text_center(
                "[space/esc/enter] to continue",
                Panel::screen,
                P((screen_w - 1) / 2, screen_h - 2),
                colors::menu_dark(),
                false, // Do not draw background color
                colors::black(),
                false); // Do not allow pixel-level adjustment

        io::update_screen();

        query::wait_for_confirm();
}

void on_mon_seen(Actor& actor)
{
        auto& d = *actor.data;

        if (!d.has_player_seen)
        {
                d.has_player_seen = true;

                // Give XP based on monster shock rating
                int xp_gained = 0;

                switch (d.mon_shock_lvl)
                {
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

                if (xp_gained > 0)
                {
                        const std::string name = actor.name_a();

                        msg_log::add("I have discovered " + name + "!");

                        incr_player_xp(xp_gained);

                        msg_log::more_prompt();

                        add_history_event("Discovered " + name + ".");

                        // We also cause some shock the first time
                        double shock_value =
                                map::player->shock_lvl_to_value(
                                        d.mon_shock_lvl);

                        // Dampen the progression a bit
                        shock_value = pow(shock_value, 0.9);

                        map::player->incr_shock(shock_value, ShockSrc::see_mon);
                }
        }
}

void on_mon_killed(Actor& actor)
{
        ActorData& d = *actor.data;

        d.nr_kills += 1;

        const int min_hp_for_sadism_bon = 4;

        if (d.hp >= min_hp_for_sadism_bon &&
            insanity::has_sympt(InsSymptId::sadism))
        {
                map::player->shock_ = std::max(0.0, map::player->shock_ - 3.0);
        }

        if (d.is_unique)
        {
                const std::string name = actor.name_the();

                add_history_event("Defeated " + name + ".");
        }
}

void add_history_event(const std::string msg)
{
        const int turn_nr = game_time::turn_nr();

        history_events_.push_back({msg, turn_nr});
}

const std::vector<HistoryEvent>& history()
{
        return history_events_;
}

} // game

// -----------------------------------------------------------------------------
// Game state
// -----------------------------------------------------------------------------
StateId GameState::id()
{
        return StateId::game;
}

void GameState::on_start()
{
        if (entry_mode_ == GameEntryMode::new_game)
        {
                // Character creation may have affected maximum hp and spi
                // (either positively or negatively), so here we need to (re)set
                // the current hp and spi to the maximum values
                map::player->hp = actor::max_hp(*map::player);
                map::player->sp = actor::max_sp(*map::player);

                map::player->data->ability_values.reset();

                actor_items::make_for_actor(*map::player);

                game::add_history_event("Started journey");

                if (!config::is_intro_lvl_skipped())
                {
                        io::clear_screen();

                        const std::string msg =
                                "I stand on a cobbled forest path, ahead lies "
                                "a shunned and decrepit old church building. "
                                "This is the access point to the abhorred "
                                "\"Cult of Starry Wisdom\". "
                                "I am determined to enter these sprawling "
                                "catacombs and rob them of treasures and "
                                "knowledge. "
                                "At the depths of the abyss lies my true "
                                "destiny, an artifact of non-human origin "
                                "called \"The shining Trapezohedron\" - a "
                                "window to all the secrets of the universe!";

                        popup::msg(
                                msg,
                                "The story so far...",
                                SfxId::END,
                                5);
                }
        }

        if (config::is_intro_lvl_skipped() ||
            (entry_mode_ == GameEntryMode::load_game))
        {
                map_travel::go_to_nxt();
        }
        else
        {
                const auto map_builder =
                        map_builder::make(MapType::intro_forest);

                map_builder->build();

                draw_map::clear();

                minimap::clear();

                map::update_vision();

                if (map_control::controller)
                {
                        map_control::controller->on_start();
                }
        }

        start_time_ = current_time();
}

void GameState::draw()
{
        if (states::is_current_state(*this))
        {
#ifndef NDEBUG
                if (!init::is_demo_mapgen)
                {
#endif // NDEBUG

                        viewport::focus_on(map::player->pos);

#ifndef NDEBUG
                }
#endif // NDEBUG
        }

        draw_map::run();

        status_lines::draw();

        msg_log::draw();
}

void GameState::update()
{
        // To avoid redrawing the map for each actor, we instead run acting
        // inside a loop here. We exit the loop if the next actor is the player.
        // Then another state cycle will be executed, and rendering performed.
        while (true)
        {
                // Let the current actor act
                Actor* actor = game_time::current_actor();

                const bool allow_act = actor->properties.allow_act();

                const bool is_gibbed = actor->state == ActorState::destroyed;

                if (allow_act && !is_gibbed)
                {
                        // Tell actor to "do something". If this is the player,
                        // input is read from either the player or the bot. If
                        // it's a monster, the AI handles it.
                        actor->act();
                }
                else // Actor cannot act
                {
                        if (actor->is_player())
                        {
                                sdl_base::sleep(ms_delay_player_unable_act);
                        }

                        game_time::tick();
                }

                // NOTE: This state may have been popped at this point

                // We have quit the current game, or the player is dead?
                if (!map::player ||
                    !states::contains_state(StateId::game) ||
                    !map::player->is_alive())
                {
                        break;
                }

                // Stop if the next actor is the player (to trigger rendering).
                const Actor* next_actor = game_time::current_actor();

                if (next_actor->is_player())
                {
                        break;
                }
        }

        // Player is dead?
        if (map::player && !map::player->is_alive())
        {
                TRACE << "Player died" << std::endl;

                static_cast<Player*>(map::player)->wait_turns_left = -1;

                audio::play(SfxId::death);

                msg_log::add("-I AM DEAD!-",
                             colors::msg_bad(),
                             false,
                             MorePromptOnMsg::yes);

                // Go to postmortem menu
                states::pop();

                states::push(std::make_unique<PostmortemMenu>(IsWin::no));

                return;
        }
}
