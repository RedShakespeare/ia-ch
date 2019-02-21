// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef THROWING_HPP
#define THROWING_HPP


namespace item
{

class Item;

} // item


namespace actor
{

class Actor;

} // actor


struct P;


namespace throwing
{

void throw_item(
        actor::Actor& actor_throwing,
        const P& tgt_pos,
        item::Item& item_thrown);

void player_throw_lit_explosive(const P& aim_cell);

} // Throwing

#endif // THROWING_HPP
