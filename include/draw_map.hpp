// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef DRAW_MAP_HPP
#define DRAW_MAP_HPP

#include "global.hpp"
#include "colors.hpp"
#include "gfx.hpp"

struct CellRenderData;

namespace draw_map
{

void clear();

void run();

const CellRenderData& get_drawn_cell(int x, int y);

const CellRenderData& get_drawn_cell_player_memory(int x, int y);

} // draw_map

#endif // DRAW_MAP_HPP
