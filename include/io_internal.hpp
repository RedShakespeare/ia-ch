// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef IO_INTERNAL_HPP
#define IO_INTERNAL_HPP

#include "io.hpp"

#include "colors.hpp"
#include "gfx.hpp"
#include "panel.hpp"
#include "pos.hpp"

namespace io {

inline constexpr size_t g_font_nr_x = 16;
inline constexpr size_t g_font_nr_y = 7;

extern SDL_Window* g_sdl_window;
extern SDL_Renderer* g_sdl_renderer;

// The font texture has a version with black contours, and one without contours
extern SDL_Texture* g_font_texture_with_contours;
extern SDL_Texture* g_font_texture;

// All tile textures have contours
extern SDL_Texture* g_tile_textures[(size_t)gfx::TileId::END];

extern SDL_Texture* g_logo_texture;

// Used for centering the rendering area on the screen
extern P g_rendering_px_offset;

Color read_px_on_surface(const SDL_Surface& surface, const P& px_pos);

void put_px_on_surface(
        SDL_Surface& surface,
        const P& px_pos,
        const Color& color);

void try_set_window_gui_cells(P new_gui_dims);

void on_window_resized();

bool is_window_maximized();

P sdl_window_gui_dims();

int panel_px_w(Panel panel);
int panel_px_h(Panel panel);
P panel_px_dims(Panel panel);

void draw_character_at_px(
        char character,
        P px_pos,
        const Color& color,
        io::DrawBg draw_bg = io::DrawBg::yes,
        const Color& bg_color = {0, 0, 0});

void draw_text_at_px(
        const std::string& str,
        P px_pos,
        const Color& color,
        const DrawBg draw_bg,
        const Color& bg_color);

} // namespace io

#endif // IO_HPP
