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

void draw_rectangle(R px_rect, const Color& color)
{
        px_rect = px_rect.with_offset(g_rendering_px_offset);

        SDL_Rect rect;

        rect.x = px_rect.p0.x;
        rect.y = px_rect.p0.y;
        rect.w = px_rect.w();
        rect.h = px_rect.h();

        SDL_SetRenderDrawColor(
                g_sdl_renderer,
                color.r(),
                color.g(),
                color.b(),
                0xFFu);

        SDL_RenderDrawRect(g_sdl_renderer, &rect);
}

void draw_rectangle_filled(R px_rect, const Color& color)
{
        px_rect = px_rect.with_offset(g_rendering_px_offset);

        SDL_Rect rect;

        rect.x = px_rect.p0.x;
        rect.y = px_rect.p0.y;
        rect.w = px_rect.w();
        rect.h = px_rect.h();

        SDL_SetRenderDrawColor(
                g_sdl_renderer,
                color.r(),
                color.g(),
                color.b(),
                0xFFu);

        SDL_RenderFillRect(g_sdl_renderer, &rect);
}

} // namespace io
