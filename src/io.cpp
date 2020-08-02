// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "io.hpp"

#include <iostream>
#include <vector>

#include "actor.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "attack.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "init.hpp"
#include "inventory.hpp"
#include "item.hpp"
#include "map.hpp"
#include "marker.hpp"
#include "msg_log.hpp"
#include "paths.hpp"
#include "sdl_base.hpp"
#include "text_format.hpp"
#include "version.hpp"
#include "viewport.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static SDL_Window* s_sdl_window = nullptr;
static SDL_Renderer* s_sdl_renderer = nullptr;

// Bytes per pixel
static int s_bpp = -1;

// TODO: This was in global.hpp. Why/where is this used? What is the difference
// between this and 'bpp'???
static const int s_screen_bpp = 32;

static SDL_Surface* s_screen_srf = nullptr;
static SDL_Texture* s_screen_texture = nullptr;

static SDL_Surface* s_main_menu_logo_srf = nullptr;

static const size_t s_font_nr_x = 16;
static const size_t s_font_nr_y = 7;

static std::vector<P> s_tile_px_data[(size_t)gfx::TileId::END];
static std::vector<P> s_font_px_data[s_font_nr_x][s_font_nr_y];

static std::vector<P> s_tile_contour_px_data[(size_t)gfx::TileId::END];
static std::vector<P> s_font_contour_px_data[s_font_nr_x][s_font_nr_y];

static const SDL_Color s_sdl_color_black = {0, 0, 0, 0};

static SDL_Event s_sdl_event;

static void init_screen_surface(const P px_dims)
{
        TRACE << "Initializing screen surface" << std::endl;

        if (s_screen_srf) {
                SDL_FreeSurface(s_screen_srf);
        }

        s_screen_srf =
                SDL_CreateRGBSurface(
                        0,
                        px_dims.x,
                        px_dims.y,
                        s_screen_bpp,
                        0x00FF0000,
                        0x0000FF00,
                        0x000000FF,
                        0xFF000000);

        if (!s_screen_srf) {
                TRACE_ERROR_RELEASE << "Failed to create screen surface"
                                    << std::endl
                                    << SDL_GetError()
                                    << std::endl;

                PANIC;
        }
}

static void init_window(const P px_dims)
{
        TRACE << "Initializing window" << std::endl;

        std::string title = "Infra Arcana ";

        if (version_info::g_version_str.empty()) {
                const auto git_sha1_str =
                        version_info::read_git_sha1_str_from_file();

                title +=
                        "(build " +
                        git_sha1_str +
                        ", " +
                        version_info::g_date_str +
                        ")";
        } else {
                title += version_info::g_version_str;
        }

        if (s_sdl_window) {
                SDL_DestroyWindow(s_sdl_window);
        }

        if (config::is_fullscreen()) {
                TRACE << "Fullscreen mode" << std::endl;

                s_sdl_window = SDL_CreateWindow(
                        title.c_str(),
                        SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED,
                        px_dims.x,
                        px_dims.y,
                        SDL_WINDOW_FULLSCREEN_DESKTOP);
        } else {
                // Windowed mode
                TRACE << "Windowed mode" << std::endl;

                s_sdl_window = SDL_CreateWindow(
                        title.c_str(),
                        SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED,
                        px_dims.x,
                        px_dims.y,
                        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        }

        if (!s_sdl_window) {
                TRACE << "Failed to create window"
                      << std::endl
                      << SDL_GetError()
                      << std::endl;
        }
}

static void init_renderer(const P px_dims)
{
        TRACE << "Initializing renderer" << std::endl;

        if (s_sdl_renderer) {
                SDL_DestroyRenderer(s_sdl_renderer);
        }

        s_sdl_renderer =
                SDL_CreateRenderer(
                        s_sdl_window,
                        -1,
                        SDL_RENDERER_SOFTWARE);

        if (!s_sdl_renderer) {
                TRACE << "Failed to create SDL renderer"
                      << std::endl
                      << SDL_GetError()
                      << std::endl;

                PANIC;
        }

        SDL_RenderSetLogicalSize(s_sdl_renderer, px_dims.x, px_dims.y);

        // SDL_RenderSetIntegerScale(s_sdl_renderer_, SDL_TRUE);

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
}

static void init_screen_texture(const P px_dims)
{
        TRACE << "Initializing screen texture" << std::endl;

        if (s_screen_texture) {
                SDL_DestroyTexture(s_screen_texture);
        }

        s_screen_texture =
                SDL_CreateTexture(
                        s_sdl_renderer,
                        SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING,
                        px_dims.x,
                        px_dims.y);

        if (!s_screen_texture) {
                TRACE_ERROR_RELEASE << "Failed to create screen texture"
                                    << std::endl
                                    << SDL_GetError()
                                    << std::endl;

                PANIC;
        }
}

static P native_resolution_from_sdl()
{
        SDL_DisplayMode dm;

        auto result = SDL_GetDesktopDisplayMode(0, &dm);

        if (result != 0) {
                TRACE_ERROR_RELEASE << "Failed to read native resolution"
                                    << std::endl
                                    << SDL_GetError()
                                    << std::endl;

                PANIC;
        }

        return P(dm.w, dm.h);
}

static bool is_window_maximized()
{
        // TODO: This does not seem to work very well:
        // * The flag is sometimes not set when maximizing the window
        // * The flag sometimes gets "stuck" after the window is restored from
        //   being maximized. The flag is only cleared again after tabbing to
        //   another window and back again, or after minimizing and restoring
        //   the window
        //
        // Is this an SDL bug?
        //
        return SDL_GetWindowFlags(s_sdl_window) & SDL_WINDOW_MAXIMIZED;
}

// Pointer to function used for writing pixels on the screen (there are
// different variants depending on bpp)
static void (*put_px_ptr)(
        const SDL_Surface& srf,
        int pixel_x,
        int pixel_y,
        Uint32 px) = nullptr;

static Uint32 px(const SDL_Surface& srf, const int pixel_x, const int pixel_y)
{
        // 'p' is the address to the pixel we want to retrieve
        Uint8* p =
                (Uint8*)srf.pixels +
                (pixel_y * srf.pitch) +
                (pixel_x * s_bpp);

        switch (s_bpp) {
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
                (pixel_x * s_bpp);

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
                (pixel_x * s_bpp);

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
                (pixel_x * s_bpp);

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
                (pixel_x * s_bpp);

        *(Uint32*)p = px;
}

static void blit_surface(SDL_Surface& srf, const P px_pos)
{
        SDL_Rect dst_rect;

        dst_rect.x = px_pos.x;
        dst_rect.y = px_pos.y;
        dst_rect.w = srf.w;
        dst_rect.h = srf.h;

        SDL_BlitSurface(&srf, nullptr, s_screen_srf, &dst_rect);
}

static void load_contour(
        const std::vector<P>& source_px_data,
        std::vector<P>& dest_px_data,
        const P cell_px_dims)
{
        for (const P source_px_pos : source_px_data) {
                const int size = 1;

                const int px_x0 =
                        std::max(0, source_px_pos.x - size);

                const int px_y0 =
                        std::max(0, source_px_pos.y - size);

                const int px_x1 =
                        std::min(cell_px_dims.x - 1, source_px_pos.x + size);

                const int px_y1 =
                        std::min(cell_px_dims.y - 1, source_px_pos.y + size);

                for (int px_x = px_x0; px_x <= px_x1; ++px_x) {
                        for (int px_y = px_y0; px_y <= px_y1; ++px_y) {
                                const P p(px_x, px_y);

                                // Only mark pixel as contour if it's not marked
                                // on the source
                                const auto it =
                                        std::find(
                                                std::begin(source_px_data),
                                                std::end(source_px_data),
                                                p);

                                if (it == end(source_px_data)) {
                                        dest_px_data.push_back(p);
                                }
                        }
                }
        }
}

static void load_images()
{
        TRACE_FUNC_BEGIN;

        // Main menu logo
        SDL_Surface* tmp_srf = IMG_Load(paths::logo_img_path().c_str());

        ASSERT(tmp_srf && "Failed to load main menu logo image");

        s_main_menu_logo_srf =
                SDL_ConvertSurface(tmp_srf, s_screen_srf->format, 0);

        SDL_FreeSurface(tmp_srf);

        TRACE_FUNC_END;
}

static void load_font()
{
        TRACE_FUNC_BEGIN;

        const std::string font_path =
                paths::fonts_dir() +
                config::font_name();

        SDL_Surface* const font_srf_tmp = IMG_Load(font_path.c_str());

        if (!font_srf_tmp) {
                TRACE << "Failed to load font: " << IMG_GetError() << std::endl;

                PANIC;
        }

        const Uint32 img_color = SDL_MapRGB(
                font_srf_tmp->format,
                255,
                255,
                255);

        const P cell_px_dims(
                config::gui_cell_px_w(),
                config::gui_cell_px_h());

        for (int x = 0; x < (int)s_font_nr_x; ++x) {
                for (int y = 0; y < (int)s_font_nr_y; ++y) {
                        auto& px_data = s_font_px_data[x][y];

                        px_data.clear();

                        const int sheet_x0 = x * cell_px_dims.x;

                        const int sheet_y0 = y * cell_px_dims.y;

                        const int sheet_x1 = sheet_x0 + cell_px_dims.x - 1;

                        const int sheet_y1 = sheet_y0 + cell_px_dims.y - 1;

                        for (int sheet_x = sheet_x0;
                             sheet_x <= sheet_x1;
                             ++sheet_x) {
                                for (int sheet_y = sheet_y0;
                                     sheet_y <= sheet_y1;
                                     ++sheet_y) {
                                        const auto current_px =
                                                px(*font_srf_tmp,
                                                   sheet_x,
                                                   sheet_y);

                                        const bool is_img_px =
                                                current_px == img_color;

                                        if (is_img_px) {
                                                const int x_relative =
                                                        sheet_x - sheet_x0;

                                                const int y_relative =
                                                        sheet_y - sheet_y0;

                                                px_data.emplace_back(
                                                        x_relative,
                                                        y_relative);
                                        }
                                }
                        }

                        load_contour(
                                px_data,
                                s_font_contour_px_data[x][y],
                                cell_px_dims);
                }
        }

        SDL_FreeSurface(font_srf_tmp);

        TRACE_FUNC_END;
}

static void load_tiles()
{
        TRACE_FUNC_BEGIN;

        const P cell_px_dims(
                config::map_cell_px_w(),
                config::map_cell_px_h());

        for (size_t i = 0; i < (size_t)gfx::TileId::END; ++i) {
                const auto id = (gfx::TileId)i;

                const std::string img_name = gfx::tile_id_to_str(id);

                const std::string img_path =
                        paths::tiles_dir() +
                        img_name +
                        ".png";

                SDL_Surface* const tile_srf_tmp = IMG_Load(img_path.c_str());

                if (!tile_srf_tmp) {
                        TRACE_ERROR_RELEASE
                                << "Could not load tile image, at: "
                                << img_path
                                << std::endl;

                        PANIC;
                }

                // Verify width and height of loaded image
                if ((tile_srf_tmp->w != cell_px_dims.x) ||
                    (tile_srf_tmp->h != cell_px_dims.y)) {
                        TRACE_ERROR_RELEASE
                                << "Tile image at \""
                                << img_path
                                << "\" has wrong size: "
                                << tile_srf_tmp->w
                                << "x"
                                << tile_srf_tmp->h
                                << ", expected: "
                                << cell_px_dims.x
                                << "x"
                                << cell_px_dims.y
                                << std::endl;

                        PANIC;
                }

                const Uint32 img_color =
                        SDL_MapRGB(tile_srf_tmp->format, 255, 255, 255);

                auto& px_data = s_tile_px_data[i];

                px_data.clear();

                for (int x = 0; x < cell_px_dims.x; ++x) {
                        for (int y = 0; y < cell_px_dims.y; ++y) {
                                const auto current_px =
                                        px(*tile_srf_tmp, x, y);

                                const bool is_img_px =
                                        current_px == img_color;

                                if (is_img_px) {
                                        px_data.emplace_back(x, y);
                                }
                        }
                }

                load_contour(
                        px_data,
                        s_tile_contour_px_data[i],
                        cell_px_dims);

                SDL_FreeSurface(tile_srf_tmp);
        }

        TRACE_FUNC_END;
}

static void put_pixels_on_screen(
        const std::vector<P> px_data,
        const P px_pos,
        const Color& color)
{
        const auto sdl_color = color.sdl_color();

        const int px_color =
                SDL_MapRGB(
                        s_screen_srf->format,
                        sdl_color.r,
                        sdl_color.g,
                        sdl_color.b);

        for (const auto p_relative : px_data) {
                const int screen_px_x = px_pos.x + p_relative.x;
                const int screen_px_y = px_pos.y + p_relative.y;

                put_px_ptr(
                        *s_screen_srf,
                        screen_px_x,
                        screen_px_y,
                        px_color);
        }
}

static void put_pixels_on_screen(
        const gfx::TileId tile,
        const P px_pos,
        const Color& color)
{
        const auto& pixel_data = s_tile_px_data[(size_t)tile];

        put_pixels_on_screen(pixel_data, px_pos, color);
}

static void put_pixels_on_screen(
        const char character,
        const P px_pos,
        const Color& color)
{
        const P sheet_pos(gfx::character_pos(character));

        const auto& pixel_data = s_font_px_data[sheet_pos.x][sheet_pos.y];

        put_pixels_on_screen(pixel_data, px_pos, color);
}

static void draw_character_at_px(
        const char character,
        const P px_pos,
        const Color& color,
        const io::DrawBg draw_bg = io::DrawBg::yes,
        const Color& bg_color = Color(0, 0, 0))
{
        if (draw_bg == io::DrawBg::yes) {
                const P cell_dims(
                        config::gui_cell_px_w(),
                        config::gui_cell_px_h());

                io::draw_rectangle_filled(
                        {px_pos, px_pos + cell_dims - 1},
                        bg_color);
        }

        // Draw contour if neither foreground/background is black
        if ((color != colors::black()) &&
            (bg_color != colors::black())) {
                const P char_pos(gfx::character_pos(character));

                const auto& contour_px_data =
                        s_font_contour_px_data[char_pos.x][char_pos.y];

                put_pixels_on_screen(
                        contour_px_data,
                        px_pos,
                        s_sdl_color_black);
        }

        put_pixels_on_screen(
                character,
                px_pos,
                color);
}

static int panel_px_w(const Panel panel)
{
        return io::gui_to_px_coords_x(panels::w(panel));
}

static int panel_px_h(const Panel panel)
{
        return io::gui_to_px_coords_y(panels::h(panel));
}

static P panel_px_dims(const Panel panel)
{
        return io::gui_to_px_coords(panels::dims(panel));
}

static void draw_text_at_px(
        const std::string& str,
        P px_pos,
        const Color& color,
        const io::DrawBg draw_bg,
        const Color& bg_color)
{
        if ((px_pos.y < 0) || (px_pos.y >= panel_px_h(Panel::screen))) {
                return;
        }

        const int cell_px_w = config::gui_cell_px_w();
        const int msg_w = str.size();
        const int msg_px_w = msg_w * cell_px_w;

        const auto sdl_color = color.sdl_color();

        const auto sdl_bg_color = bg_color.sdl_color();

        const auto sdl_color_gray = colors::gray();

        const int screen_px_w = panel_px_w(Panel::screen);
        const int msg_px_x1 = px_pos.x + msg_px_w - 1;
        const bool msg_w_fit_on_screen = msg_px_x1 < screen_px_w;

        // X position to start drawing dots instead when the message does not
        // fit on the screen horizontally.
        const int px_x_dots = screen_px_w - (cell_px_w * 3);

        for (int i = 0; i < msg_w; ++i) {
                if (px_pos.x < 0 || px_pos.x >= screen_px_w) {
                        return;
                }

                const bool draw_dots =
                        !msg_w_fit_on_screen &&
                        (px_pos.x >= px_x_dots);

                if (draw_dots) {
                        draw_character_at_px(
                                '.',
                                px_pos,
                                sdl_color_gray,
                                draw_bg,
                                bg_color);
                } else {
                        // Whole message fits, or we are not yet near the edge
                        draw_character_at_px(
                                str[i],
                                px_pos,
                                sdl_color,
                                draw_bg,
                                sdl_bg_color);
                }

                px_pos.x += cell_px_w;
        }
}

static P sdl_window_px_dims()
{
        P px_dims;

        SDL_GetWindowSize(
                s_sdl_window,
                &px_dims.x,
                &px_dims.y);

        return px_dims;
}

static P sdl_window_gui_dims()
{
        const P px_dims = sdl_window_px_dims();

        return io::px_to_gui_coords(px_dims);
}

static void try_set_window_gui_cells(P new_gui_dims)
{
        const P min_gui_dims = io::min_screen_gui_dims();

        new_gui_dims.x = std::max(new_gui_dims.x, min_gui_dims.x);
        new_gui_dims.y = std::max(new_gui_dims.y, min_gui_dims.y);

        const P new_px_dims = io::gui_to_px_coords(new_gui_dims);

        SDL_SetWindowSize(
                s_sdl_window,
                new_px_dims.x,
                new_px_dims.y);
}

static void on_window_resized()
{
        P new_px_dims = sdl_window_px_dims();

        config::set_screen_px_w(new_px_dims.x);
        config::set_screen_px_h(new_px_dims.y);

        TRACE << "New window size: "
              << new_px_dims.x
              << ", "
              << new_px_dims.y
              << std::endl;

        panels::init(io::px_to_gui_coords(new_px_dims));

        init_renderer(new_px_dims);

        init_screen_surface(new_px_dims);

        init_screen_texture(new_px_dims);

        states::on_window_resized();
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace io {

void init()
{
        TRACE_FUNC_BEGIN;

        cleanup();

        P screen_px_dims;

        if (config::is_fullscreen()) {
                const P resolution =
                        config::is_native_resolution_fullscreen()
                        ? native_resolution_from_sdl()
                        : io::gui_to_px_coords(io::min_screen_gui_dims());

                panels::init(io::px_to_gui_coords(resolution));

                screen_px_dims = panel_px_dims(Panel::screen);

                init_window(screen_px_dims);

                if (!s_sdl_window) {
                        // Fullscreen failed, fall back on windowed mode instead
                        config::set_fullscreen(false);
                }
        }

        // NOTE: Fullscreen may have been disabled while attempting to set up a
        // fullscreen "window" (see above), so we check again here if fullscreen
        // is enabled
        if (!config::is_fullscreen()) {
                const auto min_gui_dims = io::min_screen_gui_dims();

                const auto config_res =
                        P(config::screen_px_w(),
                          config::screen_px_h());

                const auto config_gui_dims = px_to_gui_coords(config_res);

                const auto native_res = native_resolution_from_sdl();

                TRACE << "Minimum required GUI dimensions: "
                      << min_gui_dims.x
                      << ","
                      << min_gui_dims.y
                      << std::endl;

                TRACE << "Config GUI dimensions: "
                      << config_gui_dims.x
                      << ","
                      << config_gui_dims.y
                      << std::endl;

                TRACE << "Config resolution: "
                      << config_res.x
                      << ","
                      << config_res.y
                      << std::endl;

                TRACE << "Native resolution: "
                      << native_res.x
                      << ","
                      << native_res.y
                      << std::endl;

                const auto screen_gui_dims_used =
                        ((config_res.x <= native_res.x) &&
                         (config_res.y <= native_res.y))
                        ? config_gui_dims
                        : min_gui_dims;

                TRACE << "Max number of GUI cells used (based on desired and "
                      << "native resolution): "
                      << std::endl
                      << screen_gui_dims_used.x
                      << ","
                      << screen_gui_dims_used.y
                      << std::endl;

                panels::init(screen_gui_dims_used);

                const P screen_panel_px_dims = panel_px_dims(Panel::screen);

                screen_px_dims =
                        P(std::max(screen_panel_px_dims.x, config_res.x),
                          std::max(screen_panel_px_dims.y, config_res.y));

                init_window(screen_px_dims);
        }

        if (!s_sdl_window) {
                TRACE_ERROR_RELEASE << "Failed to set up window"
                                    << std::endl
                                    << SDL_GetError()
                                    << std::endl;
        }

        init_renderer(screen_px_dims);

        init_screen_surface(screen_px_dims);

        s_bpp = s_screen_srf->format->BytesPerPixel;

        TRACE << "bpp: " << s_bpp << std::endl;

        switch (s_bpp) {
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
                TRACE_ERROR_RELEASE << "Invalid bpp: " << s_bpp << std::endl;

                PANIC;
        }

        init_screen_texture(screen_px_dims);

        load_font();

        if (config::is_tiles_mode()) {
                load_tiles();

                load_images();
        }

        TRACE_FUNC_END;
}

void cleanup()
{
        TRACE_FUNC_BEGIN;

        if (s_sdl_renderer) {
                SDL_DestroyRenderer(s_sdl_renderer);
                s_sdl_renderer = nullptr;
        }

        if (s_sdl_window) {
                SDL_DestroyWindow(s_sdl_window);
                s_sdl_window = nullptr;
        }

        if (s_screen_texture) {
                SDL_DestroyTexture(s_screen_texture);
                s_screen_texture = nullptr;
        }

        if (s_screen_srf) {
                SDL_FreeSurface(s_screen_srf);
                s_screen_srf = nullptr;
        }

        if (s_main_menu_logo_srf) {
                SDL_FreeSurface(s_main_menu_logo_srf);
                s_main_menu_logo_srf = nullptr;
        }

        TRACE_FUNC_END;
}

void update_screen()
{
        const auto screen_panel_dims = panel_px_dims(Panel::screen);

        bool is_centering_allowed = true;

        if (!panels::is_valid() &&
            (screen_panel_dims.x > config::gui_cell_px_w()) &&
            (screen_panel_dims.y > config::gui_cell_px_h())) {
                draw_text_at_px(
                        "Window too small",
                        {0, 0},
                        colors::light_white(),
                        DrawBg::no,
                        colors::black());

                is_centering_allowed = false;
        }

        SDL_UpdateTexture(
                s_screen_texture,
                nullptr,
                s_screen_srf->pixels,
                s_screen_srf->pitch);

        const auto screen_srf_dims =
                P(s_screen_srf->w,
                  s_screen_srf->h);

        P offsets(0, 0);

        if (is_centering_allowed) {
                offsets =
                        (screen_srf_dims - screen_panel_dims)
                                .scaled_down(2);

                // Since we render the screen texture with an offset, we create
                // a blank space which we need to clear
                const auto bg_color = colors::black();

                SDL_SetRenderDrawColor(
                        s_sdl_renderer,
                        bg_color.r(),
                        bg_color.g(),
                        bg_color.b(),
                        255);

                SDL_RenderClear(s_sdl_renderer);
        }

        SDL_Rect dstrect;

        dstrect.x = offsets.x;
        dstrect.y = offsets.y;
        dstrect.w = s_screen_srf->w;
        dstrect.h = s_screen_srf->h;

        SDL_RenderCopy(
                s_sdl_renderer,
                s_screen_texture,
                nullptr,
                &dstrect);

        SDL_RenderPresent(s_sdl_renderer);
}

void clear_screen()
{
        const auto clr = colors::black();

        SDL_FillRect(
                s_screen_srf,
                nullptr,
                SDL_MapRGB(
                        s_screen_srf->format,
                        clr.r(),
                        clr.g(),
                        clr.b()));
}

P min_screen_gui_dims()
{
        // Use the maximum of:
        // * The hard minimum required number of gui cells
        // * The minimum required resolution, converted to gui cells, rounded up

        const int gui_cell_w = config::gui_cell_px_w();
        const int gui_cell_h = config::gui_cell_px_h();

        int min_gui_cells_x = (g_min_res_w + gui_cell_w - 1) / gui_cell_w;
        int min_gui_cells_y = (g_min_res_h + gui_cell_h - 1) / gui_cell_h;

        min_gui_cells_x = std::max(min_gui_cells_x, g_min_nr_gui_cells_x);
        min_gui_cells_y = std::max(min_gui_cells_y, g_min_nr_gui_cells_y);

        return P(min_gui_cells_x, min_gui_cells_y);
}

void on_fullscreen_toggled()
{
        clear_screen();

        update_screen();

        // TODO: This should not be necessary - but without this call there is
        // some strange behavior with the window losing focus, and becoming
        // hidden behind other windows when toggling fullscreen (until you
        // alt-tab back to the window)
        sdl_base::init();

        init();

        states::draw();

        update_screen();
}

R gui_to_px_rect(const R rect)
{
        const int gui_cell_px_w = config::gui_cell_px_w();
        const int gui_cell_px_h = config::gui_cell_px_h();

        R px_rect = rect.scaled_up(gui_cell_px_w, gui_cell_px_h);

        // We must include ALL pixels in the given gui area
        px_rect.p1 =
                px_rect.p1.with_offsets(
                        gui_cell_px_w - 1,
                        gui_cell_px_h - 1);

        return px_rect;
}

int gui_to_px_coords_x(const int value)
{
        return value * config::gui_cell_px_w();
}

int gui_to_px_coords_y(const int value)
{
        return value * config::gui_cell_px_h();
}

int map_to_px_coords_x(const int value)
{
        return value * config::map_cell_px_w();
}

int map_to_px_coords_y(const int value)
{
        return value * config::map_cell_px_h();
}

P gui_to_px_coords(const P pos)
{
        return P(gui_to_px_coords_x(pos.x), gui_to_px_coords_y(pos.y));
}

P gui_to_px_coords(const int x, const int y)
{
        return gui_to_px_coords(P(x, y));
}

P map_to_px_coords(const P pos)
{
        return P(map_to_px_coords_x(pos.x), map_to_px_coords_y(pos.y));
}

P map_to_px_coords(const int x, const int y)
{
        return map_to_px_coords(P(x, y));
}

P px_to_gui_coords(const P px_pos)
{
        return P(px_pos.x / config::gui_cell_px_w(), px_pos.y / config::gui_cell_px_h());
}

P px_to_map_coords(const P px_pos)
{
        return P(px_pos.x / config::map_cell_px_w(), px_pos.y / config::map_cell_px_h());
}

P gui_to_map_coords(const P gui_pos)
{
        const P px_coords = gui_to_px_coords(gui_pos);

        return px_to_map_coords(px_coords);
}

P gui_to_px_coords(const Panel panel, const P offset)
{
        const P pos = panels::area(panel).p0 + offset;

        return gui_to_px_coords(pos);
}

P map_to_px_coords(const Panel panel, const P offset)
{
        const P px_p0 = gui_to_px_coords(panels::area(panel).p0);

        const P px_offset = map_to_px_coords(offset);

        return px_p0 + px_offset;
}

void draw_tile(
        const gfx::TileId tile,
        const Panel panel,
        const P pos,
        const Color& color,
        const DrawBg draw_bg,
        const Color& bg_color)
{
        if (!panels::is_valid()) {
                return;
        }

        const P px_pos = map_to_px_coords(panel, pos);

        if (draw_bg == DrawBg::yes) {
                const P cell_dims(
                        config::map_cell_px_w(),
                        config::map_cell_px_h());

                draw_rectangle_filled(
                        {px_pos, px_pos + cell_dims - 1},
                        bg_color);
        }

        // Draw contour if neither the foreground nor background is black
        if ((color != colors::black()) &&
            (bg_color != colors::black())) {
                const auto& contour_px_data =
                        s_tile_contour_px_data[(size_t)tile];

                put_pixels_on_screen(
                        contour_px_data,
                        px_pos,
                        s_sdl_color_black);
        }

        put_pixels_on_screen(
                tile,
                px_pos,
                color.sdl_color());
}

void draw_character(
        const char character,
        const Panel panel,
        const P pos,
        const Color& color,
        const DrawBg draw_bg,
        const Color& bg_color)
{
        if (!panels::is_valid()) {
                return;
        }

        const P px_pos = gui_to_px_coords(panel, pos);

        const auto sdl_color = color.sdl_color();

        const auto sdl_color_bg = bg_color.sdl_color();

        draw_character_at_px(
                character,
                px_pos,
                sdl_color,
                draw_bg,
                sdl_color_bg);
}

void draw_text(
        const std::string& str,
        const Panel panel,
        const P pos,
        const Color& color,
        const DrawBg draw_bg,
        const Color& bg_color)
{
        if (!panels::is_valid()) {
                return;
        }

        P px_pos = gui_to_px_coords(panel, pos);

        draw_text_at_px(
                str,
                px_pos,
                color,
                draw_bg,
                bg_color);
}

void draw_text_center(
        const std::string& str,
        const Panel panel,
        const P pos,
        const Color& color,
        const DrawBg draw_bg,
        const Color& bg_color,
        const bool is_pixel_pos_adj_allowed)
{
        if (!panels::is_valid()) {
                return;
        }

        const int len = str.size();
        const int len_half = len / 2;
        const int x_pos_left = pos.x - len_half;

        P px_pos = gui_to_px_coords(
                panel,
                P(x_pos_left, pos.y));

        if (is_pixel_pos_adj_allowed) {
                const int pixel_x_adj =
                        ((len_half * 2) == len) ? (config::gui_cell_px_w() / 2) : 0;

                px_pos += P(pixel_x_adj, 0);
        }

        draw_text_at_px(
                str,
                px_pos,
                color,
                draw_bg,
                bg_color);
}

void draw_text_right(
        const std::string& str,
        const Panel panel,
        const P pos,
        const Color& color,
        const DrawBg draw_bg,
        const Color& bg_color)
{
        if (!panels::is_valid()) {
                return;
        }

        const int x_pos_left = pos.x - (int)str.size() + 1;

        P px_pos = gui_to_px_coords(
                panel,
                P(x_pos_left, pos.y));

        draw_text_at_px(
                str,
                px_pos,
                color,
                draw_bg,
                bg_color);
}

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
                s_screen_srf,
                &sdl_rect,
                SDL_MapRGB(
                        s_screen_srf->format,
                        sdl_color.r,
                        sdl_color.g,
                        sdl_color.b));
}

void cover_panel(const Panel panel, const Color& color)
{
        if (!panels::is_valid()) {
                return;
        }

        const R px_area = gui_to_px_rect(panels::area(panel));

        draw_rectangle_filled(px_area, color);
}

void cover_area(
        const Panel panel,
        const R area,
        const Color& color)
{
        const P panel_p0 = panels::p0(panel);

        const R screen_area = area.with_offset(panel_p0);

        const R px_area = gui_to_px_rect(screen_area);

        draw_rectangle_filled(px_area, color);
}

void cover_area(
        const Panel panel,
        const P offset,
        const P dims,
        const Color& color)
{
        cover_area(
                panel,
                {offset, offset + dims - 1},
                color);
}

void cover_cell(const Panel panel, const P offset)
{
        cover_area(panel, offset, P(1, 1));
}

void draw_descr_box(const std::vector<ColoredString>& lines)
{
        cover_panel(Panel::item_descr);

        P pos(0, 0);

        for (const auto& line : lines) {
                const auto formatted =
                        text_format::split(
                                line.str,
                                panels::w(Panel::item_descr));

                for (const auto& formatted_line : formatted) {
                        draw_text(
                                formatted_line,
                                Panel::item_descr,
                                pos,
                                line.color);

                        ++pos.y;
                }

                ++pos.y;
        }
}

void draw_main_menu_logo()
{
        if (!panels::is_valid()) {
                return;
        }

        const int screen_px_w = panel_px_w(Panel::screen);

        const int logo_px_h = s_main_menu_logo_srf->w;

        const P px_pos((screen_px_w - logo_px_h) / 2, 0);

        blit_surface(*s_main_menu_logo_srf, px_pos);
}

void draw_blast_at_cells(const std::vector<P>& positions, const Color& color)
{
        TRACE_FUNC_BEGIN;

        if (!panels::is_valid()) {
                TRACE_FUNC_END;

                return;
        }

        states::draw();

        for (const P pos : positions) {
                if (!viewport::is_in_view(pos)) {
                        continue;
                }

                draw_symbol(
                        gfx::TileId::blast1,
                        '*',
                        Panel::map,
                        viewport::to_view_pos(pos),
                        color);
        }

        update_screen();

        sdl_base::sleep(config::delay_explosion() / 2);

        for (const P pos : positions) {
                if (!viewport::is_in_view(pos)) {
                        continue;
                }

                draw_symbol(
                        gfx::TileId::blast2,
                        '*',
                        Panel::map,
                        viewport::to_view_pos(pos),
                        color);
        }

        update_screen();

        sdl_base::sleep(config::delay_explosion() / 2);

        TRACE_FUNC_END;
}

void draw_blast_at_seen_cells(const std::vector<P>& positions, const Color& color)
{
        if (!panels::is_valid()) {
                return;
        }

        std::vector<P> positions_with_vision;

        for (const P p : positions) {
                if (map::g_cells.at(p).is_seen_by_player) {
                        positions_with_vision.push_back(p);
                }
        }

        if (!positions_with_vision.empty()) {
                io::draw_blast_at_cells(positions_with_vision, color);
        }
}

void draw_blast_at_seen_actors(
        const std::vector<actor::Actor*>& actors,
        const Color& color)
{
        if (!panels::is_valid()) {
                return;
        }

        std::vector<P> positions;

        positions.reserve(actors.size());
        for (auto* const actor : actors) {
                positions.push_back(actor->m_pos);
        }

        draw_blast_at_seen_cells(positions, color);
}

void draw_symbol(
        const gfx::TileId tile,
        const char character,
        const Panel panel,
        const P pos,
        const Color& color,
        const DrawBg draw_bg,
        const Color& color_bg)
{
        if (config::is_tiles_mode()) {
                draw_tile(
                        tile,
                        panel,
                        pos,
                        color,
                        draw_bg,
                        color_bg);
        } else {
                draw_character(
                        character,
                        panel,
                        pos,
                        color,
                        draw_bg,
                        color_bg);
        }
}

void flush_input()
{
        SDL_PumpEvents();
}

void clear_events()
{
        while (SDL_PollEvent(&s_sdl_event)) {
        }
}

InputData get()
{
        InputData input;

        SDL_StartTextInput();

        bool is_done = false;

        bool is_window_resized = false;

        uint32_t ms_at_last_window_resize = 0;

        while (!is_done) {
                sdl_base::sleep(1);

                const SDL_Keymod mod = SDL_GetModState();

                input.is_shift_held = mod & KMOD_SHIFT;
                input.is_ctrl_held = mod & KMOD_CTRL;
                input.is_alt_held = mod & KMOD_ALT;

                const bool did_poll_event = SDL_PollEvent(&s_sdl_event);

                // Handle window resizing
                if (!config::is_fullscreen()) {
                        if (is_window_resized) {
                                on_window_resized();

                                clear_screen();

                                io::update_screen();

                                clear_events();

                                is_window_resized = false;

                                ms_at_last_window_resize = SDL_GetTicks();

                                continue;
                        }

                        if (ms_at_last_window_resize != 0) {
                                const auto d =
                                        SDL_GetTicks() -
                                        ms_at_last_window_resize;

                                if (d > 150) {
                                        states::draw();

                                        io::update_screen();

                                        ms_at_last_window_resize = 0;
                                }
                        }
                }

                if (!did_poll_event) {
                        continue;
                }

                switch (s_sdl_event.type) {
                case SDL_WINDOWEVENT: {
                        switch (s_sdl_event.window.event) {
                        case SDL_WINDOWEVENT_SIZE_CHANGED: {
                                TRACE << "Window resized" << std::endl;

                                if (!config::is_fullscreen()) {
                                        is_window_resized = true;
                                }
                        } break;

                        case SDL_WINDOWEVENT_RESTORED: {
                                TRACE << "Window restored" << std::endl;
                        }
                        // Fallthrough
                        case SDL_WINDOWEVENT_FOCUS_GAINED: {
                                TRACE << "Window gained focus" << std::endl;

                                clear_events();

                                sdl_base::sleep(100);
                        }
                        // Fallthrough
                        case SDL_WINDOWEVENT_EXPOSED: {
                                TRACE << "Window exposed" << std::endl;

                                states::draw();

                                io::update_screen();
                        } break;

                        default:
                        {
                        } break;
                        } // window event switch
                } break; // case SDL_WINDOWEVENT

                case SDL_QUIT: {
                        input.key = SDLK_ESCAPE;

                        is_done = true;
                } break;

                case SDL_KEYDOWN: {
                        input.key = s_sdl_event.key.keysym.sym;

                        switch (input.key) {
                        case SDLK_RETURN:
                        case SDLK_RETURN2:
                        case SDLK_KP_ENTER: {
                                if (input.is_alt_held) {
                                        TRACE << "Alt-Enter pressed"
                                              << std::endl;

                                        config::set_fullscreen(
                                                !config::is_fullscreen());

                                        on_fullscreen_toggled();

                                        // SDL_SetWindowSize(sdl_window_,

                                        // TODO: For some reason, the alt key
                                        // gets "stuck" after toggling
                                        // fullscreen, and must be cleared here
                                        // manually. Don't know if this is an
                                        // issue in the IA code, or an SDL bug.
                                        SDL_SetModState(KMOD_NONE);

                                        clear_events();

                                        flush_input();

                                        continue;
                                } else {
                                        // Alt is not held
                                        input.key = SDLK_RETURN;

                                        is_done = true;
                                }
                        } break; // case SDLK_KP_ENTER

                        case SDLK_KP_6:
                        case SDLK_KP_1:
                        case SDLK_KP_2:
                        case SDLK_KP_3:
                        case SDLK_KP_4:
                        case SDLK_KP_5:
                        case SDLK_KP_7:
                        case SDLK_KP_8:
                        case SDLK_KP_9:
                        case SDLK_KP_0:
                        case SDLK_SPACE:
                        case SDLK_BACKSPACE:
                        case SDLK_TAB:
                        case SDLK_PAGEUP:
                        case SDLK_PAGEDOWN:
                        case SDLK_END:
                        case SDLK_HOME:
                        case SDLK_INSERT:
                        case SDLK_DELETE:
                        case SDLK_LEFT:
                        case SDLK_RIGHT:
                        case SDLK_UP:
                        case SDLK_DOWN:
                        case SDLK_ESCAPE:
                        case SDLK_F1:
                        case SDLK_F2:
                        case SDLK_F3:
                        case SDLK_F4:
                        case SDLK_F5:
                        case SDLK_F6:
                        case SDLK_F7:
                        case SDLK_F8:
                        case SDLK_F9: {
                                is_done = true;
                        } break;

                        default:
                        {
                        } break;
                        } // Key down switch
                } break; // case SDL_KEYDOWN

                case SDL_KEYUP: {
                        const auto sdl_keysym = s_sdl_event.key.keysym.sym;

                        switch (sdl_keysym) {
                        case SDLK_LSHIFT:
                        case SDLK_RSHIFT: {
                                // Shift released

                                // On Windows, when the user presses
                                // shift + a numpad key, a shift release event
                                // can be received before the numpad key event,
                                // which breaks shift + numpad combinations.
                                // As a workaround, we check for "future"
                                // numpad events here.
                                SDL_Event sdl_event_tmp;

                                while (SDL_PollEvent(&sdl_event_tmp)) {
                                        if (sdl_event_tmp.type != SDL_KEYDOWN) {
                                                continue;
                                        }

                                        switch (sdl_event_tmp.key.keysym.sym) {
                                        case SDLK_KP_0:
                                        case SDLK_KP_1:
                                        case SDLK_KP_2:
                                        case SDLK_KP_3:
                                        case SDLK_KP_4:
                                        case SDLK_KP_5:
                                        case SDLK_KP_6:
                                        case SDLK_KP_7:
                                        case SDLK_KP_8:
                                        case SDLK_KP_9: {
                                                input.key =
                                                        sdl_event_tmp.key.keysym
                                                                .sym;

                                                is_done = true;
                                        } break;

                                        default:
                                        {
                                        } break;
                                        } // Key down switch
                                } // while polling event
                        } break;

                        default:
                        {
                        } break;
                        } // Key released switch
                } break; // case SDL_KEYUP

                case SDL_TEXTINPUT: {
                        const char c = s_sdl_event.text.text[0];

                        if (c == '+' || c == '-') {
                                if (config::is_fullscreen() ||
                                    is_window_maximized()) {
                                        continue;
                                }

                                P gui_dims = sdl_window_gui_dims();

                                if (c == '+') {
                                        if (input.is_ctrl_held) {
                                                ++gui_dims.y;
                                        } else {
                                                ++gui_dims.x;
                                        }
                                } else if (c == '-') {
                                        if (input.is_ctrl_held) {
                                                --gui_dims.y;
                                        } else {
                                                --gui_dims.x;
                                        }
                                }

                                try_set_window_gui_cells(gui_dims);

                                is_window_resized = true;

                                continue;
                        }

                        if (c >= 33 && c < 126) {
                                // ASCII char entered
                                // (Decimal unicode '!' = 33, '~' = 126)

                                clear_events();

                                input.key = c;

                                is_done = true;
                        } else {
                                continue;
                        }
                } break; // case SDL_TEXT_INPUT

                default:
                {
                } break;

                } // event switch

        } // while

        SDL_StopTextInput();

        return input;
}

} // namespace io
