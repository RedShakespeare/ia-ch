#include "info_screen_state.hpp"

#include "common_messages.hpp"
#include "io.hpp"
#include "panel.hpp"

int InfoScreenState::max_nr_lines_on_screen() const
{
        return panels::h(Panel::screen) - 2;
}

void InfoScreenState::draw_interface() const
{
        // const int screen_w = panels::w(Panel::screen);
        const int screen_h = panels::h(Panel::screen);

        const int screen_center_x = panels::center_x(Panel::screen);

        // if (config::is_tiles_mode())
        // {
        //         for (int x = 0; x < screen_w; ++x)
        //         {
        //                 io::draw_tile(
        //                         TileId::popup_hor,
        //                         Panel::screen,
        //                         P(x, 0),
        //                         colors::title());

        //                 io::draw_tile(
        //                         TileId::popup_hor,
        //                         Panel::screen,
        //                         P(x, screen_h - 1),
        //                         colors::title());
        //         }
        // }
        // else // Text mode
        // {
        //         const std::string decoration_line(map_w, '-');

        //         io::draw_text(
        //                 decoration_line,
        //                 Panel::screen,
        //                 P(0, 0),
        //                 colors::title());

        //         io::draw_text(
        //                 decoration_line,
        //                 Panel::screen,
        //                 P(0, screen_h - 1),
        //                 colors::title());
        // }

        io::draw_text_center(
                " " + title() + " ",
                Panel::screen,
                P(screen_center_x, 0),
                colors::title());

        const std::string cmd_info =
                (type() == InfoScreenType::scrolling)
                ? common_messages::info_screen_tip_scrollable
                : common_messages::info_screen_tip;

        io::draw_text_center(
                " " + cmd_info + " ",
                Panel::screen,
                P(screen_center_x, screen_h - 1),
                colors::title());
}
