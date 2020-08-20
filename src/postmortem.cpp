// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "postmortem.hpp"

#include "SDL.h"

#include "common_text.hpp"
#include "draw_box.hpp"
#include "io.hpp"
#include "panel.hpp"
#include "pos.hpp"

// -----------------------------------------------------------------------------
// Postmortem info
// -----------------------------------------------------------------------------
StateId PostmortemInfo::id() const
{
        return StateId::postmortem_info;
}

void PostmortemInfo::draw()
{
        io::clear_screen();

        draw_box(panels::area(Panel::screen));

        const int screen_center_x = panels::center_x(Panel::screen);

        io::draw_text_center(
                " " + title() + " ",
                Panel::screen,
                P(screen_center_x, 0),
                colors::title());

        const auto command_info =
                common_text::g_scroll_hint +
                " " +
                common_text::g_postmortem_exit_hint;

        io::draw_text_center(
                " " + command_info + " ",
                Panel::screen,
                P(screen_center_x, panels::y1(Panel::screen)),
                colors::title());

        const int nr_lines = (int)m_lines.size();

        int screen_y = 1;

        for (int i = m_top_idx;
             (i < nr_lines) && ((i - m_top_idx) < max_nr_lines_on_screen());
             ++i) {
                const auto& line = m_lines[i];

                io::draw_text(
                        line.str,
                        Panel::screen,
                        P(1, screen_y),
                        line.color);

                ++screen_y;
        }

        io::update_screen();
}

void PostmortemInfo::update()
{
        const int line_jump = 3;

        const int nr_lines = m_lines.size();

        const auto input = io::get();

        switch (input.key) {
        case SDLK_DOWN:
        case SDLK_KP_2:
                m_top_idx += line_jump;

                if (nr_lines <= max_nr_lines_on_screen()) {
                        m_top_idx = 0;
                } else {
                        m_top_idx = std::min(
                                nr_lines - max_nr_lines_on_screen(),
                                m_top_idx);
                }
                break;

        case SDLK_UP:
        case SDLK_KP_8:
                m_top_idx = std::max(0, m_top_idx - line_jump);
                break;

        case SDLK_SPACE:
        case SDLK_ESCAPE:
                // Exit screen
                states::pop();
                break;
        }
}
