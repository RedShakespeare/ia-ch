// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "map_render_batch.hpp"

#include <vector>

#include "SDL.h"
#include "SDL_render.h"
#include "colors.hpp"
#include "io_internal.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
namespace
{

enum class DrawCommandType
{
    texture,
    filled_rect,
};

struct DrawCommand
{
    DrawCommandType type {DrawCommandType::texture};
    SDL_Texture* texture {};
    SDL_Rect src_rect {};
    SDL_Rect dst_rect {};
    Color color {};
    uint8_t alpha {SDL_ALPHA_OPAQUE};
    bool has_src_rect {false};
};

static std::vector<DrawCommand> s_draw_commands;
static bool s_is_batching_active = false;

}  // namespace

// -----------------------------------------------------------------------------
// map_render_batch
// -----------------------------------------------------------------------------
namespace map_render_batch
{

void begin_frame()
{
    s_draw_commands.clear();
    // Reserve capacity for typical frame (~1000 draw calls)
    // This avoids repeated reallocations during the frame
    if (s_draw_commands.capacity() < 1024) {
        s_draw_commands.reserve(1024);
    }
    s_is_batching_active = true;
}

void add_tile(SDL_Texture* texture,
              const SDL_Rect* src_rect,
              const SDL_Rect& dst_rect,
              const Color& color)
{
    if (!s_is_batching_active) {
        return;
    }

    DrawCommand cmd;
    cmd.type = DrawCommandType::texture;
    cmd.texture = texture;
    cmd.has_src_rect = (src_rect != nullptr);
    cmd.src_rect = src_rect ? *src_rect : SDL_Rect{0, 0, 0, 0};
    cmd.dst_rect = dst_rect;
    cmd.color = color;

    s_draw_commands.push_back(cmd);
}

void add_character(SDL_Texture* texture,
                   const SDL_Rect& src_rect,
                   const SDL_Rect& dst_rect,
                   const Color& color)
{
    if (!s_is_batching_active) {
        return;
    }

    DrawCommand cmd;
    cmd.type = DrawCommandType::texture;
    cmd.texture = texture;
    cmd.has_src_rect = true;
    cmd.src_rect = src_rect;
    cmd.dst_rect = dst_rect;
    cmd.color = color;

    s_draw_commands.push_back(cmd);
}

void add_filled_rect(
    const SDL_Rect& rect,
    const Color& color,
    const uint8_t alpha)
{
    if (!s_is_batching_active) {
        return;
    }

    DrawCommand cmd;
    cmd.type = DrawCommandType::filled_rect;
    cmd.dst_rect = rect;
    cmd.color = color;
    cmd.alpha = alpha;

    s_draw_commands.push_back(cmd);
}

void flush()
{
    if (!s_is_batching_active) {
        return;
    }

    s_is_batching_active = false;

    if (s_draw_commands.empty()) {
        return;
    }

    // If SDL renderer is not initialized, skip rendering
    // (this happens in test builds)
    if (!io::g_sdl_renderer) {
        s_draw_commands.clear();
        return;
    }

    // Preserve insertion order. Map cells depend on strict draw order: terrain, items,
    // actors, and overlays intentionally overwrite earlier layers.
    SDL_Texture* current_texture = nullptr;
    Color current_color = colors::black();

    io::set_clip_rect_to_panel(Panel::map);

    for (const auto& cmd : s_draw_commands) {
        if (cmd.type == DrawCommandType::filled_rect) {
            SDL_SetRenderDrawColor(
                io::g_sdl_renderer,
                cmd.color.r(),
                cmd.color.g(),
                cmd.color.b(),
                cmd.alpha);

            SDL_RenderFillRect(io::g_sdl_renderer, &cmd.dst_rect);

            continue;
        }

        // Set texture color mod only when texture or color changes.
        if (cmd.texture != current_texture || cmd.color != current_color) {
            io::set_texture_color_mod_if_needed(cmd.texture, cmd.color);

            current_texture = cmd.texture;
            current_color = cmd.color;
        }

        // Render the command
        const SDL_Rect* src_ptr = cmd.has_src_rect ? &cmd.src_rect : nullptr;

        SDL_RenderCopy(io::g_sdl_renderer, cmd.texture, src_ptr, &cmd.dst_rect);
    }

    io::disable_clip_rect();

    s_draw_commands.clear();
}

bool is_active()
{
    return s_is_batching_active;
}

}  // namespace map_render_batch
