// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "msg_log.hpp"

#include <vector>
#include <string>

#include "init.hpp"
#include "io.hpp"
#include "query.hpp"
#include "actor_player.hpp"
#include "map.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<Msg> s_lines[2];

static const size_t s_history_capacity = 200;

static size_t s_history_size = 0;
static size_t s_history_count = 0;

static std::vector<Msg> s_history[s_history_capacity];

static const std::string s_more_str = "-More-";

static const int s_repeat_str_len = 4;

static const int s_space_reserved_for_more_prompt = s_more_str.size() + 1;

static bool s_is_waiting_more_pompt = false;


static int x_after_msg(const Msg* const msg)
{
        if (!msg)
        {
                return 0;
        }

        const std::string str = msg->text_with_repeats();

        return msg->x_pos() + str.size() + 1;
}

static int worst_case_msg_w_for_line_nr(
        const int line_nr,
        const std::string& text)
{
        const int space_reserved_for_more_prompt_this_line =
                (line_nr == 0)
                ? 0
                : s_space_reserved_for_more_prompt;

        const int max_w =
                (int)text.size()
                + s_repeat_str_len
                + space_reserved_for_more_prompt_this_line;

        return max_w;
}

static int msg_area_w_avail_for_text_part()
{
        const int w_avail =
                panels::w(Panel::log)
                - s_repeat_str_len
                - s_space_reserved_for_more_prompt;

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
        std::string frenzied_str = text_format::all_to_upper(str);

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

// Used by normal log and history viewer
static void draw_line(
        const std::vector<Msg>& line,
        const Panel panel,
        const P& pos)
{
        for (const Msg& msg : line)
        {
                const std::string str = msg.text_with_repeats();

                io::draw_text(
                        str,
                        panel,
                        pos.with_x_offset(msg.x_pos()),
                        msg.color(),
                        false); // Do not draw background
        }
}

static void draw_more_prompt()
{
        int more_x0 = 0;

        int line_nr =
                s_lines[1].empty()
                ? 0
                : 1;

        const auto& line = s_lines[(size_t)line_nr];

        if (!line.empty())
        {
                const Msg& last_msg = line.back();

                more_x0 = x_after_msg(&last_msg);

                // If this is the first line, the "more" prompt text may be
                // moved to the beginning of the second line if it does not fit
                // on the first line. For the second line however, the "more"
                // text MUST fit on the line (handled when adding messages).
                if (line_nr == 0)
                {
                        const int more_x1 =
                                more_x0 + (int)s_more_str.size() - 1;

                        if (more_x1 >= panels::w(Panel::log))
                        {
                                more_x0 = 0;
                                line_nr = 1;
                        }
                }
        }

        ASSERT((more_x0 + (int)s_more_str.size()) <=
               (panels::w(Panel::log)));

        io::draw_text(
                s_more_str,
                Panel::log,
                P(more_x0, line_nr),
                colors::black(),
                true, // Draw background color
                colors::gray());
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

        io::draw_box(panels::area(Panel::log_border));

        const int nr_lines_with_content =
                s_lines[0].empty() ? 0 :
                s_lines[1].empty() ? 1 : 2;

        for (int i = 0; i < nr_lines_with_content; ++i)
        {
                const int y_pos = i;

                const auto& line = s_lines[i];

                draw_line(line, Panel::log, P(0, y_pos));
        }

        if (s_is_waiting_more_pompt)
        {
                draw_more_prompt();
        }
}

void clear()
{
        for (std::vector<Msg>& line : s_lines)
        {
                if (!line.empty())
                {
                        // Add cleared line to history
                        ::s_history[s_history_count % s_history_capacity] =
                                line;

                        ++s_history_count;

                        if (s_history_size < s_history_capacity)
                        {
                                ++s_history_size;
                        }

                        line.clear();
                }
        }
}

void add(const std::string& str,
         const Color& color,
         const bool interrupt_all_player_actions,
         const MorePromptOnMsg add_more_prompt_on_msg)
{
        ASSERT(!str.empty());

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
                const std::string frenzied_str =
                        convert_to_frenzied_str(str);

                add(frenzied_str,
                    color,
                    interrupt_all_player_actions,
                    add_more_prompt_on_msg);

                return;
        }

        // If a single message will not fit on the next empty line in the worst
        // case (i.e. with space reserved for a repetition string, and also for
        // a "more" prompt if the next empty line is the second line), split the
        // message into multiple sub-messages through recursive calls.
        const int next_empty_line_nr =
                (s_lines[0].empty() || !s_lines[1].empty())
                ? 0
                : 1;

        const bool should_split =
                worst_case_msg_w_for_line_nr(
                        next_empty_line_nr,
                        str)
                > panels::w(Panel::log);

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
                //
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
                        const bool interrupt_actions_current_line =
                                is_last_msg
                                ? interrupt_all_player_actions
                                : false;

                        // If a more prompt was requested through the parameter,
                        // only allow this on the last message
                        const auto add_more_prompt_current_line =
                                is_last_msg
                                ? add_more_prompt_on_msg
                                : MorePromptOnMsg::no;

                        add(lines[i],
                            color,
                            interrupt_actions_current_line,
                            add_more_prompt_current_line);
                }

                return;
        }

        int current_line_nr =
                s_lines[1].empty()
                ? 0
                : 1;

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

                if (prev_text.compare(str) == 0)
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
                        if (current_line_nr == 0)
                        {
                                current_line_nr = 1;
                        }
                        else
                        {
                                more_prompt();

                                current_line_nr = 0;
                        }

                        msg_x0 = 0;
                }

                s_lines[current_line_nr].push_back(
                        Msg(str, color, msg_x0));
        }

        io::clear_screen();

        states::draw();

        if (add_more_prompt_on_msg == MorePromptOnMsg::yes)
        {
                more_prompt();
        }

        // Messages may stop long actions like first aid and quick walk
        if (interrupt_all_player_actions)
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
                0);

        ::s_history[s_history_count % s_history_capacity] = {msg};

        ++s_history_count;

        if (s_history_size < s_history_capacity)
        {
                ++s_history_size;
        }
}

const std::vector< std::vector<Msg> > history()
{
        std::vector< std::vector<Msg> > ret;

        ret.reserve(s_history_size);

        size_t start = 0;

        if (s_history_count >= s_history_capacity)
        {
                start = s_history_count - s_history_capacity;
        }

        for (size_t i = start; i < s_history_count; ++i)
        {
                const auto& line = ::s_history[i % s_history_capacity];

                ret.push_back(line);
        }

        return ret;
}

} // msg_log

// -----------------------------------------------------------------------------
// Message history state
// -----------------------------------------------------------------------------
StateId MsgHistoryState::id()
{
        return StateId::message;
}

void MsgHistoryState::on_start()
{
        m_history = msg_log::history();

        const int history_size = m_history.size();

        m_top_line_nr = history_size - max_nr_lines_on_screen();
        m_top_line_nr = std::max(0, m_top_line_nr);

        m_btm_line_nr = m_top_line_nr + max_nr_lines_on_screen();
        m_btm_line_nr = std::min(history_size - 1, m_btm_line_nr);
}

std::string MsgHistoryState::title() const
{
        std::string title = "";

        if (m_history.empty())
        {
                title = "No message history";
        }
        else // History has content
        {
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

        int y = 1;

        for (int i = m_top_line_nr; i <= m_btm_line_nr; ++i)
        {
                const auto& line = m_history[i];

                draw_line(line, Panel::screen, P(0, y));

                ++y;
        }
}

void MsgHistoryState::update()
{
        const int line_jump = 3;

        const int history_size = m_history.size();

        const auto input = io::get();

        switch (input.key)
        {
        case SDLK_DOWN:
        case SDLK_KP_2:
        {
                m_top_line_nr += line_jump;

                const int top_nr_max =
                        std::max(0, history_size - max_nr_lines_on_screen());

                m_top_line_nr =
                        std::min(top_nr_max, m_top_line_nr);
        }
        break;

        case SDLK_UP:
        case SDLK_KP_8:
        {
                m_top_line_nr =
                        std::max(0, m_top_line_nr - line_jump);
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
                m_top_line_nr + max_nr_lines_on_screen() - 1,
                history_size - 1);
}
