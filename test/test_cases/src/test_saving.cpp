// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "catch.hpp"

#include "actor_player.hpp"
#include "game_time.hpp"
#include "inventory.hpp"
#include "item_data.hpp"
#include "item_device.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "map_travel.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "property.hpp"
#include "property_handler.hpp"
#include "saving.hpp"
#include "test_utils.hpp"

TEST_CASE("Saving and loading the game")
{
        // ---------------------------------------------------------------------
        // Saving
        // ---------------------------------------------------------------------
        {
                test_utils::init_all();

                // Item data
                item_data::g_data[(size_t)ItemId::scroll_telep]
                        .is_spell_domain_known = true;

                item_data::g_data[(size_t)ItemId::scroll_opening]
                        .is_identified = true;

                // Bonus
                player_bon::pick_bg(Bg::rogue);
                player_bon::g_traits[(size_t)Trait::healer] = true;

                // Player inventory
                auto& inv = map::g_player->m_inv;

                // First, remove all present items to get a clean state
                for (Item* item : inv.m_backpack)
                {
                        delete item;
                }

                inv.m_backpack.clear();

                for (size_t i = 0; i < (size_t)SlotId::END; ++i)
                {
                        auto& slot = inv.m_slots[i];

                        if (slot.item)
                        {
                                delete slot.item;
                                slot.item = nullptr;
                        }
                }

                // Put new items
                Item* item = nullptr;

                item = item_factory::make(ItemId::mi_go_gun);

                inv.put_in_slot(
                        SlotId::wpn,
                        item,
                        Verbosity::verbose);

                map::g_player->set_unarmed_wpn(
                        static_cast<Wpn*>(
                                item_factory::make(ItemId::player_punch)));

                // Wear asbestos suit to test properties from wearing items
                item = item_factory::make(ItemId::armor_asb_suit);

                inv.put_in_slot(
                        SlotId::body,
                        item,
                        Verbosity::verbose);

                item = item_factory::make(ItemId::pistol_mag);
                static_cast<AmmoMag*>(item)->m_ammo = 1;
                inv.put_in_backpack(item);

                item = item_factory::make(ItemId::pistol_mag);
                static_cast<AmmoMag*>(item)->m_ammo = 2;
                inv.put_in_backpack(item);

                item = item_factory::make(ItemId::pistol_mag);
                static_cast<AmmoMag*>(item)->m_ammo = 3;
                inv.put_in_backpack(item);

                item = item_factory::make(ItemId::pistol_mag);
                static_cast<AmmoMag*>(item)->m_ammo = 3;
                inv.put_in_backpack(item);

                item = item_factory::make(ItemId::device_blaster);
                static_cast<StrangeDevice*>(item)->condition =
                        Condition::shoddy;
                inv.put_in_backpack(item);

                item = item_factory::make(ItemId::lantern);

                DeviceLantern* lantern = static_cast<DeviceLantern*>(item);

                lantern->nr_turns_left = 789;
                lantern->is_activated = true;

                inv.put_in_backpack(item);

                // Player
                map::g_player->m_data->name_a = "TEST PLAYER";
                map::g_player->m_data->name_the = "THIS IS OVERWRITTEN";

                map::g_player->m_base_max_hp = 456;

                // map
                map::g_dlvl = 7;

                // Actor data
                actor_data::g_data[(size_t)ActorId::END - 1].nr_kills = 123;

                // Learned spells
                player_spells::learn_spell(
                        SpellId::bless,
                        Verbosity::silent);

                player_spells::learn_spell(
                        SpellId::aza_wrath,
                        Verbosity::silent);

                // Applied properties
                auto& props = map::g_player->m_properties;

                {
                        auto* const prop = new PropRSleep();
                        prop->set_duration(3);
                        props.apply(prop);
                }


                {
                        auto* const prop = new PropDiseased();
                        prop->set_indefinite();
                        props.apply(prop);
                }

                props.apply(new PropBlessed());

                REQUIRE(props.has(PropId::diseased));
                REQUIRE(props.has(PropId::blessed));
                REQUIRE(!props.has(PropId::confused));

                saving::save_game();

                REQUIRE(saving::is_save_available());

                test_utils::cleanup_all();
        }

        // ---------------------------------------------------------------------
        // Loading
        // ---------------------------------------------------------------------
        {
                test_utils::init_all();

                REQUIRE(saving::is_save_available());

                saving::load_game();

                // Item data
                REQUIRE(item_data::g_data[(size_t)ItemId::scroll_telep]
                        .is_spell_domain_known);

                REQUIRE(!item_data::g_data[(size_t)ItemId::scroll_telep]
                        .is_identified);

                REQUIRE(item_data::g_data[(size_t)ItemId::scroll_opening]
                        .is_identified);

                REQUIRE(!item_data::g_data[(size_t)ItemId::scroll_opening]
                        .is_spell_domain_known);

                REQUIRE(!item_data::g_data[(size_t)ItemId::scroll_searching]
                        .is_spell_domain_known);

                REQUIRE(!item_data::g_data[(size_t)ItemId::scroll_searching]
                        .is_identified);

                // Player
                REQUIRE(map::g_player->m_data->name_a == "TEST PLAYER");
                REQUIRE(map::g_player->m_data->name_the == "TEST PLAYER");

                // Check max HP (affected by disease)
                REQUIRE(actor::max_hp(*map::g_player) == 456 / 2);

                // Bonus
                REQUIRE(player_bon::bg() == Bg::rogue);

                REQUIRE(player_bon::g_traits[(size_t)Trait::healer]);

                REQUIRE(!player_bon::g_traits[(size_t)Trait::vigilant]);

                // Player inventory
                const auto& inv  = map::g_player->m_inv;

                REQUIRE(inv.m_backpack.size() == 6);

                REQUIRE(inv.item_in_slot(SlotId::wpn)->data().id ==
                        ItemId::mi_go_gun);

                REQUIRE(map::g_player->unarmed_wpn().id() ==
                        ItemId::player_punch);

                REQUIRE(inv.item_in_slot(SlotId::body)->data().id ==
                        ItemId::armor_asb_suit);

                int nr_mag_with_1 = 0;
                int nr_mag_with_2 = 0;
                int nr_mag_with_3 = 0;
                bool is_sentry_device_found = false;
                bool is_lantern_found = false;

                for (Item* item : inv.m_backpack)
                {
                        ItemId id = item->id();

                        if (id == ItemId::pistol_mag)
                        {
                                switch (static_cast<AmmoMag*>(item)->m_ammo)
                                {
                                case 1:
                                        ++nr_mag_with_1;
                                        break;

                                case 2:
                                        ++nr_mag_with_2;
                                        break;

                                case 3:
                                        ++nr_mag_with_3;
                                        break;

                                default:
                                        break;
                                }
                        }
                        else if (id == ItemId::device_blaster)
                        {
                                is_sentry_device_found = true;

                                const auto* const device =
                                        static_cast<StrangeDevice*>(item);

                                REQUIRE(device->condition == Condition::shoddy);
                        }
                        else if (id == ItemId::lantern)
                        {
                                is_lantern_found = true;

                                const auto* const lantern =
                                        static_cast<DeviceLantern*>(item);

                                REQUIRE(lantern->nr_turns_left == 789);

                                REQUIRE(lantern->is_activated);
                        }
                }

                REQUIRE(nr_mag_with_1 == 1);
                REQUIRE(nr_mag_with_2 == 1);
                REQUIRE(nr_mag_with_3 == 2);
                REQUIRE(is_sentry_device_found);
                REQUIRE(is_lantern_found);

                // map
                REQUIRE(map::g_dlvl == 7);

                // Actor data
                REQUIRE(actor_data::g_data[(int)ActorId::END - 1]
                        .nr_kills == 123);

                // Learned spells
                REQUIRE(player_spells::is_spell_learned(SpellId::bless));

                REQUIRE(player_spells::is_spell_learned(SpellId::aza_wrath));

                REQUIRE(!player_spells::is_spell_learned(SpellId::mayhem));

                // Properties
                const auto& props = map::g_player->m_properties;

                {
                        const auto* const prop = props.prop(PropId::diseased);

                        REQUIRE(prop);
                        REQUIRE(prop->nr_turns_left() == -1);
                }

                // Check currrent HP (should not be affected by disease)
                REQUIRE(map::g_player->m_hp == map::g_player->m_data->hp);

                {
                        const auto* const prop = props.prop(PropId::r_sleep);

                        REQUIRE(prop);
                        REQUIRE(prop->nr_turns_left() == 3);
                }

                {
                        const auto* const prop = props.prop(PropId::blessed);

                        REQUIRE(prop);
                        REQUIRE(prop->nr_turns_left() > 0);
                }

                // Properties from worn item
                {
                        const auto* const prop = props.prop(PropId::r_acid);

                        REQUIRE(prop);
                        REQUIRE(prop->nr_turns_left() == -1);
                }

                {
                        const auto* const prop = props.prop(PropId::r_fire);

                        REQUIRE(prop);
                        REQUIRE(prop->nr_turns_left() == -1);
                }

                // Turn number
                REQUIRE(game_time::turn_nr() == 0);

                test_utils::cleanup_all();
        }
}
