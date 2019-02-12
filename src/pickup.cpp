// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "pickup.hpp"

#include <string>

#include "actor_player.hpp"
#include "audio.hpp"
#include "drop.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "item.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "query.hpp"

namespace item_pickup
{

void try_pick()
{
        msg_log::clear();

        const P& pos = map::g_player->m_pos;

        Item* const item = map::g_cells.at(pos).item;

        if (!item)
        {
                msg_log::add("I see nothing to pick up here.");

                return;
        }

        const std::string item_name = item->name(ItemRefType::plural);

        audio::play(SfxId::pickup);

        msg_log::add("I pick up " + item_name + ".");

        // NOTE: This calls the items pickup hook, which may destroy the item
        // (e.g. combine with others)
        map::g_player->m_inv.put_in_backpack(item);

        map::g_cells.at(pos).item = nullptr;

        game_time::tick();
}

Ammo* unload_ranged_wpn(Wpn& wpn)
{
        ASSERT(!wpn.data().ranged.has_infinite_ammo);

        const int nr_ammo_loaded = wpn.m_ammo_loaded;

        if (nr_ammo_loaded == 0)
        {
                return nullptr;
        }

        const ItemId ammo_id = wpn.data().ranged.ammo_item_id;

        ItemData& ammo_data = item_data::g_data[(size_t)ammo_id];

        Item* spawned_ammo = item_factory::make(ammo_id);

        if (ammo_data.type == ItemType::ammo_mag)
        {
                // Unload a mag
                static_cast<AmmoMag*>(spawned_ammo)->m_ammo = nr_ammo_loaded;
        }
        else
        {
                // Unload loose ammo
                spawned_ammo->m_nr_items = nr_ammo_loaded;
        }

        wpn.m_ammo_loaded = 0;

        return static_cast<Ammo*>(spawned_ammo);
}

void try_unload_or_pick()
{
        Item* item = map::g_cells.at(map::g_player->m_pos).item;

        if (item &&
            item->data().ranged.is_ranged_wpn &&
            !item->data().ranged.has_infinite_ammo)
        {
                Wpn* const wpn = static_cast<Wpn*>(item);

                Ammo* const spawned_ammo = unload_ranged_wpn(*wpn);

                ASSERT(spawned_ammo);

                if (spawned_ammo)
                {
                        audio::play(SfxId::pickup);

                        const std::string name_a =
                                item->name(ItemRefType::a, ItemRefInf::yes);

                        msg_log::add("I unload " + name_a + ".");

                        map::g_player->m_inv.put_in_backpack(spawned_ammo);

                        game_time::tick();

                        return;
                }
        }

        // Did not unload a ranged weapon, run the normal item picking instead

        try_pick();
}

} // item_pickup
