// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MAP_RENDER_BATCH_HPP
#define MAP_RENDER_BATCH_HPP

#include <cstdint>

#include "SDL_rect.h"

// Forward declarations
struct SDL_Texture;
struct Color;

// -----------------------------------------------------------------------------
// Map render batch system
// -----------------------------------------------------------------------------
// This system batches map rendering calls. Commands are replayed in insertion
// order because map rendering uses later layers to overwrite earlier layers.
//
// Usage:
//   1. Call begin_frame() at start of draw_map::run()
//   2. Replace immediate SDL_RenderCopy calls with add_tile()/add_character()
//   3. Call flush() at end of draw_map::run() to render all batches
// -----------------------------------------------------------------------------
namespace map_render_batch
{

// Prepare for a new frame of batched rendering
void begin_frame();

// Add a tile rendering command to the batch
void add_tile(SDL_Texture* texture,
              const SDL_Rect* src_rect,
              const SDL_Rect& dst_rect,
              const Color& color);

// Add a character rendering command to the batch
void add_character(SDL_Texture* texture,
                   const SDL_Rect& src_rect,
                   const SDL_Rect& dst_rect,
                   const Color& color);

// Add a filled rectangle command to the batch
void add_filled_rect(
    const SDL_Rect& rect,
    const Color& color,
    uint8_t alpha);

// Render all queued commands in insertion order.
void flush();

// Check if batching is currently active
bool is_active();

}  // namespace map_render_batch

#endif  // MAP_RENDER_BATCH_HPP
