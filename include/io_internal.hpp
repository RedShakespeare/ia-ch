// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef IO_INTERNAL_HPP
#define IO_INTERNAL_HPP

#include "io.hpp"

#include "colors.hpp"
#include "panel.hpp"
#include "pos.hpp"

namespace io {

extern int g_bpp;

inline constexpr size_t g_font_nr_x = 16;
inline constexpr size_t g_font_nr_y = 7;

extern SDL_Window* g_sdl_window;
extern SDL_Renderer* g_sdl_renderer;
extern SDL_Surface* g_screen_srf;
extern SDL_Texture* g_screen_texture;

extern SDL_Surface* g_main_menu_logo_srf;

extern std::vector<P> g_tile_px_data[(size_t)gfx::TileId::END];
extern std::vector<P> g_font_px_data[g_font_nr_x][g_font_nr_y];

extern std::vector<P> g_tile_contour_px_data[(size_t)gfx::TileId::END];
extern std::vector<P> g_font_contour_px_data[g_font_nr_x][g_font_nr_y];

void init_px_manip();

Uint32 px(const SDL_Surface& srf, const int pixel_x, const int pixel_y);

void put_pixels_on_screen(
        const std::vector<P> px_data,
        const P& px_pos,
        const Color& color);

void put_pixels_on_screen(
        const char character,
        const P& px_pos,
        const Color& color);

void put_pixels_on_screen(
        const gfx::TileId tile,
        const P& px_pos,
        const Color& color);

void try_set_window_gui_cells(P new_gui_dims);

void on_window_resized();

bool is_window_maximized();

P sdl_window_gui_dims();

int panel_px_w(Panel panel);
int panel_px_h(Panel panel);
P panel_px_dims(Panel panel);

void draw_text_at_px(
        const std::string& str,
        P px_pos,
        const Color& color,
        const DrawBg draw_bg,
        const Color& bg_color);

} // namespace io

#endif // IO_HPP
