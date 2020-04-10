// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "info_screen_state.hpp"

#include "common_text.hpp"
#include "draw_box.hpp"
#include "io.hpp"
#include "panel.hpp"
#include "pos.hpp"
#include "rect.hpp"

int InfoScreenState::max_nr_lines_on_screen() const
{
        return panels::h(Panel::screen) - 2;
}

void InfoScreenState::draw_interface() const
{
        draw_box(panels::area(Panel::screen));

        const int screen_center_x = panels::center_x(Panel::screen);

        io::draw_text_center(
                " " + title() + " ",
                Panel::screen,
                P(screen_center_x, 0),
                colors::title());

        const std::string cmd_info =
                (type() == InfoScreenType::scrolling)
                ? common_text::g_scrollable_info_screen_hint
                : common_text::g_screen_exit_hint;

        io::draw_text_center(
                " " + cmd_info + " ",
                Panel::screen,
                P(screen_center_x, panels::y1(Panel::screen)),
                colors::title());
}
