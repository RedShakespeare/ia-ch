// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef RELOAD_HPP
#define RELOAD_HPP

class Actor;
class Item;

namespace reload
{

void try_reload(Actor& actor, Item* const item_to_reload);

void player_arrange_pistol_mags();

} //reload

#endif
