// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "query.hpp"

#include <iostream>

#include "config.hpp"
#include "game_commands.hpp"
#include "io.hpp"
#include "pos.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static bool s_is_inited = false;


// -----------------------------------------------------------------------------
// query
// -----------------------------------------------------------------------------
namespace query
{

void init()
{
        s_is_inited = true;
}

void cleanup()
{
        s_is_inited = false;
}

void wait_for_key_press()
{
        if (s_is_inited && !config::is_bot_playing())
        {
                io::update_screen();

                io::get();
        }
}

BinaryAnswer yes_or_no(
        const char key_for_special_event,
        const AllowSpaceCancel allow_space_cancel)
{
        if (!s_is_inited || config::is_bot_playing())
        {
                return BinaryAnswer::yes;
        }

        io::update_screen();

        InputData input;

        while (true)
        {
                input = io::get();

                const bool is_special_key_pressed =
                        (key_for_special_event != -1) &&
                        (input.key == key_for_special_event);

                const bool is_canceled_with_space =
                        (input.key == SDLK_SPACE) &&
                        (allow_space_cancel == AllowSpaceCancel::yes);

                if ((input.key == 'y') ||
                    (input.key == 'n') ||
                    (input.key == SDLK_ESCAPE) ||
                    is_canceled_with_space ||
                    is_special_key_pressed)
                {
                        break;
                }
        }

        if ((input.key == key_for_special_event) &&
            (key_for_special_event != -1))
        {
                return BinaryAnswer::special;
        }

        return
                (input.key == 'y')
                ? BinaryAnswer::yes
                : BinaryAnswer::no;
}

InputData letter(const bool accept_enter)
{
        InputData input;

        if (!s_is_inited || config::is_bot_playing())
        {
                input.key = 'a';

                return input;
        }

        io::update_screen();

        while (true)
        {
                input = io::get();

                if ((accept_enter && (input.key == SDLK_RETURN)) ||
                    (input.key == SDLK_ESCAPE) ||
                    (input.key == SDLK_SPACE) ||
                    ((input.key >= 'a') && (input.key <= 'z')) ||
                    ((input.key >= 'A') && (input.key <= 'Z')))
                {
                        return input;
                }
        }

        // Unreachable
        return input;
}

int number(
        const P& pos,
        const Color color,
        const int min,
        const int max_nr_digits,
        const int default_value,
        const bool cancel_returns_default)
{
        if (!s_is_inited || config::is_bot_playing())
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
                InputData input;

                while (((input.key < '0') || (input.key > '9')) &&
                       (input.key != SDLK_RETURN) &&
                       (input.key != SDLK_SPACE) &&
                       (input.key != SDLK_ESCAPE) &&
                       (input.key != SDLK_BACKSPACE))
                {
                        input = io::get();

                        // Translate keypad keys to numbers
                        switch (input.key)
                        {
                        case SDLK_KP_1: input.key = '1'; break;
                        case SDLK_KP_2: input.key = '2'; break;
                        case SDLK_KP_3: input.key = '3'; break;
                        case SDLK_KP_4: input.key = '4'; break;
                        case SDLK_KP_5: input.key = '5'; break;
                        case SDLK_KP_6: input.key = '6'; break;
                        case SDLK_KP_7: input.key = '7'; break;
                        case SDLK_KP_8: input.key = '8'; break;
                        case SDLK_KP_9: input.key = '9'; break;
                        case SDLK_KP_0: input.key = '0'; break;
                        default: break;
                        }
                }

                if (input.key == SDLK_RETURN)
                {
                        return std::max(min, ret_num);
                }

                if ((input.key == SDLK_SPACE) || (input.key == SDLK_ESCAPE))
                {
                        return
                                cancel_returns_default
                                ? default_value
                                : -1;
                }

                const std::string ret_num_str = std::to_string(ret_num);

                const int current_num_digits = ret_num_str.size();

                if (input.key == SDLK_BACKSPACE)
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
                        int current_digit = input.key - '0';

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
        if (!s_is_inited || config::is_bot_playing())
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
                        const auto input = io::get();

                        if ((input.key == SDLK_SPACE) ||
                            (input.key == SDLK_ESCAPE) ||
                            (input.key == SDLK_RETURN) ||
                            (input.key == SDLK_TAB))
                        {
                                break;
                        }
                }
        }
}

void wait_for_confirm()
{
        if (!s_is_inited || config::is_bot_playing())
        {
                return;
        }

        io::update_screen();

        while (true)
        {
                const auto input = io::get();

                if ((input.key == SDLK_SPACE) ||
                    (input.key == SDLK_ESCAPE) ||
                    (input.key == SDLK_RETURN))
                {
                        break;
                }
        }
}

Dir dir(const AllowCenter allow_center)
{
        if (!s_is_inited || config::is_bot_playing())
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

} // namespace query
