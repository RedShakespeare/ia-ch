// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "panel.hpp"

#include "io.hpp"

#include "debug.hpp"
#include "rect.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static R panels_[(size_t)Panel::END];

static bool is_valid_;

static void set_panel_area(
        const Panel panel,
        const int x0,
        const int y0,
        const int x1,
        const int y1)
{
        panels_[(size_t)panel] = {x0, y0, x1, y1};
}

static void set_x0(const Panel panel, int x)
{
        R& r =  panels_[(size_t)panel];

        const int w = r.w();

        r.p0.x = x;
        r.p1.x = x + w - 1;
}

static void set_y0(const Panel panel, int y)
{
        R& r =  panels_[(size_t)panel];

        const int h = r.h();

        r.p0.y = y;
        r.p1.y = y + h - 1;
}

static void set_w(const Panel panel, int w)
{
        R& r =  panels_[(size_t)panel];

        r.p1.x = r.p0.x + w - 1;
}

static void finalize_screen_dims()
{
        R& screen = panels_[(size_t)Panel::screen];

        for (const R& panel : panels_)
        {
                screen.p1.x = std::max(screen.p1.x, panel.p1.x);
                screen.p1.y = std::max(screen.p1.y, panel.p1.y);
        }

        TRACE << "Screen GUI size was set to: "
              << panels::w(Panel::screen)
              << ", "
              << panels::h(Panel::screen)
              << std::endl;
}

static void validate_panels(const P max_gui_dims)
{
        TRACE_FUNC_BEGIN;

        is_valid_ = true;

        const R& screen = panels_[(size_t)Panel::screen];

        for (const R& panel : panels_)
        {
                if ((panel.p1.x >= max_gui_dims.x) ||
                    (panel.p1.y >= max_gui_dims.y) ||
                    (panel.p1.x > screen.p1.x) ||
                    (panel.p1.y > screen.p1.y) ||
                    (panel.p0.x > panel.p1.x) ||
                    (panel.p0.y > panel.p1.y))
                {
                        TRACE << "Window too small for panel requiring size: "
                              << panel.p1.x
                              << ", "
                              << panel.p1.y
                              << std::endl
                              << "With position: "
                              << panel.p0.x
                              << ", "
                              << panel.p0.y
                              << std::endl;

                        is_valid_ = false;

                        break;
                }
        }

        // In addition to requirements from individual panels, we also put some
        // requirements on the screen (window) size itself - smaller screen than
        // this is not reasonable to go on with...
        const P min_gui_dims = io::min_screen_gui_dims();

        if ((screen.p1.x + 1) < min_gui_dims.x ||
            (screen.p1.y + 1) < min_gui_dims.y)
        {
                TRACE << "Window too small for static minimum screen limit: "
                      << min_gui_dims.x
                      << ", "
                      << min_gui_dims.y
                      << std::endl;

                is_valid_ = false;
        }

#ifndef NDEBUG
        if (is_valid_)
        {
                TRACE << "Panels OK" << std::endl;
        }
        else
        {
                TRACE << "Panels NOT OK" << std::endl;
        }
#endif // NDDEBUG

        TRACE_FUNC_END;
}

// -----------------------------------------------------------------------------
// panels
// -----------------------------------------------------------------------------
namespace panels
{

void init(const P max_gui_dims)
{
        TRACE_FUNC_BEGIN;

        TRACE << "Maximum allowed GUI size: "
              << max_gui_dims.x
              << ", "
              << max_gui_dims.y
              << std::endl;

        for (auto& panel : panels_)
        {
                panel = R(0, 0, 0, 0);
        }

        // (Finalized later)
        set_panel_area(Panel::player_stats, 0, 0, 21, max_gui_dims.y - 1);

        // (Finalized later)
        set_panel_area(Panel::log_border, 0, 0, 0, 3);

        const int map_panel_w = max_gui_dims.x - w(Panel::player_stats);

        const int map_panel_h = max_gui_dims.y - h(Panel::log_border);

        set_panel_area(
                Panel::map,
                0,
                0,
                map_panel_w - 1,
                map_panel_h - 1);

        set_x0(Panel::player_stats, x1(Panel::map) + 1);

        set_w(Panel::log_border, w(Panel::map));

        set_y0(Panel::log_border, y1(Panel::map) + 1);

        set_panel_area(
                Panel::log,
                x0(Panel::log_border) + 1,
                y0(Panel::log_border) + 1,
                x1(Panel::log_border) - 1,
                y1(Panel::log_border) - 1);

        finalize_screen_dims();

        set_panel_area(
                Panel::create_char_menu,
                1,
                2,
                25,
                y1(Panel::screen));

        set_panel_area(
                Panel::create_char_descr,
                x1(Panel::create_char_menu) + 2,
                2,
                x1(Panel::screen) - 1,
                y1(Panel::screen));

        set_panel_area(
                Panel::item_menu,
                1,
                1,
                x1(Panel::screen) - 36,
                y1(Panel::screen) - 1);

        set_panel_area(
                Panel::item_descr,
                x1(Panel::item_menu) + 2,
                1,
                x1(Panel::screen) - 1,
                y1(Panel::screen) - 1);

        validate_panels(max_gui_dims);

        TRACE_FUNC_END;
}

bool is_valid()
{
        return is_valid_;
}

R area(const Panel panel)
{
        return panels_[(size_t)panel];
}

P dims(const Panel panel)
{
        return area(panel).dims();
}

P p0(const Panel panel)
{
        return area(panel).p0;
}

P p1(const Panel panel)
{
        return area(panel).p1;
}

int x0(const Panel panel)
{
        return area(panel).p0.x;
}

int y0(const Panel panel)
{
        return area(panel).p0.y;
}

int x1(const Panel panel)
{
        return area(panel).p1.x;
}

int y1(const Panel panel)
{
        return area(panel).p1.y;
}

int w(const Panel panel)
{
        return area(panel).w();
}

int h(const Panel panel)
{
        return area(panel).h();
}

P center(const Panel panel)
{
        const P center(
                center_x(panel),
                center_y(panel));

        return center;
}

int center_x(const Panel panel)
{
        return (x1(panel) - x0(panel)) / 2;
}

int center_y(const Panel panel)
{
        return (y1(panel) - y0(panel)) / 2;
}

} // panels
