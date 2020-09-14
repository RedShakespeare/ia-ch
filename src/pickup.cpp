// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
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
#include "state.hpp"

namespace item_pickup
{
void try_pick()
{
        msg_log::clear();

        const P& pos = map::g_player->m_pos;

        auto* const item = map::g_cells.at(pos).item;

        if (!item)
        {
                msg_log::add("I see nothing to pick up here.");

                return;
        }

        const auto pre_pickup_result = item->pre_pickup_hook();

        auto& cell = map::g_cells.at(pos);

        switch (pre_pickup_result)
        {
        case ItemPrePickResult::do_pickup:
        {
                audio::play(audio::SfxId::pickup);

                const std::string item_name = item->name(ItemRefType::plural);

                msg_log::add("I pick up " + item_name + ".");

                // NOTE: This may destroy the item (e.g. combine with others)
                map::g_player->m_inv.put_in_backpack(item);

                cell.item = nullptr;
        }
        break;

        case ItemPrePickResult::destroy_item:
        {
                delete item;
                cell.item = nullptr;
        }
        break;

        case ItemPrePickResult::do_nothing:
        {
        }
        break;
        }

        // NOTE: The player might have won the game by picking up the
        // Trapezohedron, if so do not tick time
        if (states::contains_state(StateId::game))
        {
                game_time::tick();
        }
}

item::Ammo* unload_ranged_wpn(item::Wpn& wpn)
{
        ASSERT(!wpn.data().ranged.has_infinite_ammo);

        const int nr_ammo_loaded = wpn.m_ammo_loaded;

        if (nr_ammo_loaded == 0)
        {
                return nullptr;
        }

        const auto ammo_id = wpn.data().ranged.ammo_item_id;

        auto& ammo_data = item::g_data[(size_t)ammo_id];

        auto* spawned_ammo = item::make(ammo_id);

        if (ammo_data.type == ItemType::ammo_mag)
        {
                // Unload a mag
                static_cast<item::AmmoMag*>(spawned_ammo)->m_ammo =
                        nr_ammo_loaded;
        }
        else
        {
                // Unload loose ammo
                spawned_ammo->m_nr_items = nr_ammo_loaded;
        }

        wpn.m_ammo_loaded = 0;

        return static_cast<item::Ammo*>(spawned_ammo);
}

void try_unload_or_pick()
{
        auto* item = map::g_cells.at(map::g_player->m_pos).item;

        if (item &&
            item->data().ranged.is_ranged_wpn &&
            !item->data().ranged.has_infinite_ammo)
        {
                auto* const wpn = static_cast<item::Wpn*>(item);

                auto* const spawned_ammo = unload_ranged_wpn(*wpn);

                if (spawned_ammo)
                {
                        audio::play(audio::SfxId::pickup);

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

}  // namespace item_pickup
