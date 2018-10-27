#include "minimap.hpp"

#include <climits>

#include "actor_player.hpp"
#include "common_messages.hpp"
#include "feature_door.hpp"
#include "feature_rigid.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "panel.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
Array2<Color> minimap_(0, 0);

static int get_px_w_per_cell()
{
        // TODO: Make this dynamic, i.e. depend on number of map cells and/or
        // window dimensions
        return 6;
}

static R get_map_area_explored()
{
        // Find the most top left and bottom right map cells explored
        R area_explored(INT_MAX, INT_MAX, 0, 0);

        const P& map_dims = minimap_.dims();

        for (int x = 0; x < map_dims.x; ++x)
        {
                for (int y = 0; y < map_dims.y; ++y)
                {
                        if (map::cells.at(x, y).is_explored)
                        {
                                area_explored.p0.x = std::min(
                                        area_explored.p0.x,
                                        x);

                                area_explored.p0.y = std::min(
                                        area_explored.p0.y,
                                        y);

                                area_explored.p1.x = std::max(
                                        area_explored.p1.x,
                                        x);

                                area_explored.p1.y = std::max(
                                        area_explored.p1.y,
                                        y);
                        }
                }
        }

        return area_explored;
}

static R get_minimap_px_rect_on_screen(const R& map_area_explored)
{
        const int px_w_per_cell = get_px_w_per_cell();

        const P minimap_px_dims =
                map_area_explored
                .dims()
                .scaled_up(px_w_per_cell);

        const P screen_px_dims =
                io::gui_to_px_coords(
                        panels::dims(Panel::screen));

        const P minimap_p0(
                screen_px_dims.x / 2 - minimap_px_dims.x / 2,
                screen_px_dims.y / 2 - minimap_px_dims.y / 2);

        const R px_rect(
                minimap_p0,
                minimap_p0 + minimap_px_dims - 1);

        return px_rect;
}

// -----------------------------------------------------------------------------
// ViewMinimap
// -----------------------------------------------------------------------------
void ViewMinimap::draw()
{
        io::draw_text_center(
                "Viewing minimap " + common_messages::info_screen_tip,
                Panel::screen,
                P(panels::center_x(Panel::screen), 0),
                colors::title());

        const int px_w_per_cell = get_px_w_per_cell();

        const R area_explored = get_map_area_explored();

        const R minimap_px_rect = get_minimap_px_rect_on_screen(area_explored);

        for (int x = area_explored.p0.x; x <= area_explored.p1.x; ++x)
        {
                for (int y = area_explored.p0.y; y <= area_explored.p1.y; ++y)
                {
                        const P pos_relative_to_explored_area =
                                P(x, y) - area_explored.p0;

                        const P px_pos =
                                pos_relative_to_explored_area
                                .scaled_up(px_w_per_cell)
                                .with_offsets(minimap_px_rect.p0);

                        const P px_dims(px_w_per_cell, px_w_per_cell);

                        const R cell_px_rect(px_pos, px_pos + px_dims - 1);

                        Color color;

                        if (map::player->pos == P(x, y))
                        {
                                color = colors::light_green();
                        }
                        else
                        {
                                color = minimap_.at(x, y);
                        }

                        io::draw_rectangle_filled(cell_px_rect, color);
                }
        }
}

void ViewMinimap::update()
{
        const auto input = io::get();

        switch (input.key)
        {
        case SDLK_SPACE:
        case SDLK_ESCAPE:
                // Exit screen
                states::pop();
                break;

        default:
                break;
        }
}

// -----------------------------------------------------------------------------
// minimap
// -----------------------------------------------------------------------------
namespace minimap
{

void clear()
{
        minimap_.resize(0, 0);
}

void update()
{
        if (minimap_.dims() != map::dims())
        {
                minimap_.resize(map::dims());
        }

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::no)
                .run(blocked, blocked.rect());

        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                const auto& map_cell = map::cells.at(i);

                if (!map_cell.is_seen_by_player)
                {
                        continue;
                }

                auto& cell = minimap_.at(i);

                const Rigid* const feature = map_cell.rigid;

                const auto feature_id = feature->id();

                if (map_cell.item)
                {
                        cell = colors::light_magenta();
                }
                else if (feature_id == FeatureId::stairs)
                {
                        cell = colors::yellow();
                }
                else if (feature_id == FeatureId::door &&
                         !static_cast<const Door*>(feature)->is_secret())
                {
                        cell = colors::light_white();
                }
                else if (feature_id == FeatureId::liquid_deep)
                {
                        cell = colors::blue();
                }
                else if (blocked.at(i))
                {
                        cell = colors::sepia();
                }
                else
                {
                        cell = colors::dark_gray_brown();
                }
        }
}

} // minimap
