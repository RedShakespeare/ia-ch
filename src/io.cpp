// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "io.hpp"

#include <algorithm>
#include <cstddef>
#include <charconv>
#include <fstream>
#include <iterator>
#include <regex>
#include <optional>
#include <ostream>
#include <unordered_map>

#include "SDL.h"
#include "SDL_blendmode.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_filesystem.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_stdinc.h"
#include "SDL_surface.h"
#include "SDL_timer.h"
#include "SDL_video.h"
#include "audio.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "direction.hpp"
#include "io_internal.hpp"
#include "paths.hpp"
#include "state.hpp"
#include "text_format.hpp"
#include "version.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static SDL_Surface* load_surface(const std::string& path)
{
    auto* const surface = IMG_Load(path.c_str());

    if (!surface) {
        TRACE_ERROR_RELEASE
            << "Failed to load surface from path '"
            << path
            << "': "
            << IMG_GetError()
            << "\n";

        PANIC;
    }

    return surface;
}

static std::unordered_map<std::string, int> s_font_glyph_indices;
static int s_font_glyph_columns = 95;

struct FontGlyph
{
    SDL_Rect source_rect {};
    int logical_w {};
    int logical_h {};
    int advance {};
    int render_offset_x {};
    int render_offset_y {};
};

static std::unordered_map<std::string, FontGlyph> s_font_glyphs;

static std::string font_map_path_from_img_path(const std::string& img_path)
{
    const size_t suffix_pos = img_path.find_last_of('.');

    if (suffix_pos == std::string::npos) {
        return img_path + ".json";
    }

    return img_path.substr(0, suffix_pos) + ".json";
}

static std::optional<int> parse_json_int(const std::string& line, const std::string& key)
{
    const std::regex regex("^\\s*\"" + key + "\"\\s*:\\s*(-?[0-9]+),?\\s*$");
    std::smatch match;

    if (!std::regex_match(line, match, regex)) {
        return std::nullopt;
    }

    return std::stoi(match[1].str());
}

static std::optional<int> parse_json_codepoint(const std::string& line)
{
    const std::regex regex(
        R"REGEX(^\s*"codepoint"\s*:\s*"U\+([0-9A-Fa-f]+)",?\s*$)REGEX");
    std::smatch match;

    if (!std::regex_match(line, match, regex)) {
        return std::nullopt;
    }

    int codepoint = 0;
    const std::string codepoint_str = match[1].str();
    const auto result = std::from_chars(
        codepoint_str.data(),
        codepoint_str.data() + codepoint_str.size(),
        codepoint,
        16);

    if (result.ec != std::errc()) {
        return std::nullopt;
    }

    return codepoint;
}

static std::string codepoint_to_utf8(const int codepoint)
{
    std::string result;

    if (codepoint <= 0x7f) {
        result.push_back(static_cast<char>(codepoint));
    }
    else if (codepoint <= 0x7ff) {
        result.push_back(static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else if (codepoint <= 0xffff) {
        result.push_back(static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else {
        result.push_back(static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }

    return result;
}

static void load_font_map(const std::string& img_path)
{
    s_font_glyph_indices.clear();
    s_font_glyphs.clear();
    s_font_glyph_columns = 95;

    const std::string map_path = font_map_path_from_img_path(img_path);

    std::ifstream file(map_path);

    if (!file) {
        TRACE << "No font map found at: " << map_path << "\n";

        return;
    }

    TRACE << "Loading font map: " << map_path << "\n";

    const std::regex columns_regex(R"(^\s*"columns"\s*:\s*([0-9]+),?\s*$)");
    const std::regex glyph_regex(R"REGEX(^\s{4}"((?:[^"\\]|\\.)+)"\s*:\s*\{\s*$)REGEX");
    const std::regex index_regex(R"(^\s*"index"\s*:\s*([0-9]+),?\s*$)");
    const std::regex glyph_end_regex(R"(^\s{4}\},?\s*$)");

    std::string current_glyph;
    std::string line;
    std::optional<int> current_codepoint;
    std::optional<int> current_index;
    std::optional<int> current_x_px;
    std::optional<int> current_y_px;
    std::optional<int> current_w;
    std::optional<int> current_h;
    std::optional<int> current_logical_w;
    std::optional<int> current_logical_h;
    std::optional<int> current_advance;
    std::optional<int> current_render_offset_x;
    std::optional<int> current_render_offset_y;

    const auto finish_glyph = [&]() {
        if (current_glyph.empty()) {
            return;
        }

        const std::string glyph =
            current_codepoint ? codepoint_to_utf8(*current_codepoint) : current_glyph;

        if (current_x_px &&
            current_y_px &&
            current_w &&
            current_h) {
            const int logical_w = current_logical_w.value_or(config::gui_cell_px_w());
            const int logical_h = current_logical_h.value_or(config::gui_cell_px_h());

            s_font_glyphs[glyph] = FontGlyph {
                {*current_x_px, *current_y_px, *current_w, *current_h},
                logical_w,
                logical_h,
                current_advance.value_or(logical_w),
                current_render_offset_x.value_or(0),
                current_render_offset_y.value_or(0)};
        }

        if (current_index) {
            const bool is_non_ascii =
                static_cast<unsigned char>(glyph[0]) >= 0x80;

            if (is_non_ascii) {
                s_font_glyph_indices[glyph] = *current_index;
            }
        }

        current_glyph.clear();
        current_codepoint.reset();
        current_index.reset();
        current_x_px.reset();
        current_y_px.reset();
        current_w.reset();
        current_h.reset();
        current_logical_w.reset();
        current_logical_h.reset();
        current_advance.reset();
        current_render_offset_x.reset();
        current_render_offset_y.reset();
    };

    while (std::getline(file, line)) {
        std::smatch match;

        if (std::regex_match(line, match, columns_regex)) {
            s_font_glyph_columns = std::stoi(match[1].str());

            continue;
        }

        if (std::regex_match(line, match, glyph_regex)) {
            finish_glyph();
            current_glyph = match[1].str();

            continue;
        }

        if (current_glyph.empty()) {
            continue;
        }

        if (std::regex_match(line, match, index_regex)) {
            current_index = std::stoi(match[1].str());

            continue;
        }

        if (const auto codepoint_value = parse_json_codepoint(line)) {
            current_codepoint = *codepoint_value;
        }
        else if (const auto x_px_value = parse_json_int(line, "x_px")) {
            current_x_px = *x_px_value;
        }
        else if (const auto y_px_value = parse_json_int(line, "y_px")) {
            current_y_px = *y_px_value;
        }
        else if (const auto width_value = parse_json_int(line, "width")) {
            current_w = *width_value;
        }
        else if (const auto height_value = parse_json_int(line, "height")) {
            current_h = *height_value;
        }
        else if (const auto logical_width_value = parse_json_int(line, "logical_width")) {
            current_logical_w = *logical_width_value;
        }
        else if (const auto logical_height_value = parse_json_int(line, "logical_height")) {
            current_logical_h = *logical_height_value;
        }
        else if (const auto advance_value = parse_json_int(line, "advance")) {
            current_advance = *advance_value;
        }
        else if (const auto render_offset_x_value = parse_json_int(line, "render_offset_x")) {
            current_render_offset_x = *render_offset_x_value;
        }
        else if (const auto render_offset_y_value = parse_json_int(line, "render_offset_y")) {
            current_render_offset_y = *render_offset_y_value;
        }
        else if (std::regex_match(line, glyph_end_regex)) {
            finish_glyph();
        }
    }

    finish_glyph();
}

static int glyph_index(const std::string& glyph)
{
    if (glyph.size() == 1) {
        const auto c = static_cast<unsigned char>(glyph[0]);

        if ((c >= ' ') && (c <= '~')) {
            return c - ' ';
        }
    }

    const auto it = s_font_glyph_indices.find(glyph);

    if (it != std::end(s_font_glyph_indices)) {
        return it->second;
    }

    return '?' - ' ';
}

static std::optional<FontGlyph> glyph_metadata(const std::string& glyph)
{
    const auto it = s_font_glyphs.find(glyph);

    if (it != std::end(s_font_glyphs)) {
        return it->second;
    }

    return std::nullopt;
}

static void swap_surface_color(
    SDL_Surface& surface,
    const Color& color_before,
    const Color& color_after)
{
    for (int x = 0; x < surface.w; ++x) {
        for (int y = 0; y < surface.h; ++y) {
            const P p(x, y);

            const auto color = io::read_px_on_surface(surface, p);

            if (color == color_before) {
                io::put_px_on_surface(surface, p, color_after);
            }
        }
    }
}

static bool should_put_contour_at(
    const SDL_Surface& surface,
    const P& surface_px_pos,
    const R& surface_px_rect,
    const Color& bg_color,
    const Color& contour_color)
{
    // Only allow drawing a contour at pixels with the same color as the
    // background color parameter.
    const auto color = io::read_px_on_surface(surface, surface_px_pos);

    if (color != bg_color) {
#ifndef NDEBUG
        if ((color.r() != color.g()) ||
            (color.r() != color.b())) {
            TRACE
                << "Found color other than grayscale color: "
                << (int)color.r()
                << ","
                << (int)color.g()
                << ","
                << (int)color.b()
                << " - at position: "
                << surface_px_pos.x
                << "x"
                << surface_px_pos.y
                << "\n"
                << "(Background color is: "
                << (int)bg_color.r()
                << ","
                << (int)bg_color.g()
                << ","
                << (int)bg_color.b()
                << ")"
                << "\n";

            PANIC;
        }
#endif  // NDEBUG

        return false;
    }

    // Draw a contour here if it has a neighbour with different color than
    // the background or contour color (i.e. if it has a neighbour with a
    // color that will be drawn to the screen).
    auto pred = [&](const auto d) {
        const auto adj_p = surface_px_pos + d;

        if (!surface_px_rect.is_pos_inside(adj_p)) {
            return false;
        }

        const auto adj_color = io::read_px_on_surface(surface, adj_p);

        const bool is_bg_color = (adj_color == bg_color);
        const bool is_contour_color = (adj_color == contour_color);

        return !is_bg_color && !is_contour_color;
    };

    return (
        std::any_of(
            std::cbegin(dir_utils::g_dir_list),
            std::cend(dir_utils::g_dir_list),
            pred));
}

static void draw_black_contour_for_surface(
    SDL_Surface& surface,
    const Color& bg_color)
{
    const R surface_px_rect({0, 0}, {surface.w - 1, surface.h - 1});

    const auto contour_color = colors::black();

    for (const auto& surface_px_pos : surface_px_rect.positions()) {
        const bool should_put_contour =
            should_put_contour_at(
                surface,
                surface_px_pos,
                surface_px_rect,
                bg_color,
                contour_color);

        if (should_put_contour) {
            io::put_px_on_surface(
                surface,
                surface_px_pos,
                contour_color);
        }
    }
}

static void verify_tile_colors(
    const SDL_Surface& surface,
    const std::string& img_path)
{
#ifndef NDEBUG
    const Color full_black(0, 0, 0);
    const Color full_white(255, 255, 255);

    for (int x = 0; x < surface.w; ++x) {
        for (int y = 0; y < surface.h; ++y) {
            const Color color = io::read_px_on_surface(surface, {x, y});

            if ((color == full_black) || (color == full_white)) {
                continue;
            }

            TRACE
                << "Found illegal tile color in image '"
                << img_path
                << "': "
                << (int)color.r()
                << ","
                << (int)color.g()
                << ","
                << (int)color.b()
                << " - at position: "
                << x
                << "x"
                << y
                << "\n";
            PANIC;
        }
    }
#endif  // NDEBUG
}

static void verify_texture_size(
    SDL_Texture* texture,
    const P& expected_size,
    const std::string& img_path)
{
    P size;

    SDL_QueryTexture(texture, nullptr, nullptr, &size.x, &size.y);

    // Verify width and height of loaded image
    if (size != expected_size) {
        TRACE_ERROR_RELEASE
            << "Tile image at \""
            << img_path
            << "\" has wrong size: "
            << size.x
            << "x"
            << size.x
            << ", expected: "
            << expected_size.x
            << "x"
            << expected_size.y
            << "\n";

        PANIC;
    }
}

static SDL_Texture* create_texture_from_surface(SDL_Surface& surface)
{
    auto* const texture =
        SDL_CreateTextureFromSurface(
            io::g_sdl_renderer,
            &surface);

    if (!texture) {
        TRACE_ERROR_RELEASE
            << "Failed to create texture from surface: "
            << IMG_GetError()
            << "\n";

        PANIC;
    }

    return texture;
}

static void set_surface_color_key(SDL_Surface& surface, const Color& color)
{
    const auto v =
        SDL_MapRGB(
            surface.format,
            color.r(),
            color.g(),
            color.b());

    const bool enable_color_key = true;

    SDL_SetColorKey(&surface, enable_color_key, v);
}

static SDL_Texture* load_texture(const std::string& path)
{
    auto* const surface = load_surface(path);

    set_surface_color_key(*surface, colors::black());

    auto* const texture = create_texture_from_surface(*surface);

    SDL_FreeSurface(surface);

    return texture;
}

static SDL_Renderer* create_renderer()
{
    TRACE_FUNC_BEGIN;

    uint32_t flags = 0U;

    switch (config::renderer_type()) {
    case RendererType::auto_select:
        break;

    case RendererType::sw:
        flags = SDL_RENDERER_SOFTWARE;
        break;

    case RendererType::END:
        ASSERT(false);
        break;
    }

    SDL_Renderer* const renderer =
        SDL_CreateRenderer(io::g_sdl_window, -1, flags);

    if (!renderer) {
        TRACE_ERROR_RELEASE
            << "Failed to create SDL renderer: "
            << SDL_GetError()
            << "\n";

        PANIC;
    }

    SDL_RendererInfo info;

    if (SDL_GetRendererInfo(renderer, &info) != 0) {
        TRACE_ERROR_RELEASE
            << "Could not get RendererInfo: "
            << SDL_GetError()
            << "\n";

        PANIC;
    }

    TRACE
        << "Created SDL Renderer with name: '" << info.name << "'"
        << "\n";

    std::string flags_str;

    if (info.flags & SDL_RENDERER_SOFTWARE) {
        text_format::append_as_comma_list(
            flags_str, "SDL_RENDERER_SOFTWARE");
    }

    if (info.flags & SDL_RENDERER_ACCELERATED) {
        text_format::append_as_comma_list(
            flags_str, "SDL_RENDERER_ACCELERATED");
    }

    if (info.flags & SDL_RENDERER_PRESENTVSYNC) {
        text_format::append_as_comma_list(
            flags_str, "SDL_RENDERER_PRESENTVSYNC");
    }

    if (info.flags & SDL_RENDERER_TARGETTEXTURE) {
        text_format::append_as_comma_list(
            flags_str, "SDL_RENDERER_TARGETTEXTURE");
    }

    TRACE << "Flags: [" + flags_str + "]" << "\n";

    TRACE_FUNC_END;

    return renderer;
}

static void init_renderer()
{
    TRACE_FUNC_BEGIN;

    if (io::g_sdl_renderer) {
        SDL_DestroyRenderer(io::g_sdl_renderer);
    }

    io::g_sdl_renderer = create_renderer();

    TRACE_FUNC_END;
}

static void load_logo()
{
    TRACE_FUNC_BEGIN;

    const std::string img_path = paths::logo_img_path();

    io::g_logo_texture = load_texture(img_path);

    TRACE_FUNC_END;
}

static void load_font()
{
    TRACE_FUNC_BEGIN;

    const std::string img_path = paths::fonts_dir() + config::font_name();

    TRACE << "Loading font image: " << img_path << "\n";

    load_font_map(img_path);

    SDL_Surface* const surface = load_surface(img_path);

    swap_surface_color(*surface, colors::black(), colors::magenta());

    set_surface_color_key(*surface, colors::magenta());

    // Create the non-contour version
    SDL_Texture* texture = create_texture_from_surface(*surface);

    io::g_font_texture = texture;

    draw_black_contour_for_surface(*surface, colors::magenta());

    // Create the version with contour
    texture = create_texture_from_surface(*surface);

    io::g_font_texture_with_contours = texture;

    SDL_FreeSurface(surface);

    TRACE_FUNC_END;
}

static void load_tile(const gfx::TileId id, const P& cell_px_dims)
{
    const std::string img_path = paths::tiles_dir() + gfx::tile_id_to_filename(id);

    TRACE << "Loading tile image: " << img_path << "\n";

    SDL_Surface* const surface = load_surface(img_path);

    verify_tile_colors(*surface, img_path);

    swap_surface_color(*surface, colors::black(), colors::magenta());

    set_surface_color_key(*surface, colors::magenta());

    // Create the non-contour version
    SDL_Texture* texture = create_texture_from_surface(*surface);

    io::g_tile_textures[(size_t)id] = texture;

    draw_black_contour_for_surface(*surface, colors::magenta());

    // Create the version with contour
    texture = create_texture_from_surface(*surface);

    io::g_tile_textures_with_contours[(size_t)id] = texture;

    verify_texture_size(texture, cell_px_dims, img_path);

    SDL_FreeSurface(surface);
}

static void load_tiles()
{
    TRACE_FUNC_BEGIN;

    const P cell_px_dims(
        config::map_cell_px_w(),
        config::map_cell_px_h());

    for (size_t i = 0; i < (size_t)gfx::TileId::END; ++i) {
        load_tile((gfx::TileId)i, cell_px_dims);
    }

    TRACE_FUNC_END;
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace io
{
SDL_Texture* g_font_texture_with_contours = nullptr;
SDL_Texture* g_font_texture = nullptr;
SDL_Texture* g_tile_textures[(size_t)gfx::TileId::END] = {};
SDL_Texture* g_tile_textures_with_contours[(size_t)gfx::TileId::END] = {};
SDL_Texture* g_logo_texture = nullptr;

P g_rendering_px_offset = {};

void init_sdl()
{
    TRACE_FUNC_BEGIN;

    cleanup_sdl();

    const uint32_t sdl_init_flags =
        SDL_INIT_VIDEO |
        SDL_INIT_AUDIO |
        SDL_INIT_EVENTS;

    if (SDL_Init(sdl_init_flags) == -1) {
        TRACE_ERROR_RELEASE
            << "Failed to init SDL"
            << "\n"
            << SDL_GetError()
            << "\n";

        PANIC;
    }

    SDL_ShowCursor(SDL_FALSE);

    const uint32_t sdl_img_flags = IMG_INIT_PNG;

    if (IMG_Init(sdl_img_flags) == -1) {
        TRACE_ERROR_RELEASE
            << "Failed to init SDL_image"
            << "\n"
            << SDL_GetError()
            << "\n";

        PANIC;
    }

    TRACE_FUNC_END;
}

void cleanup_sdl()
{
    if (!SDL_WasInit(SDL_INIT_EVERYTHING)) {
        return;
    }

    IMG_Quit();

    SDL_Quit();
}

void init_sdl_audio()
{
    cleanup_sdl_audio();

    const int audio_freq = 44100;
    const Uint16 audio_format = MIX_DEFAULT_FORMAT;
    const int audio_channels = MIX_DEFAULT_CHANNELS;
    const int audio_buffers = config::audio_buffer_size();

    const int result =
        Mix_OpenAudio(
            audio_freq,
            audio_format,
            audio_channels,
            audio_buffers);

    if (result == -1) {
        TRACE_ERROR_RELEASE
            << "Failed to init SDL_mixer"
            << "\n"
            << SDL_GetError()
            << "\n";

        ASSERT(false);
    }

    Mix_AllocateChannels(audio::g_allocated_channels);
}

void cleanup_sdl_audio()
{
    Mix_AllocateChannels(0);

    Mix_CloseAudio();
}

void init_other()
{
    TRACE_FUNC_BEGIN;

    cleanup_other();

    init_window();
    init_renderer();

    SDL_SetRenderDrawBlendMode(io::g_sdl_renderer, SDL_BLENDMODE_BLEND);

    load_font();

    if (config::is_tiles_mode()) {
        load_tiles();
        load_logo();
    }

    init_input();
    init_animation();

    TRACE_FUNC_END;
}

void cleanup_other()
{
    TRACE_FUNC_BEGIN;

    if (g_sdl_renderer) {
        SDL_DestroyRenderer(g_sdl_renderer);
        g_sdl_renderer = nullptr;
    }

    if (g_sdl_window) {
        SDL_DestroyWindow(g_sdl_window);
        g_sdl_window = nullptr;
    }

    TRACE_FUNC_END;
}

void on_user_toggle_fullscreen()
{
    TRACE_FUNC_BEGIN;

    init_other();

    states::draw();
    update_screen();

    TRACE_FUNC_END;
}

void on_user_toggle_scaling()
{
    TRACE_FUNC_BEGIN;

    init_other();

    states::draw();
    update_screen();

    TRACE_FUNC_END;
}

void set_clip_rect_to_panel(const Panel panel)
{
    auto px_area = gui_to_px_rect(panels::area(panel));

    px_area = px_area.scaled_up(config::video_scale_factor());

    const SDL_Rect clip_rect {
        px_area.p0.x,
        px_area.p0.y,
        px_area.w(),
        px_area.h()};

    SDL_RenderSetClipRect(g_sdl_renderer, &clip_rect);
}

void disable_clip_rect()
{
    SDL_RenderSetClipRect(g_sdl_renderer, nullptr);
}

static void draw_glyph_index_at_px(
    const int glyph_idx,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color)
{
    // TODO: Black foreground looks terrible with grayscale shaded font
    // image (all shades of white are colored black). The current solution
    // to this is to simply never use black foreground, since it's not
    // really necessary.
    ASSERT(color != colors::black());

    P gui_cell_px_dims(config::gui_cell_px_w(), config::gui_cell_px_h());

    if (draw_bg == io::DrawBg::yes) {
        // NOTE: No rendering offsets or scaling calculated yet, the
        // rectangle function performs its own offsets and scaling.
        io::draw_rectangle_filled(
            {px_pos, px_pos + gui_cell_px_dims - 1},
            bg_color);
    }

    // Set up the texture clip rectangle, before calculating scaling
    // NOTE: We expect one pixel separator between each glyph.
    P char_px_pos(
        glyph_idx % s_font_glyph_columns,
        glyph_idx / s_font_glyph_columns);

    char_px_pos.x *= (gui_cell_px_dims.x + 1);
    char_px_pos.y *= (gui_cell_px_dims.y);

    SDL_Rect clip_rect;

    clip_rect.x = char_px_pos.x;
    clip_rect.y = char_px_pos.y;
    clip_rect.w = gui_cell_px_dims.x;
    clip_rect.h = gui_cell_px_dims.y;

    // * Now apply offset and scaling *

    // Scaling
    const int scale_factor = config::video_scale_factor();

    px_pos = px_pos.scaled_up(scale_factor);
    gui_cell_px_dims = gui_cell_px_dims.scaled_up(scale_factor);

    // Apply rendering offsets (to center the graphics in the window)
    px_pos = px_pos.with_offsets(g_rendering_px_offset);

    SDL_Rect render_rect;

    render_rect.x = px_pos.x;
    render_rect.y = px_pos.y;
    render_rect.w = gui_cell_px_dims.x;
    render_rect.h = gui_cell_px_dims.y;

    SDL_Texture* texture = nullptr;

    // TODO: If black foreground will not be allowed, the contour version
    // can probably always be used
    if (/* (color == colors::black()) || */
        (bg_color == colors::black())) {
        texture = g_font_texture;
    }
    else {
        texture = g_font_texture_with_contours;
    }

    const Color color_adapted = color.with_brightness(config::brightness_pct());

    SDL_SetTextureColorMod(
        texture,
        color_adapted.r(),
        color_adapted.g(),
        color_adapted.b());

    SDL_RenderCopy(g_sdl_renderer, texture, &clip_rect, &render_rect);
}

static void draw_glyph_metadata_at_px(
    const FontGlyph& glyph,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color)
{
    ASSERT(color != colors::black());

    const P logical_px_dims(glyph.logical_w, glyph.logical_h);

    if (draw_bg == io::DrawBg::yes) {
        io::draw_rectangle_filled(
            {px_pos, px_pos + logical_px_dims - 1},
            bg_color);
    }

    const int scale_factor = config::video_scale_factor();

    SDL_Rect clip_rect = glyph.source_rect;

    px_pos.x += glyph.render_offset_x;
    px_pos.y += glyph.render_offset_y;
    px_pos = px_pos.scaled_up(scale_factor);
    px_pos = px_pos.with_offsets(g_rendering_px_offset);

    SDL_Rect render_rect;
    render_rect.x = px_pos.x;
    render_rect.y = px_pos.y;
    render_rect.w = glyph.source_rect.w * scale_factor;
    render_rect.h = glyph.source_rect.h * scale_factor;

    SDL_Texture* texture = nullptr;

    if (bg_color == colors::black()) {
        texture = g_font_texture;
    }
    else {
        texture = g_font_texture_with_contours;
    }

    const Color color_adapted = color.with_brightness(config::brightness_pct());

    SDL_SetTextureColorMod(
        texture,
        color_adapted.r(),
        color_adapted.g(),
        color_adapted.b());

    SDL_RenderCopy(g_sdl_renderer, texture, &clip_rect, &render_rect);
}

void draw_character_at_px(
    const char character,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color)
{
    draw_glyph_at_px(std::string(1, character), px_pos, color, draw_bg, bg_color);
}

void draw_glyph_at_px(
    const std::string& glyph,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color)
{
    if (const auto metadata = glyph_metadata(glyph)) {
        draw_glyph_metadata_at_px(
            *metadata,
            px_pos,
            color,
            draw_bg,
            bg_color);

        return;
    }

    draw_glyph_index_at_px(
        glyph_index(glyph),
        px_pos,
        color,
        draw_bg,
        bg_color);
}

int glyph_advance_px(const std::string& glyph)
{
    if (const auto metadata = glyph_metadata(glyph)) {
        return metadata->advance;
    }

    return config::gui_cell_px_w();
}

void draw_character(const CharacterDrawObj& obj)
{
    const auto px_pos = gui_to_px_coords(obj.panel, obj.pos);

    const auto sdl_color = obj.color.sdl_color();
    const auto sdl_color_bg = obj.bg_color.sdl_color();

    draw_character_at_px(
        obj.character,
        px_pos,
        sdl_color,
        obj.draw_bg,
        sdl_color_bg);
}

void draw_tile(const TileDrawObj& obj)
{
    P px_pos = map_to_px_coords(obj.panel, obj.pos);

    P map_cell_px_dims(config::map_cell_px_w(), config::map_cell_px_h());

    if (obj.draw_bg == DrawBg::yes) {
        // NOTE: No rendering offsets or scaling calculated yet, the
        // rectangle function performs its own offsets and scaling
        draw_rectangle_filled(
            {px_pos, px_pos + map_cell_px_dims - 1},
            obj.bg_color);
    }

    // * Now apply offset and scaling *

    // Scaling
    const int scale_factor = config::video_scale_factor();

    px_pos = px_pos.scaled_up(scale_factor);
    map_cell_px_dims = map_cell_px_dims.scaled_up(scale_factor);

    // Apply rendering offsets (to center the graphics in the window)
    px_pos = px_pos.with_offsets(g_rendering_px_offset);

    SDL_Rect render_rect;

    render_rect.x = px_pos.x;
    render_rect.y = px_pos.y;
    render_rect.w = map_cell_px_dims.x;
    render_rect.h = map_cell_px_dims.y;

    SDL_Texture* texture = nullptr;

    if ((obj.color == colors::black()) ||
        (obj.bg_color == colors::black())) {
        // Foreground or background is black - no contours
        texture = g_tile_textures[(size_t)obj.tile];
    }
    else {
        // Both foreground and background are non-black - use contours
        texture = g_tile_textures_with_contours[(size_t)obj.tile];
    }

    const Color color = obj.color.with_brightness(config::brightness_pct());

    SDL_SetTextureColorMod(texture, color.r(), color.g(), color.b());

    SDL_RenderCopy(g_sdl_renderer, texture, nullptr, &render_rect);
}

void cover_panel(const Panel panel, const Color& color)
{
    const auto px_area = gui_to_px_rect(panels::area(panel));

    draw_rectangle_filled(px_area, color);
}

void cover_area(
    const Panel panel,
    const R& area,
    const Color& color)
{
    const auto panel_p0 = panels::p0(panel);

    const auto screen_area = area.with_offset(panel_p0);

    const auto px_area =
        gui_to_px_rect(screen_area)
            .with_offset(g_rendering_px_offset);

    draw_rectangle_filled(px_area, color);
}

void cover_area(
    const Panel panel,
    const P& offset,
    const P& dims,
    const Color& color)
{
    const auto area = R(offset, offset + dims - 1);

    cover_area(panel, area, color);
}

void cover_cell(const Panel panel, const P& offset)
{
    cover_area(panel, offset, {1, 1});
}

void draw_logo(Color color)
{
    // Set pixel position *before* applying rendering offset and scaling
    const int screen_px_w = panel_px_w(Panel::screen);

    P img_px_dims;

    SDL_QueryTexture(
        g_logo_texture,
        nullptr,
        nullptr,
        &img_px_dims.x,
        &img_px_dims.y);

    P px_pos((screen_px_w - img_px_dims.x) / 2, gui_to_px_coords_y(1));

    // * Now apply offset and scaling *

    // Scaling
    const int scale_factor = config::video_scale_factor();

    px_pos = px_pos.scaled_up(scale_factor);
    img_px_dims = img_px_dims.scaled_up(scale_factor);

    // Apply rendering offsets (to center the graphics in the window)
    px_pos = px_pos.with_offsets(g_rendering_px_offset);

    SDL_Rect render_rect;

    render_rect.x = px_pos.x;
    render_rect.y = px_pos.y;
    render_rect.w = img_px_dims.x;
    render_rect.h = img_px_dims.y;

    color = color.with_brightness(config::brightness_pct());

    SDL_SetTextureColorMod(g_logo_texture, color.r(), color.g(), color.b());

    SDL_RenderCopy(g_sdl_renderer, g_logo_texture, nullptr, &render_rect);
}

std::string sdl_pref_dir()
{
    TRACE_FUNC_BEGIN;

    std::string subdir_str = version_info::g_version_str;

    std::replace(std::begin(subdir_str), std::end(subdir_str), '.', '_');
    std::replace(std::begin(subdir_str), std::end(subdir_str), '-', '_');

    const auto sha1_result = version_info::read_git_sha1_str_from_file();

    if (sha1_result) {
        subdir_str += "_" + sha1_result.value();
    }

    char* const path_ptr =
        // NOTE: This is somewhat of a hack, see the function arguments.
        SDL_GetPrefPath(
            "infra_arcana",       // "Organization"
            subdir_str.c_str());  // "Application"

    std::string path_str = path_ptr;

    SDL_free(path_ptr);

    TRACE << "SDL_GetPrefPath returned path '" << path_str << "'" << "\n";

    TRACE_FUNC_END;

    return path_str;
}

void sleep(const uint32_t duration)
{
    if ((duration == 0) || config::is_bot_playing()) {
        return;
    }
    else if (duration == 1) {
        SDL_Delay(duration);
    }
    else {
        // Duration longer than 1 ms
        const auto wait_until = SDL_GetTicks() + duration;

        while (SDL_GetTicks() < wait_until) {
            SDL_PumpEvents();
        }
    }
}

}  // namespace io
