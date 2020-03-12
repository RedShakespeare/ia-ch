// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef HINTS_HPP
#define HINTS_HPP

namespace hints
{

enum Id
{
        altars,
        fountains,
        destroying_corpses,
        unload_weapons,
        infected,
        overburdened,
        high_shock,

        END
};


void init();

void save();

void load();

void display(Id id);

} // namespace hints

#endif // HINTS_HPP
