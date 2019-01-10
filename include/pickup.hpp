// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef PICKUP_HPP
#define PICKUP_HPP

class Ammo;
class Wpn;

namespace item_pickup
{

//Can always be called to check if something is there to be picked up.
void try_pick();

void try_unload_wpn_or_pickup_ammo();

Ammo* unload_ranged_wpn(Wpn& wpn);

} //item_pickup

#endif
