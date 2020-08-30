// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef PANEL_HPP
#define PANEL_HPP

struct PxPos;
struct R;
struct P;

enum class Panel {
        screen,
        map,
        map_gui_stats,
        map_gui_stats_border,
        log,
        log_border,
        create_char_menu,
        create_char_descr,
        item_menu,
        item_descr,
        info_screen_content,
        END
};

namespace panels {

void init(P max_gui_dims);

bool is_valid();

R area(Panel panel);

P dims(Panel panel);

P p0(Panel panel);

P p1(Panel panel);

int x0(Panel panel);

int y0(Panel panel);

int x1(Panel panel);

int y1(Panel panel);

int w(Panel panel);

int h(Panel panel);

int center_x(Panel panel);

int center_y(Panel panel);

P center(Panel panel);

} // namespace panels

#endif // PANEL_HPP
