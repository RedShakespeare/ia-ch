// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef DROP_HPP
#define DROP_HPP

#include "global.hpp"


namespace item
{
class Item;
}

namespace actor
{
class Actor;
}

struct P;


namespace item_drop
{

// This function places the item as close to the origin as possible, but never
// on top of other items, unless they can be stacked.
item::Item* drop_item_on_map(const P& intended_pos, item::Item& item);

void drop_item_from_inv(
        actor::Actor& actor,
        const InvType inv_type,
        const size_t idx,
        const int nr_items_to_drop = -1);

} // item_drop

#endif
