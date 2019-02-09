// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef THROWING_HPP
#define THROWING_HPP

class Item;
class Actor;
struct P;

namespace throwing
{

void throw_item(
        Actor& actor_throwing,
        const P& tgt_pos,
        Item& item_thrown);

void player_throw_lit_explosive(const P& aim_cell);

} //Throwing

#endif
