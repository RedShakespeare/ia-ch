#include "query.hpp"

#include <iostream>

#include "config.hpp"
#include "game_commands.hpp"
#include "io.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static bool is_inited = false;

// -----------------------------------------------------------------------------
// query
// -----------------------------------------------------------------------------
namespace query
{

void init()
{
        is_inited = true;
}

void wait_for_key_press()
{
        if (is_inited && !config::is_bot_playing())
        {
                io::update_screen();

                io::get();
        }
}

BinaryAnswer yes_or_no(char key_for_special_event)
{
        if (!is_inited || config::is_bot_playing())
        {
                return BinaryAnswer::yes;
        }

        io::update_screen();

        auto d = io::get();

        while ((d.key != 'y') &&
               (d.key != 'n') &&
               (d.key != SDLK_ESCAPE) &&
               (d.key != SDLK_SPACE) &&
               ((d.key != key_for_special_event) ||
                (key_for_special_event == -1)))
        {
                d = io::get();
        }

        if ((d.key == key_for_special_event) &&
            (key_for_special_event != -1))
        {
                return BinaryAnswer::special;
        }

        if (d.key == 'y')
        {
                return BinaryAnswer::yes;
        }

        return BinaryAnswer::no;
}

InputData letter(const bool accept_enter)
{
        if (!is_inited || config::is_bot_playing())
        {
                return 'a';
        }

        io::update_screen();

        while (true)
        {
                const auto d = io::get();

                if ((accept_enter && (d.key == SDLK_RETURN)) ||
                    (d.key == SDLK_ESCAPE) ||
                    (d.key == SDLK_SPACE) ||
                    ((d.key >= 'a') && (d.key <= 'z')) ||
                    ((d.key >= 'A') && (d.key <= 'Z')))
                {
                        return d;
                }
        }

        return InputData();
}

int number(
        const P& pos,
        const Color color,
        const int min,
        const int max_nr_digits,
        const int default_value,
        const bool cancel_returns_default)
{
        if (!is_inited || config::is_bot_playing())
        {
                return 0;
        }

        int ret_num = std::max(min, default_value);

        io::cover_area(Panel::screen,
                       pos,
                       P(max_nr_digits + 1, 1));

        const std::string str =
                ((ret_num == 0)
                 ? ""
                 : std::to_string(ret_num)) +
                "_";

        io::draw_text(str, Panel::screen, pos, color);

        io::update_screen();

        while (true)
        {
                InputData d;

                while (((d.key < '0') || (d.key > '9')) &&
                       (d.key != SDLK_RETURN) &&
                       (d.key != SDLK_SPACE) &&
                       (d.key != SDLK_ESCAPE) &&
                       (d.key != SDLK_BACKSPACE))
                {
                        d = io::get();
                }

                if (d.key == SDLK_RETURN)
                {
                        return std::max(min, ret_num);
                }

                if ((d.key == SDLK_SPACE) || (d.key == SDLK_ESCAPE))
                {
                        return
                                cancel_returns_default
                                ? default_value
                                : -1;
                }

                const std::string ret_num_str = std::to_string(ret_num);

                const int current_num_digits = ret_num_str.size();

                if (d.key == SDLK_BACKSPACE)
                {
                        ret_num = ret_num / 10;

                        io::cover_area(Panel::screen,
                                       pos,
                                       P(max_nr_digits + 1, 1));

                        io::draw_text(
                                std::string(
                                        ((ret_num == 0)
                                         ? ""
                                         : std::to_string(ret_num)) +
                                        "_"),
                                Panel::screen,
                                pos,
                                color);

                        io::update_screen();
                        continue;
                }

                if (current_num_digits < max_nr_digits)
                {
                        int current_digit = d.key - '0';

                        ret_num = std::max(min, ret_num * 10 + current_digit);

                        io::cover_area(Panel::screen,
                                       pos,
                                       P(max_nr_digits + 1, 1));

                        io::draw_text(
                                std::string(
                                        ((ret_num == 0)
                                         ? ""
                                         : std::to_string(ret_num)) +
                                        "_"),
                                Panel::screen,
                                pos,
                                color);

                        io::update_screen();
                }
        }

        return -1;
}

void wait_for_msg_more()
{
        if (!is_inited || config::is_bot_playing())
        {
                return;
        }

        io::update_screen();

        // Determine criteria for confirming more prompt (decided by config)
        if (config::is_any_key_confirm_more())
        {
                wait_for_key_press();
        }
        else // Only some keys confirm more prompts
        {
                while (true)
                {
                        const auto d = io::get();

                        if ((d.key == SDLK_SPACE) ||
                            (d.key == SDLK_ESCAPE) ||
                            (d.key == SDLK_RETURN) ||
                            (d.key == SDLK_TAB))
                        {
                                break;
                        }
                }
        }
}

void wait_for_confirm()
{
        if (!is_inited || config::is_bot_playing())
        {
                return;
        }

        io::update_screen();

        while (true)
        {
                const auto d = io::get();

                if ((d.key == SDLK_SPACE) ||
                    (d.key == SDLK_ESCAPE) ||
                    (d.key == SDLK_RETURN))
                {
                        break;
                }
        }
}

Dir dir(const AllowCenter allow_center)
{
        if (!is_inited || config::is_bot_playing())
        {
                return Dir::END;
        }

        io::update_screen();

        while (true)
        {
                const auto input = io::get();

                const auto game_cmd = game_commands::to_cmd(input);

                switch (game_cmd)
                {
                case GameCmd::right:
                        return Dir::right;

                case GameCmd::down:
                        return Dir::down;

                case GameCmd::left:
                        return Dir::left;

                case GameCmd::up:
                        return Dir::up;

                case GameCmd::down_right:
                        return Dir::down_right;

                case GameCmd::up_right:
                        return Dir::up_right;

                case GameCmd::down_left:
                        return Dir::down_left;

                case GameCmd::up_left:
                        return Dir::up_left;

                case GameCmd::wait:
                        if (allow_center == AllowCenter::yes)
                        {
                                return Dir::center;
                        }
                        break;

                default:
                        break;
                }

                if ((input.key == SDLK_SPACE) || (input.key == SDLK_ESCAPE))
                {
                        return Dir::END;
                }
        }

        // Unreachable
        return Dir::END;
}

} // query
