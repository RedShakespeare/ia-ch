// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "msg_log.hpp"

#include <string>
#include <vector>

#include "actor_player.hpp"
#include "draw_box.hpp"
#include "init.hpp"
#include "io.hpp"
#include "map.hpp"
#include "query.hpp"
#include "saving.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<Msg> s_lines[msg_log::g_nr_log_lines];

static const size_t s_history_cap = 200;

static size_t s_history_size = 0;
static size_t s_history_count = 0;

static Msg s_history[s_history_cap];

static const std::string s_more_str = "-more-";

static const int s_repeat_str_len = 4;

static const int s_space_reserved_for_more_prompt = (int)s_more_str.size() + 1;

static bool s_is_waiting_more_pompt = false;

static size_t find_current_line_nr()
{
        if (s_lines[0].empty())
        {
                return 0;
        }

        size_t line_nr = 1;
        while (true)
        {
                if (s_lines[line_nr].empty())
                {
                        // Empty line found, return previous line number
                        --line_nr;

                        break;
                }

                if (line_nr == (msg_log::g_nr_log_lines - 1))
                {
                        // This is the last line, return this line number
                        break;
                }

                ++line_nr;
        }

        return line_nr;
}

static int x_after_msg(const Msg* const msg)
{
        if (!msg)
        {
                return 0;
        }

        const std::string str = msg->text_with_repeats();

        return msg->x_pos() + (int)str.size() + 1;
}

static int worst_case_msg_w_for_line_nr(
        const int line_nr,
        const std::string& text)
{
        const int space_reserved_for_more_prompt_this_line =
                (line_nr == (msg_log::g_nr_log_lines - 1))
                ? s_space_reserved_for_more_prompt
                : 0;

        const int max_w =
                (int)text.size() +
                s_repeat_str_len +
                space_reserved_for_more_prompt_this_line;

        return max_w;
}

static int msg_area_w_avail_for_text_part()
{
        const int w_avail =
                panels::w(Panel::log) -
                s_repeat_str_len -
                s_space_reserved_for_more_prompt;

        return w_avail;
}

static bool allow_convert_to_frenzied_str(const std::string& str)
{
        bool has_lower_case = false;

        for (auto c : str)
        {
                if ((c >= 'a') && (c <= 'z'))
                {
                        has_lower_case = true;
                        break;
                }
        }

        const char last_msg_char = str.back();

        bool is_ended_by_punctuation =
                (last_msg_char == '.') ||
                (last_msg_char == '!');

        return has_lower_case && is_ended_by_punctuation;
}

static std::string convert_to_frenzied_str(const std::string& str)
{
        // Convert to upper case
        std::string frenzied_str = text_format::to_upper(str);

        // Do not put "!" if string contains "..."
        if (frenzied_str.find("...") == std::string::npos)
        {
                // Change "." to "!" at the end
                if (frenzied_str.back() == '.')
                {
                        frenzied_str.back() = '!';
                }

                // Add some exclamation marks
                frenzied_str += "!!";
        }

        return frenzied_str;
}

static void draw_line(
        const std::vector<Msg>& line,
        const Panel panel,
        const P& pos)
{
        for (const Msg& msg : line)
        {
                io::draw_text(
                        msg.text_with_repeats(),
                        panel,
                        pos.with_x_offset(msg.x_pos()),
                        msg.color(),
                        io::DrawBg::no);
        }
}

static void draw_more_prompt()
{
        int more_x0 = 0;

        size_t line_nr = find_current_line_nr();

        const auto& line = s_lines[line_nr];

        if (!line.empty())
        {
                const Msg& last_msg = line.back();

                more_x0 = x_after_msg(&last_msg);

                // If this is not the last line, the "more" prompt text may be
                // moved to the beginning of the next line if it does not fit on
                // the current line. For the last line however, the "more" text
                // MUST fit on the line (handled when adding messages).
                if (line_nr != (msg_log::g_nr_log_lines - 1))
                {
                        const int more_x1 =
                                more_x0 + (int)s_more_str.size() - 1;

                        if (more_x1 >= panels::w(Panel::log))
                        {
                                more_x0 = 0;
                                ++line_nr;
                        }
                }
        }

        ASSERT(
                (more_x0 + (int)s_more_str.size()) <=
                (panels::w(Panel::log)));

        io::draw_text(
                s_more_str,
                Panel::log,
                {more_x0, (int)line_nr},
                colors::msg_more(),
                io::DrawBg::no);
}

// -----------------------------------------------------------------------------
// msg_log
// -----------------------------------------------------------------------------
namespace msg_log
{
void init()
{
        for (std::vector<Msg>& line : s_lines)
        {
                line.clear();
        }

        s_history_size = 0;
        s_history_count = 0;
        s_is_waiting_more_pompt = false;
}

void draw()
{
        io::cover_panel(
                Panel::log_border,
                colors::extra_dark_gray());

        draw_box(panels::area(Panel::log_border));

        int y = 0;

        for (const auto& line : s_lines)
        {
                if (line.empty())
                {
                        break;
                }

                draw_line(line, Panel::log, {0, y});

                ++y;
        }

        if (s_is_waiting_more_pompt)
        {
                draw_more_prompt();
        }
}

void clear()
{
        for (auto& line : s_lines)
        {
                for (auto& msg : line)
                {
                        if (msg.should_copy_to_history() ==
                            CopyToMsgHistory::no)
                        {
                                continue;
                        }

                        // Add cleared line to history
                        s_history[s_history_count % s_history_cap] = msg;

                        ++s_history_count;

                        if (s_history_size < s_history_cap)
                        {
                                ++s_history_size;
                        }
                }

                line.clear();
        }
}

void add(
        const std::string& str,
        const Color& color,
        const MsgInterruptPlayer interrupt_player,
        const MorePromptOnMsg add_more_prompt_on_msg,
        const CopyToMsgHistory copy_to_history)
{
        ASSERT(!str.empty());

        if (saving::is_loading())
        {
                // If we are loading the game, never print messages (this
                // allows silently running stuff like equip hooks for items)
                return;
        }

        if (str.empty())
        {
                return;
        }

        if (str[0] == ' ')
        {
                TRACE << "Message starts with space: \""
                      << str
                      << "\""
                      << std::endl;

                ASSERT(false);

                return;
        }

        // If frenzied, change the message
        // NOTE: This is done through a recursive call - the reason for this is
        // is to allow the parameter string to be a const reference.
        if (map::g_player->m_properties.has(PropId::frenzied) &&
            allow_convert_to_frenzied_str(str))
        {
                const std::string frenzied_str = convert_to_frenzied_str(str);

                add(
                        frenzied_str,
                        color,
                        interrupt_player,
                        add_more_prompt_on_msg,
                        copy_to_history);

                return;
        }

        // If a single message will not fit on the next empty line in the worst
        // case (i.e. with space reserved for a repetition string, and also for
        // a "more" prompt if the next empty line is the last line), split the
        // message into multiple sub-messages through recursive calls.
        size_t next_empty_line_nr = 0;
        while (true)
        {
                if (s_lines[next_empty_line_nr].empty())
                {
                        break;
                }

                if (next_empty_line_nr == (g_nr_log_lines - 1))
                {
                        // The last line has content - the next empty is the
                        // first line
                        next_empty_line_nr = 0;
                        break;
                }

                ++next_empty_line_nr;
        }

        const bool should_split =
                worst_case_msg_w_for_line_nr(next_empty_line_nr, str) >
                panels::w(Panel::log);

        if (should_split)
        {
                int w_avail = msg_area_w_avail_for_text_part();

                // Since we split the message, we do not have to reserve space
                // for the repeat string (e.g. "x4").
                //
                // NOTE: In theory, it's actually possible that the last message
                // will be repeated - this would happen if another message equal
                // to the last sub-message is added subsequently, e.g.:
                //
                // Message 1  : "a long message foo bar", which is split into ->
                // Message 1a : "a long message"
                // Message 1b : "foo bar"
                // Message 2: : "foo bar"
                //
                // But this seems extremely unlikely in practice...
                //
                w_avail -= s_repeat_str_len;

                const auto lines = text_format::split(str, w_avail);

                for (size_t i = 0; i < lines.size(); ++i)
                {
                        const bool is_last_msg = (i == (lines.size() - 1));

                        // If the message is interrupting, only allow this for
                        // the last line of the split message
                        const auto interrupt_actions_current_line =
                                is_last_msg
                                ? interrupt_player
                                : MsgInterruptPlayer::no;

                        // If a more prompt was requested through the parameter,
                        // only allow this on the last message
                        const auto add_more_prompt_current_line =
                                is_last_msg
                                ? add_more_prompt_on_msg
                                : MorePromptOnMsg::no;

                        add(
                                lines[i],
                                color,
                                interrupt_actions_current_line,
                                add_more_prompt_current_line,
                                copy_to_history);
                }

                return;
        }

        // Find the line number to add the message to
        size_t current_line_nr = find_current_line_nr();

        if ((current_line_nr < (g_nr_log_lines - 1)) &&
            !s_lines[current_line_nr].empty())
        {
                // We are on a non-empty line which is *not* the last line -
                // check if the message will fit, otherwise increment to the
                // next line number
                const int worst_case_w =
                        worst_case_msg_w_for_line_nr(current_line_nr, str);

                const int new_x = x_after_msg(&s_lines[current_line_nr].back());

                const int worst_case_x1 = new_x + worst_case_w - 1;

                if (worst_case_x1 >= panels::x1(Panel::log))
                {
                        ++current_line_nr;
                }
        }

        Msg* prev_msg = nullptr;

        if (!s_lines[current_line_nr].empty())
        {
                prev_msg = &s_lines[current_line_nr].back();
        }

        bool is_repeated = false;

        // Check if message is identical to previous
        if (prev_msg && (add_more_prompt_on_msg == MorePromptOnMsg::no))
        {
                const std::string prev_text = prev_msg->text();

                if (prev_text == str)
                {
                        prev_msg->incr_repeats();

                        is_repeated = true;
                }
        }

        if (!is_repeated)
        {
                int msg_x0 = x_after_msg(prev_msg);

                const int worst_case_msg_w =
                        worst_case_msg_w_for_line_nr(
                                current_line_nr,
                                str);

                const int worst_case_msg_x1 = msg_x0 + worst_case_msg_w - 1;

                if (worst_case_msg_x1 >= panels::w(Panel::log))
                {
                        if (current_line_nr < (g_nr_log_lines - 1))
                        {
                                ++current_line_nr;
                        }
                        else
                        {
                                more_prompt();

                                current_line_nr = 0;
                        }

                        msg_x0 = 0;
                }

                s_lines[current_line_nr]
                        .emplace_back(
                                str,
                                color,
                                msg_x0,
                                copy_to_history);
        }

        io::clear_screen();

        states::draw();

        if (add_more_prompt_on_msg == MorePromptOnMsg::yes)
        {
                more_prompt();
        }

        // Messages may stop long actions like first aid and quick walk
        if (interrupt_player == MsgInterruptPlayer::yes)
        {
                map::g_player->interrupt_actions();
        }

        // Some actions are always interrupted by messages, regardless of the
        // "interrupt_all_player_actions" parameter
        map::g_player->on_log_msg_printed();
}

void more_prompt()
{
        // If the current log is empty, do nothing
        if (s_lines[0].empty())
        {
                return;
        }

        s_is_waiting_more_pompt = true;

        states::draw();

        query::wait_for_msg_more();

        s_is_waiting_more_pompt = false;

        clear();
}

void add_line_to_history(const std::string& line_to_add)
{
        const Msg msg(
                line_to_add,
                colors::white(),
                0,
                CopyToMsgHistory::yes);  // Doesn't matter at this point

        s_history[s_history_count % s_history_cap] = {msg};

        ++s_history_count;

        if (s_history_size < s_history_cap)
        {
                ++s_history_size;
        }
}

std::vector<Msg> history()
{
        std::vector<Msg> result;

        result.reserve(s_history_size);

        size_t start = 0;

        if (s_history_count >= s_history_cap)
        {
                start = s_history_count - s_history_cap;
        }

        for (size_t i = start; i < s_history_count; ++i)
        {
                const auto& msg = s_history[i % s_history_cap];

                result.push_back(msg);
        }

        return result;
}

}  // namespace msg_log

// -----------------------------------------------------------------------------
// Message history state
// -----------------------------------------------------------------------------
StateId MsgHistoryState::id() const
{
        return StateId::message_history;
}

void MsgHistoryState::init_top_btm_line_numbers()
{
        const int history_size = m_history.size();

        const int panel_h = panels::h(Panel::info_screen_content);

        m_top_line_nr = history_size - panel_h;
        m_top_line_nr = std::max(0, m_top_line_nr);

        m_btm_line_nr = m_top_line_nr + panel_h;
        m_btm_line_nr = std::min(history_size - 1, m_btm_line_nr);
}

void MsgHistoryState::on_start()
{
        m_history = msg_log::history();

        init_top_btm_line_numbers();
}

void MsgHistoryState::on_window_resized()
{
        init_top_btm_line_numbers();
}

std::string MsgHistoryState::title() const
{
        std::string title;

        if (m_history.empty())
        {
                title = "No message history";
        }
        else
        {
                // History has content
                const std::string msg_nr_str_first =
                        std::to_string(m_top_line_nr + 1);

                const std::string msg_nr_str_last =
                        std::to_string(m_btm_line_nr + 1);

                title =
                        "Messages " +
                        msg_nr_str_first + "-" +
                        msg_nr_str_last +
                        " of " + std::to_string(m_history.size());
        }

        return title;
}

void MsgHistoryState::draw()
{
        draw_interface();

        int y = 0;

        for (int i = m_top_line_nr; i <= m_btm_line_nr; ++i)
        {
                const auto& msg = m_history[i];

                io::draw_text(
                        msg.text_with_repeats(),
                        Panel::info_screen_content,
                        {0, y},
                        msg.color(),
                        io::DrawBg::no);

                ++y;
        }
}

void MsgHistoryState::update()
{
        const int line_jump = 3;

        const int history_size = m_history.size();

        const auto input = io::get();

        const int panel_h = panels::h(Panel::info_screen_content);

        switch (input.key)
        {
        case SDLK_DOWN:
        case SDLK_KP_2:
        {
                m_top_line_nr += line_jump;

                const int top_nr_max = std::max(0, history_size - panel_h);

                m_top_line_nr = std::min(top_nr_max, m_top_line_nr);
        }
        break;

        case SDLK_UP:
        case SDLK_KP_8:
        {
                m_top_line_nr = std::max(0, m_top_line_nr - line_jump);
        }
        break;

        case SDLK_SPACE:
        case SDLK_ESCAPE:
        {
                states::pop();

                return;
        }
        break;

        default:
                break;
        }

        m_btm_line_nr = std::min(
                m_top_line_nr + panel_h - 1,
                history_size - 1);
}
