// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef RELOAD_HPP
#define RELOAD_HPP


namespace item
{
class Item;
}

namespace actor
{
class Actor;
}


namespace reload
{

void try_reload(actor::Actor& actor, item::Item* const item_to_reload);

void player_arrange_pistol_mags();

} //reload

#endif
