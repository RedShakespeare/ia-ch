// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "actor.hpp"
#include "actor_factory.hpp"
#include "catch.hpp"
#include "global.hpp"
#include "map.hpp"
#include "property.hpp"
#include "property_factory.hpp"
#include "random.hpp"
#include "terrain.hpp"
#include "terrain_door.hpp"
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

// -----------------------------------------------------------------------------
// Characterization tests for terrain hit() refactor safety net.
// These pin down current behavior so the planned Extract Method refactoring
// of Wall/Pillar/Petroglyph and Door::hit can be verified to preserve it.
// -----------------------------------------------------------------------------

TEST_CASE("Terrain hit() Wall is not left as rubble_high on pure damage")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::wall, pos));

    map::g_terrain.at(pos)->hit(DmgType::pure, map::g_player);

    // On pure damage, Wall always goes through destr_stone_wall (never the
    // rubble_high branch). The position may end up as wall, floor, or
    // something else depending on adjacent conditions, but never rubble_high.
    REQUIRE(map::g_terrain.at(pos)->id() != terrain::Id::rubble_high);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Wall can become rubble_high on explosion damage")
{
    test_utils::init_all();

    const P pos(5, 5);

    // The explosion branch replaces with rubble_high on a coin toss. Try a
    // range of seeds to confirm the rubble_high branch is reachable.
    bool got_rubble_high = false;
    for (uint32_t s = 0; s < 1000 && !got_rubble_high; ++s) {
        map::update_terrain(terrain::make(terrain::Id::floor, pos));
        map::update_terrain(terrain::make(terrain::Id::wall, pos));
        rnd::seed(s);
        map::g_terrain.at(pos)->hit(DmgType::explosion, map::g_player);
        if (map::g_terrain.at(pos)->id() == terrain::Id::rubble_high) {
            got_rubble_high = true;
        }
    }

    REQUIRE(got_rubble_high);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Wall, Pillar, Petroglyph produce identical outcomes across seeds")
{
    test_utils::init_all();

    const P pos(5, 5);

    for (uint32_t s = 0; s < 200; ++s) {
        // Wall
        map::update_terrain(terrain::make(terrain::Id::floor, pos));
        map::update_terrain(terrain::make(terrain::Id::wall, pos));
        rnd::seed(s);
        map::g_terrain.at(pos)->hit(DmgType::explosion, map::g_player);
        const auto wall_id = map::g_terrain.at(pos)->id();

        // Pillar
        map::update_terrain(terrain::make(terrain::Id::floor, pos));
        map::update_terrain(terrain::make(terrain::Id::pillar, pos));
        rnd::seed(s);
        map::g_terrain.at(pos)->hit(DmgType::explosion, map::g_player);
        const auto pillar_id = map::g_terrain.at(pos)->id();

        // Petroglyph
        map::update_terrain(terrain::make(terrain::Id::floor, pos));
        map::update_terrain(terrain::make(terrain::Id::petroglyph, pos));
        rnd::seed(s);
        map::g_terrain.at(pos)->hit(DmgType::explosion, map::g_player);
        const auto petroglyph_id = map::g_terrain.at(pos)->id();

        INFO("seed=" << s << " wall=" << (int)wall_id
                     << " pillar=" << (int)pillar_id
                     << " petroglyph=" << (int)petroglyph_id);
        REQUIRE(pillar_id == wall_id);
        REQUIRE(petroglyph_id == wall_id);
    }

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Door destroyed into rubble on pure damage")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    auto* const door = static_cast<terrain::Door*>(
        terrain::make(terrain::Id::door, pos));
    door->init_type_and_state(
        terrain::DoorType::wood,
        terrain::DoorSpawnState::closed);
    map::update_terrain(door);

    map::g_terrain.at(pos)->hit(DmgType::pure, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Door destroyed into rubble on explosion damage")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    auto* const door = static_cast<terrain::Door*>(
        terrain::make(terrain::Id::door, pos));
    door->init_type_and_state(
        terrain::DoorType::wood,
        terrain::DoorSpawnState::closed);
    map::update_terrain(door);

    map::g_terrain.at(pos)->hit(DmgType::explosion, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Door shotgun destroys wood door when closed")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    auto* const door = static_cast<terrain::Door*>(
        terrain::make(terrain::Id::door, pos));
    door->init_type_and_state(
        terrain::DoorType::wood,
        terrain::DoorSpawnState::closed);
    map::update_terrain(door);

    map::g_terrain.at(pos)->hit(DmgType::shotgun, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Door shotgun destroys gate door when closed")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    auto* const door = static_cast<terrain::Door*>(
        terrain::make(terrain::Id::door, pos));
    door->init_type_and_state(
        terrain::DoorType::gate,
        terrain::DoorSpawnState::closed);
    map::update_terrain(door);

    map::g_terrain.at(pos)->hit(DmgType::shotgun, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Door shotgun leaves metal door intact when closed")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    auto* const door = static_cast<terrain::Door*>(
        terrain::make(terrain::Id::door, pos));
    door->init_type_and_state(
        terrain::DoorType::metal,
        terrain::DoorSpawnState::closed);
    map::update_terrain(door);

    map::g_terrain.at(pos)->hit(DmgType::shotgun, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::door);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Door shotgun leaves open wood door intact")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    auto* const door = static_cast<terrain::Door*>(
        terrain::make(terrain::Id::door, pos));
    door->init_type_and_state(
        terrain::DoorType::wood,
        terrain::DoorSpawnState::open);
    map::update_terrain(door);

    REQUIRE(door->is_open());

    map::g_terrain.at(pos)->hit(DmgType::shotgun, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::door);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Door fire ignites wood door when not warded")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    auto* const door = static_cast<terrain::Door*>(
        terrain::make(terrain::Id::door, pos));
    door->init_type_and_state(
        terrain::DoorType::wood,
        terrain::DoorSpawnState::closed);
    map::update_terrain(door);

    map::g_terrain.at(pos)->hit(DmgType::fire, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->is_burning());

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Door fire leaves metal door intact")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    auto* const door = static_cast<terrain::Door*>(
        terrain::make(terrain::Id::door, pos));
    door->init_type_and_state(
        terrain::DoorType::metal,
        terrain::DoorSpawnState::closed);
    map::update_terrain(door);

    map::g_terrain.at(pos)->hit(DmgType::fire, map::g_player);

    REQUIRE(!map::g_terrain.at(pos)->is_burning());
    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::door);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Door fire leaves warded wood door intact")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    auto* const door = static_cast<terrain::Door*>(
        terrain::make(terrain::Id::door, pos));
    door->init_type_and_state(
        terrain::DoorType::wood,
        terrain::DoorSpawnState::warded);
    map::update_terrain(door);

    REQUIRE(door->is_warded());

    map::g_terrain.at(pos)->hit(DmgType::fire, map::g_player);

    REQUIRE(!map::g_terrain.at(pos)->is_burning());
    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::door);

    test_utils::cleanup_all();
}

// -----------------------------------------------------------------------------
// Characterization tests for Statue/Urn/Brazier/Grate hit() refactor.
// Pin down current behavior of (a) inline rubble replacement arms on
// pure/explosion and (b) shared kicking/control_object_spell topple dispatch
// in Statue/Urn/Brazier, so the planned Extract Method refactor can be
// verified to preserve it.
// -----------------------------------------------------------------------------

TEST_CASE("Terrain hit() destroys Statue into rubble on pure damage")
{
    test_utils::init_all();

    const P pos(5, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::statue, pos));

    map::g_terrain.at(pos)->hit(DmgType::pure, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() destroys Urn into rubble on pure damage")
{
    test_utils::init_all();

    const P pos(5, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::urn, pos));

    map::g_terrain.at(pos)->hit(DmgType::pure, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() destroys Brazier into rubble on pure damage")
{
    test_utils::init_all();

    const P pos(5, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::brazier, pos));

    map::g_terrain.at(pos)->hit(DmgType::pure, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() destroys Grate into rubble on pure damage and destroys adjacent doors")
{
    test_utils::init_all();

    const P pos(5, 5);
    const P door_pos(4, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::floor, door_pos));

    auto* const door = static_cast<terrain::Door*>(
        terrain::make(terrain::Id::door, door_pos));
    door->init_type_and_state(
        terrain::DoorType::wood,
        terrain::DoorSpawnState::closed);
    map::update_terrain(door);

    REQUIRE(map::g_terrain.at(door_pos)->id() == terrain::Id::door);

    map::update_terrain(terrain::make(terrain::Id::grate, pos));

    map::g_terrain.at(pos)->hit(DmgType::pure, map::g_player);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);
    REQUIRE(map::g_terrain.at(door_pos)->id() != terrain::Id::door);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Statue topples on kicking toward open floor")
{
    test_utils::init_all();

    const P pos(5, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::statue, pos));

    // Player stands east of the statue; kick direction resolves to west,
    // toward open floor at {4,5}.
    map::g_player->m_pos.set(6, 5);

    map::g_terrain.at(pos)->hit(
        DmgType::kicking,
        map::g_player,
        map::g_player->m_pos);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Statue wiggles instead of toppling when player is weakened")
{
    test_utils::init_all();

    const P pos(5, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::statue, pos));

    map::g_player->m_pos.set(6, 5);
    map::g_player->m_properties.apply(prop::make(prop::Id::weakened));

    map::g_terrain.at(pos)->hit(
        DmgType::kicking,
        map::g_player,
        map::g_player->m_pos);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::statue);

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Brazier topples and emits burning explosion on kicking")
{
    test_utils::init_all();

    const P pos(5, 5);
    const P dst_pos(4, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::floor, dst_pos));
    map::update_terrain(terrain::make(terrain::Id::brazier, pos));

    auto* const rat = actor::make("MON_RAT", dst_pos);

    REQUIRE(!rat->m_properties.has(prop::Id::burning));

    map::g_player->m_pos.set(6, 5);

    map::g_terrain.at(pos)->hit(
        DmgType::kicking,
        map::g_player,
        map::g_player->m_pos);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);
    REQUIRE(rat->m_properties.has(prop::Id::burning));

    test_utils::cleanup_all();
}

TEST_CASE("Terrain hit() Brazier wiggles instead of toppling when player is weakened")
{
    test_utils::init_all();

    const P pos(5, 5);

    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::brazier, pos));

    map::g_player->m_pos.set(6, 5);
    map::g_player->m_properties.apply(prop::make(prop::Id::weakened));

    map::g_terrain.at(pos)->hit(
        DmgType::kicking,
        map::g_player,
        map::g_player->m_pos);

    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::brazier);

    test_utils::cleanup_all();
}
