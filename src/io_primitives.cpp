// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "io.hpp"
#include "io_internal.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace io {

void draw_rectangle(const R& px_rect, const Color& color)
{
        draw_rectangle_filled(
                R(px_rect.p0.x,
                  px_rect.p0.y,
                  px_rect.p1.x,
                  px_rect.p0.y),
                color);

        draw_rectangle_filled(
                R(px_rect.p0.x,
                  px_rect.p1.y,
                  px_rect.p1.x,
                  px_rect.p1.y),
                color);

        draw_rectangle_filled(
                R(px_rect.p0.x,
                  px_rect.p0.y,
                  px_rect.p0.x,
                  px_rect.p1.y),
                color);

        draw_rectangle_filled(
                R(px_rect.p1.x,
                  px_rect.p0.y,
                  px_rect.p1.x,
                  px_rect.p1.y),
                color);
}

void draw_rectangle_filled(const R& px_rect, const Color& color)
{
        if (!panels::is_valid()) {
                return;
        }

        SDL_Rect sdl_rect = {
                (Sint16)px_rect.p0.x,
                (Sint16)px_rect.p0.y,
                (Uint16)px_rect.w(),
                (Uint16)px_rect.h()};

        const auto& sdl_color = color.sdl_color();

        SDL_FillRect(
                io::g_screen_srf,
                &sdl_rect,
                SDL_MapRGB(
                        io::g_screen_srf->format,
                        sdl_color.r,
                        sdl_color.g,
                        sdl_color.b));
}

} // namespace io
