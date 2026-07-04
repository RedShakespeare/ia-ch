// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "actor.hpp"
#include "catch.hpp"
#include "global.hpp"
#include "map.hpp"
#include "terrain.hpp"
#include "terrain_factory.hpp"
#include "test_utils.hpp"

// -----------------------------------------------------------------------------
// Test cases
// -----------------------------------------------------------------------------

TEST_CASE("Terrain hit() destroys Tomb into rubble on pure damage")
{
    test_utils::init_all();

    const P pos(5, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::tomb, pos));

    auto* const tomb = map::g_terrain.at(pos);

    tomb->hit(DmgType::pure, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() destroys Tomb into rubble on explosion damage")
{
    test_utils::init_all();

    const P pos(5, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::tomb, pos));

    auto* const tomb = map::g_terrain.at(pos);

    tomb->hit(DmgType::explosion, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() leaves Bones unchanged on pure damage")
{
    // Bones has no hit() override (deleted as a no-op); the base class no-op
    // must leave the terrain intact.
    test_utils::init_all();

    const P pos(5, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::bones, pos));

    auto* const bones = map::g_terrain.at(pos);

    bones->hit(DmgType::pure, map::g_player);
    bones->hit(DmgType::explosion, map::g_player);
    bones->hit(DmgType::fire, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::bones);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() leaves Stairs unchanged on damage")
{
    // Stairs has no hit() override (deleted as a no-op).
    test_utils::init_all();

    const P pos(5, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::stairs, pos));

    auto* const stairs = map::g_terrain.at(pos);

    stairs->hit(DmgType::pure, map::g_player);
    stairs->hit(DmgType::explosion, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::stairs);

    test_utils::cleanup_all();
}
