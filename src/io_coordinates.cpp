// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "io.hpp"
#include "io_internal.hpp"

#include "config.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace io {

R gui_to_px_rect(const R rect)
{
        const int gui_cell_px_w = config::gui_cell_px_w();
        const int gui_cell_px_h = config::gui_cell_px_h();

        R px_rect = rect.scaled_up(gui_cell_px_w, gui_cell_px_h);

        // We must include ALL pixels in the given gui area
        px_rect.p1 =
                px_rect.p1.with_offsets(
                        gui_cell_px_w - 1,
                        gui_cell_px_h - 1);

        return px_rect;
}

int gui_to_px_coords_x(const int value)
{
        return value * config::gui_cell_px_w();
}

int gui_to_px_coords_y(const int value)
{
        return value * config::gui_cell_px_h();
}

int map_to_px_coords_x(const int value)
{
        return value * config::map_cell_px_w();
}

int map_to_px_coords_y(const int value)
{
        return value * config::map_cell_px_h();
}

P gui_to_px_coords(const P pos)
{
        return P(gui_to_px_coords_x(pos.x), gui_to_px_coords_y(pos.y));
}

P gui_to_px_coords(const int x, const int y)
{
        return gui_to_px_coords(P(x, y));
}

P map_to_px_coords(const P pos)
{
        return P(map_to_px_coords_x(pos.x), map_to_px_coords_y(pos.y));
}

P map_to_px_coords(const int x, const int y)
{
        return map_to_px_coords(P(x, y));
}

P px_to_gui_coords(const P px_pos)
{
        return {
                px_pos.x / config::gui_cell_px_w(),
                px_pos.y / config::gui_cell_px_h()};
}

P px_to_map_coords(const P px_pos)
{
        return {
                px_pos.x / config::map_cell_px_w(),
                px_pos.y / config::map_cell_px_h()};
}

P gui_to_map_coords(const P gui_pos)
{
        const P px_coords = gui_to_px_coords(gui_pos);

        return px_to_map_coords(px_coords);
}

P gui_to_px_coords(const Panel panel, const P offset)
{
        const P pos = panels::area(panel).p0 + offset;

        return gui_to_px_coords(pos);
}

P map_to_px_coords(const Panel panel, const P offset)
{
        const P px_p0 = gui_to_px_coords(panels::area(panel).p0);

        const P px_offset = map_to_px_coords(offset);

        return px_p0 + px_offset;
}

int panel_px_w(const Panel panel)
{
        return gui_to_px_coords_x(panels::w(panel));
}

int panel_px_h(const Panel panel)
{
        return gui_to_px_coords_y(panels::h(panel));
}

P panel_px_dims(const Panel panel)
{
        return gui_to_px_coords(panels::dims(panel));
}

} // namespace io
