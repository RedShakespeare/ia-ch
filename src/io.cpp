// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "io.hpp"
#include "io_internal.hpp"

#include "SDL_mixer.h"

#include "audio.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "paths.hpp"
#include "version.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static const SDL_Color s_sdl_color_black = {0, 0, 0, 0};

static SDL_Renderer* create_renderer(const P& px_dims)
{
        TRACE_FUNC_BEGIN;

        auto* const renderer =
                SDL_CreateRenderer(
                        io::g_sdl_window,
                        -1,
                        SDL_RENDERER_SOFTWARE);

        if (!renderer) {
                TRACE_ERROR_RELEASE
                        << "Failed to create SDL renderer"
                        << std::endl
                        << SDL_GetError()
                        << std::endl;

                PANIC;
        }

        SDL_RenderSetLogicalSize(renderer, px_dims.x, px_dims.y);

        // SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

        TRACE_FUNC_END;

        return renderer;
}

static SDL_Texture* create_texture(const P& px_dims)
{
        TRACE_FUNC_BEGIN;

        auto* const texture =
                SDL_CreateTexture(
                        io::g_sdl_renderer,
                        SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING,
                        px_dims.x,
                        px_dims.y);

        if (!texture) {
                TRACE_ERROR_RELEASE
                        << "Failed to create texture"
                        << std::endl
                        << SDL_GetError()
                        << std::endl;

                PANIC;
        }

        TRACE_FUNC_END;

        return texture;
}

static SDL_Surface* create_surface(const P px_dims)
{
        TRACE_FUNC_BEGIN;

        auto* surface =
                SDL_CreateRGBSurface(
                        0,
                        px_dims.x,
                        px_dims.y,
                        32,
                        0x00FF0000,
                        0x0000FF00,
                        0x000000FF,
                        0xFF000000);

        if (!surface) {
                TRACE_ERROR_RELEASE
                        << "Failed to create surface"
                        << std::endl
                        << SDL_GetError()
                        << std::endl;

                PANIC;
        }

        TRACE_FUNC_END;

        return surface;
}

static void cleanup_sdl()
{
        if (!SDL_WasInit(SDL_INIT_EVERYTHING)) {
                return;
        }

        IMG_Quit();

        Mix_AllocateChannels(0);

        Mix_CloseAudio();

        SDL_Quit();
}

static void init_sdl()
{
        TRACE_FUNC_BEGIN;

        cleanup_sdl();

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) == -1) {
                TRACE_ERROR_RELEASE << "Failed to init SDL"
                                    << std::endl
                                    << SDL_GetError()
                                    << std::endl;

                PANIC;
        }

        if (IMG_Init(IMG_INIT_PNG) == -1) {
                TRACE_ERROR_RELEASE << "Failed to init SDL_image"
                                    << std::endl
                                    << SDL_GetError()
                                    << std::endl;

                PANIC;
        }

        const int audio_freq = 44100;
        const Uint16 audio_format = MIX_DEFAULT_FORMAT;
        const int audio_channels = 2;
        const int audio_buffers = 1024;

        const int result =
                Mix_OpenAudio(
                        audio_freq,
                        audio_format,
                        audio_channels,
                        audio_buffers);

        if (result == -1) {
                TRACE_ERROR_RELEASE << "Failed to init SDL_mixer"
                                    << std::endl
                                    << SDL_GetError()
                                    << std::endl;

                ASSERT(false);
        }

        Mix_AllocateChannels(audio::g_allocated_channels);

        TRACE_FUNC_END;
}

static void init_screen_surface(const P& px_dims)
{
        TRACE_FUNC_BEGIN;

        if (io::g_screen_srf) {
                SDL_FreeSurface(io::g_screen_srf);
        }

        io::g_screen_srf = create_surface(px_dims);

        TRACE_FUNC_END;
}

static void init_window(const P px_dims)
{
        TRACE_FUNC_BEGIN;

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

        if (io::g_sdl_window) {
                SDL_DestroyWindow(io::g_sdl_window);
        }

        if (config::is_fullscreen()) {
                TRACE << "Fullscreen mode" << std::endl;

                io::g_sdl_window =
                        SDL_CreateWindow(
                                title.c_str(),
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                px_dims.x,
                                px_dims.y,
                                SDL_WINDOW_FULLSCREEN_DESKTOP);
        } else {
                // Windowed mode
                TRACE << "Windowed mode" << std::endl;

                io::g_sdl_window =
                        SDL_CreateWindow(
                                title.c_str(),
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                px_dims.x,
                                px_dims.y,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        }

        if (!io::g_sdl_window) {
                TRACE << "Failed to create window"
                      << std::endl
                      << SDL_GetError()
                      << std::endl;
        }

        TRACE_FUNC_END;
}

static void init_renderer(const P& px_dims)
{
        TRACE_FUNC_BEGIN;

        if (io::g_sdl_renderer) {
                SDL_DestroyRenderer(io::g_sdl_renderer);

                // NOTE: The above call also frees the texture
                io::g_screen_texture = nullptr;
        }

        io::g_sdl_renderer = create_renderer(px_dims);

        TRACE_FUNC_END;
}

static void init_screen_texture(const P& px_dims)
{
        TRACE_FUNC_BEGIN;

        if (io::g_screen_texture) {
                SDL_DestroyTexture(io::g_screen_texture);
        }

        io::g_screen_texture = create_texture(px_dims);

        TRACE_FUNC_END;
}

static P native_resolution_from_sdl()
{
        SDL_DisplayMode display_mode;

        const auto result = SDL_GetDesktopDisplayMode(0, &display_mode);

        if (result != 0) {
                TRACE_ERROR_RELEASE
                        << "Failed to read native resolution"
                        << std::endl
                        << SDL_GetError()
                        << std::endl;

                PANIC;
        }

        return {display_mode.w, display_mode.h};
}

static void blit_surface(SDL_Surface& srf, const P px_pos)
{
        SDL_Rect dst_rect;

        dst_rect.x = px_pos.x;
        dst_rect.y = px_pos.y;
        dst_rect.w = srf.w;
        dst_rect.h = srf.h;

        SDL_BlitSurface(&srf, nullptr, io::g_screen_srf, &dst_rect);
}

static void load_contour(
        const std::vector<P>& source_px_data,
        std::vector<P>& dest_px_data,
        const P cell_px_dims)
{
        for (const P& source_px_pos : source_px_data) {
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

        io::g_main_menu_logo_srf =
                SDL_ConvertSurface(
                        tmp_srf,
                        io::g_screen_srf->format,
                        0);

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
                TRACE_ERROR_RELEASE
                        << "Failed to load font: "
                        << IMG_GetError()
                        << std::endl;

                PANIC;
        }

        const auto img_color =
                SDL_MapRGB(
                        font_srf_tmp->format,
                        255,
                        255,
                        255);

        const P cell_px_dims(
                config::gui_cell_px_w(),
                config::gui_cell_px_h());

        for (int x = 0; x < (int)io::g_font_nr_x; ++x) {
                for (int y = 0; y < (int)io::g_font_nr_y; ++y) {
                        auto& px_data = io::g_font_px_data[x][y];

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
                                                io::px(
                                                        *font_srf_tmp,
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
                                io::g_font_contour_px_data[x][y],
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

                const auto img_color =
                        SDL_MapRGB(
                                tile_srf_tmp->format,
                                255,
                                255,
                                255);

                auto& px_data = io::g_tile_px_data[i];

                px_data.clear();

                for (int x = 0; x < cell_px_dims.x; ++x) {
                        for (int y = 0; y < cell_px_dims.y; ++y) {
                                const auto current_px =
                                        io::px(
                                                *tile_srf_tmp,
                                                x,
                                                y);

                                const bool is_img_px =
                                        current_px == img_color;

                                if (is_img_px) {
                                        px_data.emplace_back(x, y);
                                }
                        }
                }

                load_contour(
                        px_data,
                        io::g_tile_contour_px_data[i],
                        cell_px_dims);

                SDL_FreeSurface(tile_srf_tmp);
        }

        TRACE_FUNC_END;
}

static P sdl_window_px_dims()
{
        P px_dims;

        SDL_GetWindowSize(io::g_sdl_window, &px_dims.x, &px_dims.y);

        return px_dims;
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace io {

int g_bpp = -1;

SDL_Window* g_sdl_window = nullptr;
SDL_Renderer* g_sdl_renderer = nullptr;
SDL_Surface* g_screen_srf = nullptr;
SDL_Texture* g_screen_texture = nullptr;

SDL_Surface* g_main_menu_logo_srf = nullptr;

std::vector<P> g_tile_px_data[(size_t)gfx::TileId::END];
std::vector<P> g_font_px_data[g_font_nr_x][g_font_nr_y];

std::vector<P> g_tile_contour_px_data[(size_t)gfx::TileId::END];
std::vector<P> g_font_contour_px_data[g_font_nr_x][g_font_nr_y];

void init()
{
        TRACE_FUNC_BEGIN;

        cleanup();

        init_sdl();

        P screen_px_dims;

        if (config::is_fullscreen()) {
                const P resolution =
                        config::is_native_resolution_fullscreen()
                        ? native_resolution_from_sdl()
                        : io::gui_to_px_coords(io::min_screen_gui_dims());

                panels::init(io::px_to_gui_coords(resolution));

                screen_px_dims = panel_px_dims(Panel::screen);

                init_window(screen_px_dims);

                if (!g_sdl_window) {
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

                const auto screen_panel_px_dims = panel_px_dims(Panel::screen);

                screen_px_dims =
                        P(std::max(screen_panel_px_dims.x, config_res.x),
                          std::max(screen_panel_px_dims.y, config_res.y));

                init_window(screen_px_dims);
        }

        if (!g_sdl_window) {
                TRACE_ERROR_RELEASE
                        << "Failed to set up window"
                        << std::endl
                        << SDL_GetError()
                        << std::endl;

                PANIC;
        }

        init_renderer(screen_px_dims);

        init_screen_surface(screen_px_dims);

        g_bpp = g_screen_srf->format->BytesPerPixel;

        TRACE << "bpp: " << g_bpp << std::endl;

        init_px_manip();

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

        if (g_sdl_renderer) {
                SDL_DestroyRenderer(g_sdl_renderer);
                g_sdl_renderer = nullptr;

                // NOTE: The above call also frees the texture
                io::g_screen_texture = nullptr;
        }

        if (g_sdl_window) {
                SDL_DestroyWindow(g_sdl_window);
                g_sdl_window = nullptr;
        }

        if (g_screen_srf) {
                SDL_FreeSurface(g_screen_srf);
                g_screen_srf = nullptr;
        }

        if (g_main_menu_logo_srf) {
                SDL_FreeSurface(g_main_menu_logo_srf);
                g_main_menu_logo_srf = nullptr;
        }

        cleanup_sdl();

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
                g_screen_texture,
                nullptr,
                g_screen_srf->pixels,
                g_screen_srf->pitch);

        const auto screen_srf_dims =
                P(g_screen_srf->w,
                  g_screen_srf->h);

        P offsets(0, 0);

        if (is_centering_allowed) {
                offsets = (screen_srf_dims - screen_panel_dims).scaled_down(2);

                // Since we render the screen texture with an offset, we create
                // a blank space which we need to clear
                const auto bg_color = colors::black();

                SDL_SetRenderDrawColor(
                        g_sdl_renderer,
                        bg_color.r(),
                        bg_color.g(),
                        bg_color.b(),
                        255);

                SDL_RenderClear(g_sdl_renderer);
        }

        SDL_Rect dstrect;

        dstrect.x = offsets.x;
        dstrect.y = offsets.y;
        dstrect.w = g_screen_srf->w;
        dstrect.h = g_screen_srf->h;

        SDL_RenderCopy(
                g_sdl_renderer,
                g_screen_texture,
                nullptr,
                &dstrect);

        SDL_RenderPresent(g_sdl_renderer);
}

void clear_screen()
{
        const auto clr = colors::black();

        SDL_FillRect(
                g_screen_srf,
                nullptr,
                SDL_MapRGB(
                        g_screen_srf->format,
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
        io::init();

        init();

        states::draw();

        update_screen();
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
                        g_tile_contour_px_data[(size_t)tile];

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

void draw_main_menu_logo()
{
        if (!panels::is_valid()) {
                return;
        }

        const int screen_px_w = panel_px_w(Panel::screen);

        const int logo_px_h = g_main_menu_logo_srf->w;

        const P px_pos((screen_px_w - logo_px_h) / 2, 0);

        blit_surface(*g_main_menu_logo_srf, px_pos);
}

void try_set_window_gui_cells(P new_gui_dims)
{
        const P min_gui_dims = io::min_screen_gui_dims();

        new_gui_dims.x = std::max(new_gui_dims.x, min_gui_dims.x);
        new_gui_dims.y = std::max(new_gui_dims.y, min_gui_dims.y);

        const P new_px_dims = io::gui_to_px_coords(new_gui_dims);

        SDL_SetWindowSize(g_sdl_window, new_px_dims.x, new_px_dims.y);
}

void on_window_resized()
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

bool is_window_maximized()
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
        return SDL_GetWindowFlags(g_sdl_window) & SDL_WINDOW_MAXIMIZED;
}

P sdl_window_gui_dims()
{
        const P px_dims = sdl_window_px_dims();

        return io::px_to_gui_coords(px_dims);
}

std::string sdl_pref_dir()
{
        std::string version_str;

        if (version_info::g_version_str.empty()) {
                version_str = version_info::read_git_sha1_str_from_file();
        } else {
                version_str = version_info::g_version_str;
        }

        const auto path_ptr =
                // NOTE: This is somewhat of a hack, see the function arguments
                SDL_GetPrefPath(
                        "infra_arcana", // "Organization"
                        version_str.c_str()); // "Application"

        const std::string path_str = path_ptr;

        SDL_free(path_ptr);

        TRACE << "User data directory: " << path_str << std::endl;

        return path_str;
}

void sleep(const uint32_t duration)
{
        if ((duration == 0) ||
            config::is_bot_playing()) {
                return;
        }

        if (duration == 1) {
                SDL_Delay(duration);
        } else {
                // Duration longer than 1 ms
                const Uint32 wait_until = SDL_GetTicks() + duration;

                while (SDL_GetTicks() < wait_until) {
                        SDL_PumpEvents();
                }
        }
}

} // namespace io
