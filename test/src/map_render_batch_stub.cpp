// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "map_render_batch.hpp"

// Stub implementation for test builds (no SDL available)
// These are no-ops that allow tests to compile and link

namespace map_render_batch
{

void begin_frame()
{
    // No-op for tests
}

void add_tile(SDL_Texture*, const SDL_Rect*, const SDL_Rect&, const Color&)
{
    // No-op for tests
}

void add_character(SDL_Texture*, const SDL_Rect&, const SDL_Rect&, const Color&)
{
    // No-op for tests
}

void flush()
{
    // No-op for tests
}

bool is_active()
{
    return false;
}

}  // namespace map_render_batch
