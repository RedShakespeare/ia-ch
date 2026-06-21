// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>

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

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace io
{
int text_advance_px(const std::string& str)
{
    int w = 0;

    for (size_t i = 0; i < str.size();) {
        const size_t cp_size = utf8::codepoint_size(str, i);
        w += glyph_advance_px(str.substr(i, cp_size));
        i += cp_size;
    }

    return w;
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
    const int msg_px_w = text_advance_px(str);

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

    for (size_t i = 0; i < str.size();) {
        if (px_pos.x < 0 || px_pos.x >= screen_px_w) {
            return;
        }

        const bool draw_dots =
            !msg_w_fit_on_screen &&
            (px_pos.x >= px_x_dots);

        const size_t cp_size = utf8::codepoint_size(str, i);

        if (cp_size == 0) {
            break;
        }

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
            const std::string glyph = str.substr(i, cp_size);

            draw_glyph_at_px(
                glyph,
                px_pos,
                sdl_color,
                draw_bg,
                sdl_bg_color);

            px_pos.x += glyph_advance_px(glyph);
        }

        i += cp_size;
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
