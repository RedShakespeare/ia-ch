// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "SDL_blendmode.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "SDL_version.h"
#include "colors.hpp"
#include "config.hpp"
#include "io.hpp"
#include "io_internal.hpp"
#include "panel.hpp"
#include "pos.hpp"
#include "utf8.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
namespace
{
struct TextGlyph
{
    uint32_t codepoint {};
    int advance_px {};
};

struct TextRun
{
    int advance_px {};
    bool has_non_ascii {};
    std::vector<TextGlyph> glyphs;
};

struct RenderedTextKey
{
    std::string text;
    Color color;
    bool use_contours {};
    int scale_factor {};
    int brightness_pct {};

    bool operator==(const RenderedTextKey& other) const
    {
        return (text == other.text) &&
            (color == other.color) &&
            (use_contours == other.use_contours) &&
            (scale_factor == other.scale_factor) &&
            (brightness_pct == other.brightness_pct);
    }
};

struct RenderedTextKeyHash
{
    size_t operator()(const RenderedTextKey& key) const
    {
        size_t result = std::hash<std::string> {}(key.text);
        result ^= ((size_t)key.color.r() << 1U);
        result ^= ((size_t)key.color.g() << 9U);
        result ^= ((size_t)key.color.b() << 17U);
        result ^= ((size_t)key.use_contours << 25U);
        result ^= ((size_t)key.scale_factor << 26U);
        result ^= ((size_t)key.brightness_pct << 30U);
        return result;
    }
};

struct RenderedText
{
    SDL_Texture* texture {};
    int w {};
    int h {};
};

std::unordered_map<std::string, TextRun> s_text_run_cache;
std::unordered_map<RenderedTextKey, RenderedText, RenderedTextKeyHash>
    s_rendered_text_cache;
constexpr size_t max_text_run_cache_entries = 4096;
constexpr size_t max_rendered_text_cache_entries = 512;

void draw_text_glyphs_uncached(
    const TextRun& run,
    P px_pos,
    const Color& color,
    io::DrawBg draw_bg,
    const Color& bg_color,
    bool msg_w_fit_on_screen);

uint32_t codepoint_at_or_fallback(const std::string& str, const size_t pos)
{
    return utf8::codepoint_at(str, pos).value_or('?');
}

const TextRun& text_run(const std::string& str)
{
    if (const auto it = s_text_run_cache.find(str);
        it != s_text_run_cache.end()) {
        return it->second;
    }

    if (s_text_run_cache.size() >= max_text_run_cache_entries) {
        s_text_run_cache.clear();
    }

    TextRun run;
    run.glyphs.reserve(str.size());

    for (size_t i = 0; i < str.size();) {
        const size_t cp_size = utf8::codepoint_size(str, i);
        if (cp_size == 0) {
            break;
        }

        const uint32_t codepoint = codepoint_at_or_fallback(str, i);
        const int advance_px = io::glyph_advance_px(codepoint);

        run.advance_px += advance_px;
        run.has_non_ascii = run.has_non_ascii || (codepoint > 0x7f);
        run.glyphs.push_back({codepoint, advance_px});

        i += cp_size;
    }

    const auto inserted = s_text_run_cache.emplace(str, std::move(run));
    return inserted.first->second;
}

void clear_rendered_text_cache()
{
    for (auto& entry : s_rendered_text_cache) {
        SDL_DestroyTexture(entry.second.texture);
    }

    s_rendered_text_cache.clear();
}

bool use_contour_font_surface(const Color& bg_color)
{
    return bg_color != colors::black();
}

SDL_Surface* font_surface_for_bg(const Color& bg_color)
{
    return use_contour_font_surface(bg_color)
        ? io::g_font_surface_with_contours
        : io::g_font_surface;
}

void put_px_rgba(SDL_Surface& surface, const P& px_pos, const Uint32 px)
{
    auto* const p =
        (Uint8*)surface.pixels +
        (px_pos.y * surface.pitch) +
        (px_pos.x * surface.format->BytesPerPixel);

    *(Uint32*)p = px;
}

void put_scaled_px(
    SDL_Surface& surface,
    const P& px_pos,
    const int scale_factor,
    const Uint32 px)
{
    const P scaled_pos = px_pos.scaled_up(scale_factor);

    for (int x = 0; x < scale_factor; ++x) {
        for (int y = 0; y < scale_factor; ++y) {
            const P dst_pos = scaled_pos.with_offsets(x, y);

            if ((dst_pos.x < 0) ||
                (dst_pos.y < 0) ||
                (dst_pos.x >= surface.w) ||
                (dst_pos.y >= surface.h)) {
                continue;
            }

            put_px_rgba(surface, dst_pos, px);
        }
    }
}

void configure_rendered_text_texture(SDL_Texture& texture)
{
    SDL_SetTextureBlendMode(&texture, SDL_BLENDMODE_BLEND);

#if SDL_VERSION_ATLEAST(2, 0, 12)
    SDL_SetTextureScaleMode(&texture, SDL_ScaleModeNearest);
#endif
}

SDL_Texture* create_texture_from_text_surface(SDL_Surface& surface)
{
    SDL_Texture* const texture =
        SDL_CreateTextureFromSurface(io::g_sdl_renderer, &surface);

    if (texture) {
        configure_rendered_text_texture(*texture);
    }

    return texture;
}

SDL_Surface* make_text_surface(
    const TextRun& run,
    const Color& color,
    const Color& bg_color)
{
    SDL_Surface* const font_surface = font_surface_for_bg(bg_color);

    if (!font_surface) {
        return nullptr;
    }

    const int scale_factor = config::video_scale_factor();
    const int surface_w = run.advance_px * scale_factor;
    const int surface_h = config::gui_cell_px_h() * scale_factor;

    SDL_Surface* const surface =
        SDL_CreateRGBSurfaceWithFormat(
            0,
            surface_w,
            surface_h,
            font_surface->format->BitsPerPixel,
            font_surface->format->format);

    if (!surface) {
        return nullptr;
    }

    const Color transparent_src_color = colors::magenta();
    const Uint32 transparent =
        SDL_MapRGB(
            surface->format,
            transparent_src_color.r(),
            transparent_src_color.g(),
            transparent_src_color.b());

    SDL_FillRect(surface, nullptr, transparent);
    SDL_SetColorKey(surface, SDL_TRUE, transparent);

    const Color color_adapted = color.with_brightness(config::brightness_pct());

    int pen_x = 0;

    for (const TextGlyph& glyph : run.glyphs) {
        const auto glyph_data = io::glyph_draw_data(glyph.codepoint);
        const SDL_Rect& src_rect = glyph_data.source_rect;

        for (int x = 0; x < src_rect.w; ++x) {
            for (int y = 0; y < src_rect.h; ++y) {
                const P src_pos(src_rect.x + x, src_rect.y + y);

                const Color src_color =
                    io::read_px_on_surface(*font_surface, src_pos);

                if (src_color == transparent_src_color) {
                    continue;
                }

                const P dst_pos(
                    pen_x + glyph_data.render_offset_x + x,
                    glyph_data.render_offset_y + y);

                if ((dst_pos.x < 0) ||
                    (dst_pos.y < 0) ||
                    (dst_pos.x >= run.advance_px) ||
                    (dst_pos.y >= config::gui_cell_px_h())) {
                    continue;
                }

                const Uint32 px =
                    SDL_MapRGBA(
                        surface->format,
                        (Uint8)((src_color.r() * color_adapted.r()) / 255),
                        (Uint8)((src_color.g() * color_adapted.g()) / 255),
                        (Uint8)((src_color.b() * color_adapted.b()) / 255),
                        255);

                put_scaled_px(*surface, dst_pos, scale_factor, px);
            }
        }

        pen_x += glyph.advance_px;
    }

    return surface;
}

SDL_Texture* make_text_texture_with_surface(
    const TextRun& run,
    const Color& color,
    const Color& bg_color,
    int& texture_w,
    int& texture_h)
{
    SDL_Surface* const surface = make_text_surface(run, color, bg_color);

    if (!surface) {
        return nullptr;
    }

    SDL_Texture* const texture = create_texture_from_text_surface(*surface);

    if (texture) {
        texture_w = surface->w;
        texture_h = surface->h;
    }

    SDL_FreeSurface(surface);

    return texture;
}

SDL_Texture* make_text_texture_with_render_target(
    const TextRun& run,
    const Color& color,
    const Color& bg_color,
    int& texture_w,
    int& texture_h)
{
    SDL_RendererInfo renderer_info {};

    if ((SDL_GetRendererInfo(io::g_sdl_renderer, &renderer_info) != 0) ||
        !(renderer_info.flags & SDL_RENDERER_TARGETTEXTURE)) {
        return nullptr;
    }

    const int scale_factor = config::video_scale_factor();
    texture_w = run.advance_px * scale_factor;
    texture_h = config::gui_cell_px_h() * scale_factor;

    SDL_Texture* const texture =
        SDL_CreateTexture(
            io::g_sdl_renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            texture_w,
            texture_h);

    if (!texture) {
        return nullptr;
    }

    configure_rendered_text_texture(*texture);

    SDL_Texture* const prev_target = SDL_GetRenderTarget(io::g_sdl_renderer);

    SDL_Rect prev_clip {};
    const SDL_bool was_clip_enabled =
        SDL_RenderIsClipEnabled(io::g_sdl_renderer);
    SDL_RenderGetClipRect(io::g_sdl_renderer, &prev_clip);

    Uint8 prev_r = 0;
    Uint8 prev_g = 0;
    Uint8 prev_b = 0;
    Uint8 prev_a = 0;
    SDL_GetRenderDrawColor(
        io::g_sdl_renderer,
        &prev_r,
        &prev_g,
        &prev_b,
        &prev_a);

    SDL_BlendMode prev_blend_mode = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(io::g_sdl_renderer, &prev_blend_mode);

    const P prev_rendering_px_offset = io::g_rendering_px_offset;

    SDL_SetRenderTarget(io::g_sdl_renderer, texture);
    SDL_RenderSetClipRect(io::g_sdl_renderer, nullptr);
    SDL_SetRenderDrawBlendMode(io::g_sdl_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(io::g_sdl_renderer, 0, 0, 0, 0);
    SDL_RenderClear(io::g_sdl_renderer);

    io::g_rendering_px_offset = {};

    draw_text_glyphs_uncached(
        run,
        {},
        color,
        io::DrawBg::no,
        bg_color,
        true);

    io::g_rendering_px_offset = prev_rendering_px_offset;

    SDL_SetRenderTarget(io::g_sdl_renderer, prev_target);
    SDL_RenderSetClipRect(
        io::g_sdl_renderer,
        was_clip_enabled ? &prev_clip : nullptr);
    SDL_SetRenderDrawBlendMode(io::g_sdl_renderer, prev_blend_mode);
    SDL_SetRenderDrawColor(io::g_sdl_renderer, prev_r, prev_g, prev_b, prev_a);

    io::clear_texture_color_mod_cache();

    return texture;
}

void draw_text_glyphs_uncached(
    const TextRun& run,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color,
    const bool msg_w_fit_on_screen)
{
    const int cell_px_w = config::gui_cell_px_w();
    const int screen_px_w = io::panel_px_w(Panel::screen);

    const SDL_Color sdl_color = color.sdl_color();
    const SDL_Color sdl_bg_color = bg_color.sdl_color();

    const Color sdl_color_gray = colors::gray();

    // X position to start drawing dots ("(..)") instead when the message
    // does not fit on the screen horizontally.
    const char dots[] = "(...)";
    size_t dots_idx = 0;
    const int px_x_dots = screen_px_w - (cell_px_w * 5);

    for (const TextGlyph& glyph : run.glyphs) {
        if (px_pos.x < 0 || px_pos.x >= screen_px_w) {
            return;
        }

        const bool draw_dots =
            !msg_w_fit_on_screen &&
            (px_pos.x >= px_x_dots);

        if (draw_dots) {
            draw_character_at_px(
                dots[dots_idx],
                px_pos,
                sdl_color_gray,
                draw_bg,
                bg_color);

            ++dots_idx;
            px_pos.x += cell_px_w;
        }
        else {
            // Whole message fits, or we are not yet near the edge
            draw_glyph_at_px(
                glyph.codepoint,
                px_pos,
                sdl_color,
                draw_bg,
                sdl_bg_color);

            px_pos.x += glyph.advance_px;
        }
    }
}

const RenderedText* find_rendered_text(
    const std::string& str,
    const Color& color,
    const Color& bg_color)
{
    const RenderedTextKey key {
        str,
        color,
        use_contour_font_surface(bg_color),
        config::video_scale_factor(),
        config::brightness_pct()};

    if (const auto it = s_rendered_text_cache.find(key);
        it != std::end(s_rendered_text_cache)) {
        return &it->second;
    }

    return nullptr;
}

const RenderedText* make_rendered_text(
    const std::string& str,
    const TextRun& run,
    const Color& color,
    const Color& bg_color)
{
    if (!io::g_sdl_renderer || str.empty() || run.advance_px <= 0) {
        return nullptr;
    }

    if (s_rendered_text_cache.size() >= max_rendered_text_cache_entries) {
        clear_rendered_text_cache();
    }

    int texture_w = 0;
    int texture_h = 0;

    SDL_Texture* texture =
        make_text_texture_with_render_target(
            run,
            color,
            bg_color,
            texture_w,
            texture_h);

    if (!texture) {
        texture =
            make_text_texture_with_surface(
                run,
                color,
                bg_color,
                texture_w,
                texture_h);
    }

    if (!texture) {
        return nullptr;
    }

    const RenderedTextKey key {
        str,
        color,
        use_contour_font_surface(bg_color),
        config::video_scale_factor(),
        config::brightness_pct()};
    const auto inserted =
        s_rendered_text_cache.emplace(
            key,
            RenderedText {texture, texture_w, texture_h});

    return &inserted.first->second;
}

bool draw_cached_text_at_px(
    const std::string& str,
    const TextRun& run,
    P px_pos,
    const Color& color,
    const Color& bg_color,
    const bool msg_w_fit_on_screen)
{
    if (!run.has_non_ascii || !msg_w_fit_on_screen || (px_pos.x < 0)) {
        return false;
    }

    const auto* rendered = find_rendered_text(str, color, bg_color);

    if (!rendered) {
        rendered = make_rendered_text(str, run, color, bg_color);
    }

    if (!rendered) {
        return false;
    }

    const int scale_factor = config::video_scale_factor();

    px_pos = px_pos.scaled_up(scale_factor);
    px_pos = px_pos.with_offsets(io::g_rendering_px_offset);

    const SDL_Rect render_rect {
        px_pos.x,
        px_pos.y,
        rendered->w,
        rendered->h};

    SDL_RenderCopy(io::g_sdl_renderer, rendered->texture, nullptr, &render_rect);

    return true;
}
}  // namespace

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace io
{
int text_advance_px(const std::string& str)
{
    return text_run(str).advance_px;
}

void clear_text_width_cache()
{
    s_text_run_cache.clear();
    clear_rendered_text_cache();
}

void draw_text_at_px(
    const std::string& str,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color)
{
    if ((px_pos.y < 0) || (px_pos.y >= panel_px_h(Panel::screen))) {
        return;
    }

    const TextRun& run = text_run(str);
    const int msg_px_w = run.advance_px;

    const int screen_px_w = panel_px_w(Panel::screen);
    const int msg_px_x1 = px_pos.x + msg_px_w - 1;
    const bool msg_w_fit_on_screen = msg_px_x1 < screen_px_w;

    if ((draw_bg == io::DrawBg::no) &&
        draw_cached_text_at_px(
            str,
            run,
            px_pos,
            color,
            bg_color,
            msg_w_fit_on_screen)) {
        return;
    }

    draw_text_glyphs_uncached(
        run,
        px_pos,
        color,
        draw_bg,
        bg_color,
        msg_w_fit_on_screen);
}

void draw_text(
    Text text,
    const Panel panel,
    P pos,
    Color color,
    const DrawBg draw_bg,
    const Color& bg_color)
{
    text.set_color(color);

    const P line_start_px = gui_to_px_coords(panel, pos);
    P px_pos = line_start_px;

    for (const TextAction& action : text.actions()) {
        switch (action.id) {
        case TextActionId::write_str: {
            draw_text_at_px(
                action.str,
                px_pos,
                color,
                draw_bg,
                bg_color);

            px_pos.x += text_advance_px(action.str);
        } break;

        case TextActionId::newline: {
            px_pos.x = line_start_px.x;
            px_pos.y += config::gui_cell_px_h();
        } break;

        case TextActionId::change_color: {
            color = action.color;
        } break;

        case TextActionId::done: {
            return;
        } break;
        }
    }
}

void draw_text_plain(
    const std::string& str,
    const Panel panel,
    const P pos,
    const Color& color,
    const DrawBg draw_bg,
    const Color& bg_color)
{
    draw_text_at_px(
        str,
        gui_to_px_coords(panel, pos),
        color,
        draw_bg,
        bg_color);
}

void draw_text_plain_at_px(
    const std::string& str,
    const P px_pos,
    const Color& color,
    const DrawBg draw_bg,
    const Color& bg_color)
{
    draw_text_at_px(str, px_pos, color, draw_bg, bg_color);
}

void draw_text_plain_center(
    const std::string& str,
    const Panel panel,
    const P pos,
    const Color& color,
    const DrawBg draw_bg,
    const Color& bg_color,
    const bool is_pixel_pos_adj_allowed)
{
    const int text_px_w = text_advance_px(str);
    P px_pos = gui_to_px_coords(panel, pos);
    px_pos.x -= text_px_w / 2;

    if (is_pixel_pos_adj_allowed) {
        const int pixel_x_adj =
            (text_px_w % 2) == 0
            ? (config::gui_cell_px_w() / 2)
            : 0;

        px_pos += P(pixel_x_adj, 0);
    }

    draw_text_at_px(str, px_pos, color, draw_bg, bg_color);
}

void draw_text_plain_right(
    const std::string& str,
    const Panel panel,
    const P pos,
    const Color& color,
    const DrawBg draw_bg,
    const Color& bg_color)
{
    P px_pos = gui_to_px_coords(panel, pos);
    px_pos.x += config::gui_cell_px_w() - text_advance_px(str);

    draw_text_at_px(str, px_pos, color, draw_bg, bg_color);
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
    draw_text_plain_center(
        str,
        panel,
        pos,
        color,
        draw_bg,
        bg_color,
        is_pixel_pos_adj_allowed);
}

void draw_text_right(
    const std::string& str,
    const Panel panel,
    const P pos,
    const Color& color,
    const DrawBg draw_bg,
    const Color& bg_color)
{
    draw_text_plain_right(str, panel, pos, color, draw_bg, bg_color);
}

}  // namespace io
