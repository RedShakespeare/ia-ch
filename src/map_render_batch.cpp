// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "map_render_batch.hpp"

#include <algorithm>
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

struct DrawCommand
{
    SDL_Texture* texture;
    SDL_Rect src_rect;
    SDL_Rect dst_rect;
    Color color;
    bool has_src_rect;  // true if src_rect is valid, false if should be nullptr

    // Sorting key: order by texture pointer first, then by color components
    bool operator<(const DrawCommand& other) const
    {
        if (texture != other.texture) {
            return texture < other.texture;
        }

        if (color.r() != other.color.r()) {
            return color.r() < other.color.r();
        }

        if (color.g() != other.color.g()) {
            return color.g() < other.color.g();
        }

        return color.b() < other.color.b();
    }
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
    cmd.texture = texture;
    cmd.has_src_rect = true;
    cmd.src_rect = src_rect;
    cmd.dst_rect = dst_rect;
    cmd.color = color;

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

    // Sort commands by (texture, color) to minimize state changes
    std::sort(s_draw_commands.begin(), s_draw_commands.end());

    // Render all commands in batches
    SDL_Texture* current_texture = nullptr;
    Color current_color = colors::black();

    for (const auto& cmd : s_draw_commands) {
        // Set texture color mod only when texture or color changes
        if (cmd.texture != current_texture || cmd.color != current_color) {
            SDL_SetTextureColorMod(
                cmd.texture,
                cmd.color.r(),
                cmd.color.g(),
                cmd.color.b());

            current_texture = cmd.texture;
            current_color = cmd.color;
        }

        // Render the command
        const SDL_Rect* src_ptr = cmd.has_src_rect ? &cmd.src_rect : nullptr;

        SDL_RenderCopy(io::g_sdl_renderer, cmd.texture, src_ptr, &cmd.dst_rect);
    }

    s_draw_commands.clear();
}

bool is_active()
{
    return s_is_batching_active;
}

}  // namespace map_render_batch
