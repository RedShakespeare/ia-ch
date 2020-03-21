// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "viewport.hpp"

#include "io.hpp"
#include "panel.hpp"
#include "pos.hpp"
#include "rect.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static P s_p0;

static P get_view_dims()
{
        return io::gui_to_map_coords(panels::dims(Panel::map));
}

// -----------------------------------------------------------------------------
// viewport
// -----------------------------------------------------------------------------
namespace viewport {

R get_map_view_area()
{
        P view_dims = get_view_dims();

        const R map_area(s_p0, s_p0 + view_dims - 1);

        return map_area;
}

void focus_on(const P map_pos)
{
        const P view_dims = get_view_dims();

        s_p0 = map_pos - view_dims.scaled_down(2);
}

bool is_in_view(const P map_pos)
{
        return get_map_view_area().is_pos_inside(map_pos);
}

P to_view_pos(const P map_pos)
{
        return map_pos - s_p0;
}

P to_map_pos(const P view_pos)
{
        return view_pos + s_p0;
}

} // namespace viewport
