#include "viewport.hpp"

#include "io.hpp"
#include "panel.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static P p0_;

static P get_view_dims()
{
        return io::gui_to_map_coords(panels::dims(Panel::map));
}

// -----------------------------------------------------------------------------
// viewport
// -----------------------------------------------------------------------------
namespace viewport
{

R get_map_view_area()
{
        P view_dims = get_view_dims();

        const R map_area(p0_, p0_ + view_dims - 1);

        return map_area;
}

void focus_on(const P map_pos)
{
        const P view_dims = get_view_dims();

        p0_ = map_pos - view_dims.scaled_down(2);
}

bool is_in_view(const P map_pos)
{
        return get_map_view_area().is_pos_inside(map_pos);
}

P to_view_pos(const P map_pos)
{
        return map_pos - p0_;
}

P to_map_pos(const P view_pos)
{
        return view_pos + p0_;
}

} // viewport
