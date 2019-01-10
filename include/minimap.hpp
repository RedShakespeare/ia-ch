// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MINIMAP_HPP
#define MINIMAP_HPP

#include "colors.hpp"
#include "state.hpp"

// -----------------------------------------------------------------------------
// ViewMinimap
// -----------------------------------------------------------------------------
class ViewMinimap : public State
{
public:
        ViewMinimap() :
                State() {}

        StateId id() override
        {
                return StateId::view_minimap;
        }

        void draw() override;

        void update() override;
};

// -----------------------------------------------------------------------------
// minimap
// -----------------------------------------------------------------------------
namespace minimap
{

void clear();

void update();

}

#endif // MINIMAP_HPP
