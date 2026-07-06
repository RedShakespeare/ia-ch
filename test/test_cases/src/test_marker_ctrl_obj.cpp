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
#include "marker.hpp"
#include "spells.hpp"
#include "terrain.hpp"
#include "terrain_door.hpp"
#include "terrain_factory.hpp"
#include "test_utils.hpp"

// -----------------------------------------------------------------------------
// Test cases
// -----------------------------------------------------------------------------
// Characterization tests for the six CtrlObjAction subclasses in src/marker.cpp.
// These pin the current can_control / run behavior before and after the
// Move Method refactor that pushes capability queries onto Terrain virtuals.

namespace
{
 terrain::Door* make_door(
     const P& pos,
     terrain::DoorType type,
     terrain::DoorSpawnState state)
 {
     auto* const door = static_cast<terrain::Door*>(
         terrain::make(terrain::Id::door, pos));

     door->init_type_and_state(type, state);

     map::update_terrain(door);

     return door;
 }
}  // namespace

// =============================================================================
// CtrlObjOpen::can_control
// =============================================================================

TEST_CASE("CtrlObjOpen can_control on closed chest returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::chest, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on open chest returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::chest, pos));

    // Open the chest first.
    map::g_terrain.at(pos)->open(nullptr);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on closed cabinet returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::cabinet, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on open cabinet returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::cabinet, pos));

    map::g_terrain.at(pos)->open(nullptr);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on closed tomb returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::tomb, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on open tomb returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::tomb, pos));

    map::g_terrain.at(pos)->open(nullptr);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on closed non-stuck door returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::closed);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on open door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::open);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on hidden door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::secret);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on known-stuck door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    auto* door = make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::stuck);

    // Make stuck status known.
    door->reveal_stuck_status(terrain::PrintRevealMsg::yes);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on unknown-stuck door returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::stuck);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen can_control on floor returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

// =============================================================================
// CtrlObjCloseDoor::can_control
// =============================================================================

TEST_CASE("CtrlObjCloseDoor can_control on open wood door basic skill returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::open);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjCloseDoor action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjCloseDoor can_control on open metal door basic skill returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::metal, terrain::DoorSpawnState::open);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjCloseDoor action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjCloseDoor can_control on open metal door expert skill returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::metal, terrain::DoorSpawnState::open);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjCloseDoor action;

    REQUIRE(action.can_control(terrain, SpellSkill::expert));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjCloseDoor can_control on closed door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::closed);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjCloseDoor action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjCloseDoor can_control on hidden door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::secret);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjCloseDoor action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjCloseDoor can_control on floor returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjCloseDoor action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

// =============================================================================
// CtrlObjJamDoor::can_control
// =============================================================================

TEST_CASE("CtrlObjJamDoor can_control on closed wood door returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::closed);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjJamDoor action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjJamDoor can_control on open door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::open);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjJamDoor action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjJamDoor can_control on metal door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::metal, terrain::DoorSpawnState::closed);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjJamDoor action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjJamDoor can_control on hidden door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::secret);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjJamDoor action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjJamDoor can_control on known-stuck door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    auto* door = make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::stuck);

    door->reveal_stuck_status(terrain::PrintRevealMsg::yes);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjJamDoor action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

// =============================================================================
// CtrlObjDeactivateCrystal::can_control
// =============================================================================

TEST_CASE("CtrlObjDeactivateCrystal can_control on active crystal returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::crystal_key, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjDeactivateCrystal action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjDeactivateCrystal can_control on inactive crystal returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::crystal_key, pos));

    // Deactivate the crystal first (no linked door, that's fine).
    static_cast<terrain::CrystalKey*>(map::g_terrain.at(pos))->player_deactivate();

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjDeactivateCrystal action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjDeactivateCrystal can_control on floor returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjDeactivateCrystal action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

// =============================================================================
// CtrlObjStrike::can_control
// =============================================================================

TEST_CASE("CtrlObjStrike can_control on closed wood door returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::closed);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjStrike action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjStrike can_control on open door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::open);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjStrike action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjStrike can_control on metal door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::metal, terrain::DoorSpawnState::closed);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjStrike action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjStrike can_control on hidden door returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::secret);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjStrike action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjStrike can_control on brazier returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::brazier, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjStrike action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjStrike can_control on statue returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::statue, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjStrike action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjStrike can_control on urn returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::urn, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjStrike action;

    REQUIRE(action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjStrike can_control on floor returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjStrike action;

    REQUIRE(!action.can_control(terrain, SpellSkill::basic));

    test_utils::cleanup_all();
}

// =============================================================================
// CtrlObjDestrWall::can_control
// =============================================================================

TEST_CASE("CtrlObjDestrWall can_control on wall transcendent skill returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::wall, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjDestrWall action;

    REQUIRE(action.can_control(terrain, SpellSkill::transcendent));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjDestrWall can_control on rubble_high transcendent skill returns true")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::rubble_high, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjDestrWall action;

    REQUIRE(action.can_control(terrain, SpellSkill::transcendent));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjDestrWall can_control on wall master skill returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::wall, pos));

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjDestrWall action;

    REQUIRE(!action.can_control(terrain, SpellSkill::master));

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjDestrWall can_control on door transcendent skill returns false")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::closed);

    const auto& terrain = *map::g_terrain.at(pos);

    CtrlObjDestrWall action;

    REQUIRE(!action.can_control(terrain, SpellSkill::transcendent));

    test_utils::cleanup_all();
}

// =============================================================================
// run() tests
// =============================================================================

TEST_CASE("CtrlObjOpen run on closed chest opens chest")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::chest, pos));

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(action.run(terrain, SpellSkill::basic) == DidAction::yes);
    REQUIRE(static_cast<terrain::Chest&>(terrain).is_open());

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen run on closed door opens door")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::closed);

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(action.run(terrain, SpellSkill::basic) == DidAction::yes);
    REQUIRE(static_cast<terrain::Door&>(terrain).is_open());

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjOpen run on unknown-stuck door reveals stuck status and does not open")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    auto* door = make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::stuck);

    REQUIRE(!door->is_known_stuck());

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjOpen action;

    REQUIRE(action.run(terrain, SpellSkill::basic) == DidAction::yes);

    // Stuck status should now be known, and the door should still be closed.
    REQUIRE(door->is_known_stuck());
    REQUIRE(!door->is_open());

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjCloseDoor run on open door with no actor closes door")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    auto* door = make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::open);

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjCloseDoor action;

    REQUIRE(action.run(terrain, SpellSkill::basic) == DidAction::yes);
    REQUIRE(!door->is_open());

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjCloseDoor run on open door with actor at pos stays open")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    auto* door = make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::open);

    // Place an actor at the door's position.
    auto* actor = actor::make("MON_ZOMBIE", pos);
    (void)actor;

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjCloseDoor action;

    REQUIRE(action.run(terrain, SpellSkill::basic) == DidAction::no);
    REQUIRE(door->is_open());

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjJamDoor run on closed wood door jams door")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    auto* door = make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::closed);

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjJamDoor action;

    REQUIRE(action.run(terrain, SpellSkill::basic) == DidAction::yes);
    REQUIRE(door->is_stuck());
    REQUIRE(door->is_known_stuck());

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjDeactivateCrystal run on active crystal deactivates crystal")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::crystal_key, pos));

    auto* crystal = static_cast<terrain::CrystalKey*>(map::g_terrain.at(pos));

    REQUIRE(crystal->is_active());

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjDeactivateCrystal action;

    REQUIRE(action.run(terrain, SpellSkill::basic) == DidAction::yes);
    REQUIRE(!crystal->is_active());

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjStrike run on closed wood door damages door")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    make_door(pos, terrain::DoorType::wood, terrain::DoorSpawnState::closed);

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjStrike action;

    REQUIRE(action.run(terrain, SpellSkill::basic) == DidAction::yes);

    // The door should have been destroyed into rubble_low by the strike damage.
    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::rubble_low);

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjDestrWall run on wall inside outer walls destroys wall")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::wall, pos));

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjDestrWall action;

    REQUIRE(action.run(terrain, SpellSkill::transcendent) == DidAction::yes);
    REQUIRE(map::g_terrain.at(pos)->id() != terrain::Id::wall);

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjDestrWall run on wall at boundary leaves wall unchanged")
{
    test_utils::init_all();

    // (0, 5) is an outer wall position.
    const P pos(0, 5);

    // Map reset fills with floor and outer walls. Ensure we have a wall at pos.
    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::wall);

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjDestrWall action;

    REQUIRE(action.run(terrain, SpellSkill::transcendent) == DidAction::yes);
    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::wall);

    test_utils::cleanup_all();
}

TEST_CASE("CtrlObjStrike run on brazier with canceled direction returns no and leaves terrain")
{
    test_utils::init_all();

    const P pos(5, 5);
    map::update_terrain(terrain::make(terrain::Id::floor, pos));
    map::update_terrain(terrain::make(terrain::Id::brazier, pos));

    auto& terrain = *map::g_terrain.at(pos);

    CtrlObjStrike action;

    // In test stub, query::dir returns Dir::END (cancel).
    REQUIRE(action.run(terrain, SpellSkill::basic) == DidAction::no);
    REQUIRE(map::g_terrain.at(pos)->id() == terrain::Id::brazier);

    test_utils::cleanup_all();
}
