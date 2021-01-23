// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "catch.hpp"

#include "map.hpp"
#include "spells.hpp"
#include "terrain.hpp"
#include "terrain_door.hpp"
#include "test_utils.hpp"

TEST_CASE("Test opening spell effect")
{
        test_utils::init_all();

        const P wood_door_pos(3, 3);
        const P metal_door_pos(10, 10);
        const P lever_1_pos(50, 50);
        const P lever_2_pos(75, 75);

        auto* const wood_door =
                new terrain::Door(
                        wood_door_pos,
                        nullptr,
                        terrain::DoorType::wood,
                        terrain::DoorSpawnState::closed);

        auto* const metal_door =
                new terrain::Door(
                        metal_door_pos,
                        nullptr,
                        terrain::DoorType::metal,
                        terrain::DoorSpawnState::closed);

        auto* const lever_1 = new terrain::Lever(lever_1_pos);
        auto* const lever_2 = new terrain::Lever(lever_2_pos);

        map::put(wood_door);
        map::put(metal_door);
        map::put(lever_1);
        map::put(lever_2);

        lever_1->set_linked_terrain(*metal_door);
        lever_2->set_linked_terrain(*metal_door);
        lever_1->add_sibbling(lever_2);
        lever_2->add_sibbling(lever_1);

        REQUIRE(!wood_door->is_open());
        REQUIRE(!metal_door->is_open());
        REQUIRE(lever_1->is_left_pos());
        REQUIRE(lever_2->is_left_pos());

        const auto did_open_wood_door =
                spells::run_opening_spell_effect_at(
                        wood_door_pos,
                        100,  // 100% chance
                        SpellSkill::master);

        REQUIRE(did_open_wood_door == terrain::DidOpen::yes);

        REQUIRE(wood_door->is_open());
        REQUIRE(!metal_door->is_open());
        REQUIRE(lever_1->is_left_pos());
        REQUIRE(lever_2->is_left_pos());

        const auto did_open_metal_door =
                spells::run_opening_spell_effect_at(
                        metal_door_pos,
                        100,  // 100% chance
                        SpellSkill::master);

        REQUIRE(did_open_metal_door == terrain::DidOpen::yes);

        REQUIRE(wood_door->is_open());
        REQUIRE(metal_door->is_open());
        REQUIRE(!lever_1->is_left_pos());
        REQUIRE(!lever_2->is_left_pos());
}
