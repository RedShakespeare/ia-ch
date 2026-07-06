// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <string>

#include "actor.hpp"
#include "actor_data.hpp"
#include "actor_factory.hpp"
#include "actor_move.hpp"
#include "audio_data.hpp"
#include "catch.hpp"
#include "direction.hpp"
#include "game_time.hpp"
#include "map.hpp"
#include "pos.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "sound.hpp"
#include "terrain.hpp"
#include "terrain_factory.hpp"
#include "test_utils.hpp"

TEST_CASE("Sound alerts monster")
{
    test_utils::init_all();

    for (int x = 0; x < map::w(); ++x) {
        for (int y = 0; y < map::h(); ++y) {
            map::update_terrain(
                terrain::make(terrain::Id::wall, {x, y}));
        }
    }

    const P snd_origin(5, 7);
    const P wall_pos(6, 7);
    const P mon_pos(7, 7);

    // Fill a 3x3 area with floor
    for (int x = wall_pos.x - 1; x <= wall_pos.x + 1; ++x) {
        for (int y = wall_pos.y - 1; y <= wall_pos.y + 1; ++y) {
            map::update_terrain(
                terrain::make(terrain::Id::floor, {x, y}));
        }
    }

    // Put a wall in the middle (the sound will travel around this wall)
    map::update_terrain(terrain::make(terrain::Id::wall, wall_pos));

    auto* const zombie = actor::make("MON_ZOMBIE", mon_pos);

    REQUIRE(!actor::is_aware_of_player(*zombie));

    // First run a sound that does NOT alert monsters
    Snd snd(
        "",
        SndSpec{}
            .sfx(audio::SfxId::END)
            .ignore_msg_if_origin_seen(IgnoreMsgIfOriginSeen::no)
            .origin(snd_origin)
            .actor(nullptr)
            .vol(SndVol::low)
            .alerts(AlertsMon::no));

    snd.run();

    REQUIRE(!actor::is_aware_of_player(*zombie));

    // Now run a sound that does alert monsters
    snd.set_alerts_mon(AlertsMon::yes);

    snd.run();

    REQUIRE(actor::is_aware_of_player(*zombie));

    test_utils::cleanup_all();
}

TEST_CASE("Player wading alerts monsters")
{
    test_utils::init_all();

    map::update_terrain(terrain::make(terrain::Id::floor, {4, 5}));
    map::update_terrain(terrain::make(terrain::Id::floor, {5, 5}));
    map::update_terrain(terrain::make(terrain::Id::liquid, {6, 5}));
    map::update_terrain(terrain::make(terrain::Id::floor, {7, 5}));

    map::g_player->m_pos = {4, 5};

    auto* const zombie = actor::make("MON_ZOMBIE", {7, 5});

    REQUIRE(!actor::is_aware_of_player(*zombie));

    // Move player into floor
    actor::do_move_action(*map::g_player, Dir::right);

    REQUIRE(!actor::is_aware_of_player(*zombie));

    game_time::g_allow_tick = true;

    // Move player into water (wading)
    actor::do_move_action(*map::g_player, Dir::right);

    REQUIRE(actor::is_aware_of_player(*zombie));

    test_utils::cleanup_all();
}

TEST_CASE("Monster wading does not alert monsters")
{
    test_utils::init_all();

    map::update_terrain(terrain::make(terrain::Id::floor, {5, 5}));
    map::update_terrain(terrain::make(terrain::Id::liquid, {6, 5}));
    map::update_terrain(terrain::make(terrain::Id::floor, {7, 5}));

    auto* const zombie_1 = actor::make("MON_ZOMBIE", {5, 5});
    auto* const zombie_2 = actor::make("MON_ZOMBIE", {7, 5});

    REQUIRE(!actor::is_aware_of_player(*zombie_1));
    REQUIRE(!actor::is_aware_of_player(*zombie_2));

    // Move zombie 1 into water (wading)
    actor::do_move_action(*zombie_1, Dir::right);

    REQUIRE(!actor::is_aware_of_player(*zombie_1));
    REQUIRE(!actor::is_aware_of_player(*zombie_2));

    test_utils::cleanup_all();
}

// Characterization test for the SndSpec-based Snd constructor, kept after the
// legacy 7-arg ctor was removed. Pins field storage and the default
// snd_heard_effect (nullptr) behavior.

TEST_CASE("Snd constructed via SndSpec stores all fields")
{
    test_utils::init_all();

    const P origin(3, 5);
    auto* const actor = map::g_player;

    Snd snd(
        "hello",
        SndSpec{}
            .sfx(audio::SfxId::horn)
            .ignore_msg_if_origin_seen(IgnoreMsgIfOriginSeen::yes)
            .origin(origin)
            .actor(actor)
            .vol(SndVol::high)
            .alerts(AlertsMon::yes));

    REQUIRE(snd.msg() == "hello");
    REQUIRE(snd.sfx() == audio::SfxId::horn);
    REQUIRE(snd.is_msg_ignored_if_origin_seen());
    REQUIRE(snd.origin() == origin);
    REQUIRE(snd.actor_who_made_sound() == actor);
    REQUIRE(snd.volume() == SndVol::high);
    REQUIRE(snd.is_alerting_mon());
    REQUIRE(!snd.did_player_hear_sound());

    test_utils::cleanup_all();
}

TEST_CASE("Snd via SndSpec with no heard_effect behaves as if null")
{
    test_utils::init_all();

    const P origin(3, 5);
    auto* const player = map::g_player;

    // Omitting .heard_effect(...) leaves it as nullptr (the default).
    Snd snd(
        "",
        SndSpec{}
            .sfx(audio::SfxId::END)
            .ignore_msg_if_origin_seen(IgnoreMsgIfOriginSeen::no)
            .origin(origin)
            .actor(nullptr)
            .vol(SndVol::low)
            .alerts(AlertsMon::no));

    // on_heard on the player sets did_player_hear_sound; a null effect must
    // not be invoked.
    snd.on_heard(*player);

    REQUIRE(snd.did_player_hear_sound());

    test_utils::cleanup_all();
}
