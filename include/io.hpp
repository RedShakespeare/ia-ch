// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef IO_HPP
#define IO_HPP

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_video.h"

#include <vector>

#include "colors.hpp"
#include "gfx.hpp"
#include "panel.hpp"


namespace actor
{
class Actor;
} // namespace actor


struct CellRenderData
{
        CellRenderData& operator=(const CellRenderData&) = default;

        TileId tile = TileId::END;
        char character = 0;
        Color color = colors::black();
        Color color_bg = colors::black();
};

struct InputData
{
        int key {-1};
        bool is_shift_held {false};
        bool is_ctrl_held {false};
        bool is_alt_held {false};
};

namespace io
{

void init();
void cleanup();

void update_screen();

void clear_screen();

void on_fullscreen_toggled();

P min_screen_gui_dims();

// Scale from gui/map cell coordinate(s) to pixel coordinate(s)
int gui_to_px_coords_x(int value);
int gui_to_px_coords_y(int value);

int map_to_px_coords_x(int value);
int map_to_px_coords_y(int value);

P gui_to_px_coords(P pos);
P gui_to_px_coords(int x, int y);

P map_to_px_coords(P pos);
P map_to_px_coords(int x, int y);

P px_to_gui_coords(P px_pos);

P px_to_map_coords(P px_pos);

P gui_to_map_coords(P gui_pos);

// Returns a screen pixel position, relative to a cell position in a panel
P gui_to_px_coords(Panel panel, P offset);
P map_to_px_coords(Panel panel, P offset);

void draw_symbol(
        TileId tile,
        char character,
        Panel panel,
        P pos,
        const Color& color,
        bool draw_bg = true,
        const Color& color_bg = colors::black());

void draw_tile(
        TileId tile,
        Panel panel,
        P pos,
        const Color& color,
        bool draw_bg = true,
        const Color& bg_color = colors::black());

void draw_character(
        char character,
        Panel panel,
        P pos,
        const Color& color,
        bool draw_bg = true,
        const Color& bg_color = colors::black());

void draw_text(
        const std::string& str,
        Panel panel,
        P pos,
        const Color& color,
        bool draw_bg = true,
        const Color& bg_color = colors::black());

void draw_text_center(
        const std::string& str,
        Panel panel,
        P pos,
        const Color& color,
        bool draw_bg = true,
        const Color& bg_color = colors::black(),
        bool is_pixel_pos_adj_allowed = true);

void draw_text_right(
        const std::string& str,
        Panel panel,
        P pos,
        const Color& color,
        bool draw_bg = true,
        const Color& bg_color = colors::black());

void cover_cell(Panel panel, P offset);

void cover_panel(
        Panel panel,
        const Color& color = colors::black());

void cover_area(
        Panel panel,
        R area,
        const Color& color = colors::black());

void cover_area(
        Panel panel,
        P offset,
        P dims,
        const Color& color = colors::black());

void draw_rectangle(
        const R& px_rect,
        const Color& color);

void draw_rectangle_filled(
        const R& px_rect,
        const Color& color);

void draw_blast_at_cells(
        const std::vector<P>& positions,
        const Color& color);

void draw_blast_at_seen_cells(
        const std::vector<P>& positions,
        const Color& color);

void draw_blast_at_seen_actors(
        const std::vector<actor::Actor*>& actors,
        const Color& color);

void draw_main_menu_logo();

// TODO: Perhaps add an option to draw a background color inside the box
void draw_box(
        const R& border,
        const Color& color = colors::dark_gray_brown());

// Draws a description "box" for items, spells, etc. The parameter lines may be
// empty, in which case an empty area is drawn.
void draw_descr_box(const std::vector<ColoredString>& lines);

// ----------------------------------------
// TODO: WTF is the difference between these two functions?
void flush_input();
void clear_events();
// ----------------------------------------

InputData get();

} // namespace io

#endif // IO_HPP
