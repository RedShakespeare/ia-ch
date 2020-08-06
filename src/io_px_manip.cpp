// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "io.hpp"
#include "io_internal.hpp"

#include "debug.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
// Pointer to function used for writing pixels on the screen (there are
// different variants depending on bpp)
static void (*put_px_ptr)(
        const SDL_Surface& srf,
        int pixel_x,
        int pixel_y,
        Uint32 px) = nullptr;

static void put_px8(
        const SDL_Surface& srf,
        const int pixel_x,
        const int pixel_y,
        const Uint32 px)
{
        // p is the address to the pixel we want to set
        Uint8* const p =
                (Uint8*)srf.pixels +
                (pixel_y * srf.pitch) +
                (pixel_x * io::g_bpp);

        *p = px;
}

static void put_px16(
        const SDL_Surface& srf,
        const int pixel_x,
        const int pixel_y,
        const Uint32 px)
{
        // p is the address to the pixel we want to set
        Uint8* const p =
                (Uint8*)srf.pixels +
                (pixel_y * srf.pitch) +
                (pixel_x * io::g_bpp);

        *(Uint16*)p = px;
}

static void put_px24(
        const SDL_Surface& srf,
        const int pixel_x,
        const int pixel_y,
        const Uint32 px)
{
        // p is the address to the pixel we want to set
        Uint8* const p =
                (Uint8*)srf.pixels +
                (pixel_y * srf.pitch) +
                (pixel_x * io::g_bpp);

        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (px >> 16) & 0xff;
                p[1] = (px >> 8) & 0xff;
                p[2] = px & 0xff;
        } else {
                // Little endian
                p[0] = px & 0xff;
                p[1] = (px >> 8) & 0xff;
                p[2] = (px >> 16) & 0xff;
        }
}

static void put_px32(
        const SDL_Surface& srf,
        const int pixel_x,
        const int pixel_y,
        const Uint32 px)
{
        // p is the address to the pixel we want to set
        Uint8* const p =
                (Uint8*)srf.pixels +
                (pixel_y * srf.pitch) +
                (pixel_x * io::g_bpp);

        *(Uint32*)p = px;
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace io {

void init_px_manip()
{
        switch (g_bpp) {
        case 1:
                put_px_ptr = &put_px8;
                break;

        case 2:
                put_px_ptr = &put_px16;
                break;

        case 3:
                put_px_ptr = &put_px24;
                break;

        case 4:
                put_px_ptr = &put_px32;
                break;

        default:
                TRACE_ERROR_RELEASE << "Invalid bpp: " << g_bpp << std::endl;
                PANIC;
        }
}

Uint32 px(const SDL_Surface& srf, const int pixel_x, const int pixel_y)
{
        // 'p' is the address to the pixel we want to retrieve
        Uint8* p =
                (Uint8*)srf.pixels +
                (pixel_y * srf.pitch) +
                (pixel_x * io::g_bpp);

        switch (io::g_bpp) {
        case 1:
                return *p;

        case 2:
                return *(Uint16*)p;

        case 3:
                if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                        return p[0] << 16 | p[1] << 8 | p[2];
                } else {
                        // Little endian
                        return p[0] | p[1] << 8 | p[2] << 16;
                }
                break;

        case 4:
                return *(uint32_t*)p;

        default:
                break;
        }

        return -1;
}

void put_pixels_on_screen(
        const std::vector<P> px_data,
        const P& px_pos,
        const Color& color)
{
        const auto sdl_color = color.sdl_color();

        const int px_color =
                SDL_MapRGB(
                        g_screen_srf->format,
                        sdl_color.r,
                        sdl_color.g,
                        sdl_color.b);

        for (const auto& p_relative : px_data) {
                const int screen_px_x = px_pos.x + p_relative.x;
                const int screen_px_y = px_pos.y + p_relative.y;

                put_px_ptr(*g_screen_srf, screen_px_x, screen_px_y, px_color);
        }
}

void put_pixels_on_screen(
        const char character,
        const P& px_pos,
        const Color& color)
{
        const P sheet_pos(gfx::character_pos(character));

        const auto& pixel_data = g_font_px_data[sheet_pos.x][sheet_pos.y];

        put_pixels_on_screen(pixel_data, px_pos, color);
}

void put_pixels_on_screen(
        const gfx::TileId tile,
        const P& px_pos,
        const Color& color)
{
        const auto& pixel_data = g_tile_px_data[(size_t)tile];

        put_pixels_on_screen(pixel_data, px_pos, color);
}

} // namespace io
