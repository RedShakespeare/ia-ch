// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MSG_LOG_HPP
#define MSG_LOG_HPP

#include <string>
#include <vector>

#include "colors.hpp"
#include "global.hpp"
#include "info_screen_state.hpp"


enum class MorePromptOnMsg
{
        no,
        yes
};

enum class MsgInterruptPlayer
{
        no,
        yes
};

class Msg
{
public:
        Msg(const std::string& text,
            const Color& color_id,
            const int x_pos) :
                m_text(text),
                m_color(color_id),
                m_x_pos(x_pos) {}

        Msg() {}

        std::string text_with_repeats() const
        {
                std::string result_str = m_text;

                if (m_nr_repeats > 1)
                {
                        result_str += m_repeats_str;
                }

                return result_str;
        }

        std::string text() const
        {
                return m_text;
        }

        void incr_repeats()
        {
                ++m_nr_repeats;

                m_repeats_str = "(x" + std::to_string(m_nr_repeats) + ")";
        }

        int x_pos() const
        {
                return m_x_pos;
        }

        Color color() const
        {
                return m_color;
        }

private:
        std::string m_text {""};
        std::string m_repeats_str {""};
        Color m_color {colors::white()};
        int m_nr_repeats {1};
        int m_x_pos {0};
};

namespace msg_log
{

void init();

void draw();

void add(
        const std::string& str,
        const Color& color = colors::text(),
        const MsgInterruptPlayer interrupt_player = MsgInterruptPlayer::no,
        const MorePromptOnMsg add_more_prompt_on_msg = MorePromptOnMsg::no);

// NOTE: This function can safely be called at any time. If there is content in
// the log, a "more" prompt will be run, and the log is cleared. If the log
// happens to be empty, nothing is done.
void more_prompt();

void clear();

void add_line_to_history(const std::string& line_to_add);

const std::vector<Msg> history();

} // log

// -----------------------------------------------------------------------------
// Message history state
// -----------------------------------------------------------------------------
class MsgHistoryState: public InfoScreenState
{
public:
        MsgHistoryState() :
                InfoScreenState() {}

        ~MsgHistoryState() {}

        void on_start() override;

        void draw() override;

        void update() override;

        StateId id() override;

private:
        std::string title() const override;

        InfoScreenType type() const override
        {
                return InfoScreenType::scrolling;
        }

        int m_top_line_nr {0};
        int m_btm_line_nr {0};

        std::vector<Msg> m_history {};
};

#endif // MSG_LOG_HPP
