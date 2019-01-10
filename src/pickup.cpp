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

        const P& pos = map::player->pos;

        Item* const item = map::cells.at(pos).item;

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
        map::player->inv.put_in_backpack(item);

        map::cells.at(pos).item = nullptr;

        game_time::tick();
}

Ammo* unload_ranged_wpn(Wpn& wpn)
{
        ASSERT(!wpn.data().ranged.has_infinite_ammo);

        const int nr_ammo_loaded = wpn.ammo_loaded_;

        if (nr_ammo_loaded == 0)
        {
                return nullptr;
        }

        const ItemId ammo_id = wpn.data().ranged.ammo_item_id;

        ItemData& ammo_data = item_data::data[(size_t)ammo_id];

        Item* spawned_ammo = item_factory::make(ammo_id);

        if (ammo_data.type == ItemType::ammo_mag)
        {
                // Unload a mag
                static_cast<AmmoMag*>(spawned_ammo)->ammo_ = nr_ammo_loaded;
        }
        else
        {
                // Unload loose ammo
                spawned_ammo->nr_items_ = nr_ammo_loaded;
        }

        wpn.ammo_loaded_ = 0;

        return static_cast<Ammo*>(spawned_ammo);
}

void try_unload_wpn_or_pickup_ammo()
{
        Item* item = map::cells.at(map::player->pos).item;

        if (!item)
        {
                msg_log::add("I see no ammo to unload or pick up here.");

                return;
        }

        if (item->data().ranged.is_ranged_wpn)
        {
                Wpn* const wpn = static_cast<Wpn*>(item);

                const std::string wpn_name =
                        wpn->name(ItemRefType::a, ItemRefInf::yes);

                if (!wpn->data().ranged.has_infinite_ammo)
                {
                        Ammo* const spawned_ammo =
                                unload_ranged_wpn(*wpn);

                        if (spawned_ammo)
                        {
                                audio::play(SfxId::pickup);

                                msg_log::add("I unload " + wpn_name + ".");

                                map::player->inv.put_in_backpack(spawned_ammo);

                                game_time::tick();

                                return;
                        }
                }
        }
        // Not a ranged weapon
        else if (item->data().type == ItemType::ammo ||
                 item->data().type == ItemType::ammo_mag)
        {
                try_pick();

                return;
        }
}

} // item_pickup
