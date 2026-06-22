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
    std::vector<TextGlyph> glyphs;
};

std::unordered_map<std::string, TextRun> s_text_run_cache;
constexpr size_t max_text_run_cache_entries = 4096;

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
        run.glyphs.push_back({codepoint, advance_px});

        i += cp_size;
    }

    const auto inserted = s_text_run_cache.emplace(str, std::move(run));
    return inserted.first->second;
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

    const int cell_px_w = config::gui_cell_px_w();
    const TextRun& run = text_run(str);
    const int msg_px_w = run.advance_px;

    const SDL_Color sdl_color = color.sdl_color();
    const SDL_Color sdl_bg_color = bg_color.sdl_color();

    const Color sdl_color_gray = colors::gray();

    const int screen_px_w = panel_px_w(Panel::screen);
    const int msg_px_x1 = px_pos.x + msg_px_w - 1;
    const bool msg_w_fit_on_screen = msg_px_x1 < screen_px_w;

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

void draw_text_center(
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

void draw_text_right(
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

}  // namespace io
