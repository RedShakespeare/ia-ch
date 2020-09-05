// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

struct P;
struct R;

namespace viewport
{
R get_map_view_area();

void focus_on( P map_pos );

bool is_in_view( P map_pos );

P to_view_pos( P map_pos );

P to_map_pos( P view_pos );

}  // namespace viewport

#endif  // VIEWPORT_HPP
