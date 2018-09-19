#include "viewport.hpp"

#include "io.hpp"
#include "map.hpp"
#include "panel.hpp"
#include "rl_utils.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static P p0_;

// -----------------------------------------------------------------------------
// viewport
// -----------------------------------------------------------------------------
namespace viewport
{

P get_map_view_dims()
{
        const P gui_coord_dims = panels::dims(Panel::map);

        P map_coord_dims = io::gui_to_map_coords(gui_coord_dims);

        map_coord_dims.x = std::min(map_coord_dims.x, map::w());
        map_coord_dims.y = std::min(map_coord_dims.y, map::h());

        return map_coord_dims;
}

R get_map_view_area()
{
        const P map_view_dims = get_map_view_dims();

        return R(p0_, p0_ + map_view_dims - 1);
}

void focus_on(const P map_pos)
{
        const P map_view_dims = get_map_view_dims();

        p0_.x = map_pos.x - (map_view_dims.x / 2);
        p0_.y = map_pos.y - (map_view_dims.y / 2);
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
