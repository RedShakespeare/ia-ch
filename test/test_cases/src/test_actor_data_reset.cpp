// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "ability_values.hpp"
#include "actor_data.hpp"
#include "audio_data.hpp"
#include "catch.hpp"
#include "colors.hpp"
#include "global.hpp"
#include "property_data.hpp"
#include "room.hpp"
#include "spells.hpp"
#include "terrain_data.hpp"

TEST_CASE("ActorData default state matches the old reset() values")
{
    actor::ActorData d;

    REQUIRE(d.id.empty());
    REQUIRE(d.name_a.empty());
    REQUIRE(d.name_a_i18n_key.empty());
    REQUIRE(d.name_the.empty());
    REQUIRE(d.name_the_i18n_key.empty());
    REQUIRE(d.corpse_name_a.empty());
    REQUIRE(d.corpse_name_a_i18n_key.empty());
    REQUIRE(d.corpse_name_the.empty());
    REQUIRE(d.corpse_name_the_i18n_key.empty());
    REQUIRE(d.tile == gfx::TileId::END);
    REQUIRE(d.character == 'X');
    REQUIRE(d.color == colors::yellow());

    REQUIRE(d.group_sizes.empty());

    REQUIRE(d.hp == 0);
    REQUIRE(d.spi == 0);
    REQUIRE(d.item_sets.empty());
    REQUIRE(d.intr_attacks.empty());
    REQUIRE(d.spells.empty());
    REQUIRE(d.speed == actor::Speed::normal);

    for (size_t i = 0; i < (size_t)prop::Id::END; ++i) {
        REQUIRE_FALSE(d.natural_props[i]);
    }

    for (size_t i = 0; i < (size_t)actor::AiId::END; ++i) {
        const bool expected =
            (i == (size_t)actor::AiId::moves_randomly_when_unaware);
        REQUIRE(d.ai[i] == expected);
    }

    REQUIRE(d.nr_turns_aware == 0);
    REQUIRE(d.ranged_cooldown_turns == 0);
    REQUIRE(d.spawn_min_dlvl == -1);
    REQUIRE(d.spawn_max_dlvl == -1);
    REQUIRE(d.spawn_weight == 100);
    REQUIRE(d.actor_size == actor::Size::humanoid);
    REQUIRE(d.nr_kills == 0);
    REQUIRE_FALSE(d.has_player_seen);
    REQUIRE_FALSE(d.can_open_doors);
    REQUIRE_FALSE(d.can_bash_doors);
    REQUIRE_FALSE(d.prevent_knockback);
    REQUIRE(d.nr_left_allowed_to_spawn == -1);
    REQUIRE_FALSE(d.is_unique);
    REQUIRE(d.is_auto_spawn_allowed);

    REQUIRE(d.wary_msg.empty());
    REQUIRE(d.wary_msg_i18n_key.empty());
    REQUIRE(d.aware_msg_mon_seen.empty());
    REQUIRE(d.aware_msg_mon_seen_i18n_key.empty());
    REQUIRE(d.aware_msg_mon_hidden.empty());
    REQUIRE(d.aware_msg_mon_hidden_i18n_key.empty());
    REQUIRE(d.smell_msg.empty());
    REQUIRE(d.smell_msg_i18n_key.empty());

    REQUIRE_FALSE(d.use_cultist_aware_msg_mon_seen);
    REQUIRE_FALSE(d.use_cultist_aware_msg_mon_hidden);

    REQUIRE(d.aware_sfx_mon_seen == audio::SfxId::END);
    REQUIRE(d.aware_sfx_mon_hidden == audio::SfxId::END);

    REQUIRE(d.spell_msg_sound.empty());
    REQUIRE(d.spell_msg_sound_i18n_key.empty());
    REQUIRE(d.spell_msg_visual.empty());
    REQUIRE(d.spell_msg_visual_i18n_key.empty());
    REQUIRE(d.death_msg_override.empty());
    REQUIRE(d.death_msg_override_i18n_key.empty());

    REQUIRE(d.erratic_move_pct == 0);
    REQUIRE(d.mon_shock_lvl == MonShockLvl::none);

    REQUIRE_FALSE(d.is_humanoid);
    REQUIRE_FALSE(d.is_rat);
    REQUIRE_FALSE(d.is_canine);
    REQUIRE_FALSE(d.is_spider);
    REQUIRE_FALSE(d.is_ghost);
    REQUIRE_FALSE(d.is_ghoul);
    REQUIRE_FALSE(d.is_snake);
    REQUIRE_FALSE(d.is_reptile);
    REQUIRE_FALSE(d.is_amphibian);
    REQUIRE_FALSE(d.can_be_summoned_by_mon);
    REQUIRE_FALSE(d.can_spawn_from_tomb);
    REQUIRE_FALSE(d.can_be_shapeshifted_into);
    REQUIRE(d.can_bleed);
    REQUIRE(d.can_leave_corpse);
    REQUIRE_FALSE(d.prio_corpse_bash);

    REQUIRE(d.native_rooms.empty());
    REQUIRE(d.starting_allies.empty());

    REQUIRE(d.descr.empty());
    REQUIRE(d.descr_i18n_key.empty());
}

TEST_CASE("ActorData::reset() restores defaults on a populated instance")
{
    actor::ActorData d;

    // Mutate a representative set of fields.
    d.id = "MON_X";
    d.name_a = "x";
    d.tile = gfx::TileId::floor;
    d.character = 'Z';
    d.color = colors::red();
    d.hp = 42;
    d.spi = 7;
    d.speed = actor::Speed::fast;
    d.natural_props[0] = true;
    d.ability_values.set_val(AbilityId::melee, 50);
    d.ai[0] = true;
    d.ai[(size_t)actor::AiId::moves_randomly_when_unaware] = false;
    d.nr_kills = 9;
    d.has_player_seen = true;
    d.can_open_doors = true;
    d.is_unique = true;
    d.mon_shock_lvl = MonShockLvl::terrifying;
    d.can_bleed = false;
    d.native_rooms.push_back(room::RoomType::plain);
    d.starting_allies.push_back({"MON_Y", {1, 1}});
    d.intr_attacks.push_back(std::make_shared<actor::IntrAttData>());
    d.spells.push_back({SpellId::END, SpellSkill::basic, 50});

    d.reset();

    // Verify a representative subset is back to defaults.
    REQUIRE(d.id.empty());
    REQUIRE(d.name_a.empty());
    REQUIRE(d.tile == gfx::TileId::END);
    REQUIRE(d.character == 'X');
    REQUIRE(d.color == colors::yellow());
    REQUIRE(d.hp == 0);
    REQUIRE(d.spi == 0);
    REQUIRE(d.speed == actor::Speed::normal);
    REQUIRE_FALSE(d.natural_props[0]);
    REQUIRE(d.ability_values.raw_val(AbilityId::melee) == 0);
    REQUIRE_FALSE(d.ai[0]);
    REQUIRE(d.ai[(size_t)actor::AiId::moves_randomly_when_unaware]);
    REQUIRE(d.nr_kills == 0);
    REQUIRE_FALSE(d.has_player_seen);
    REQUIRE_FALSE(d.can_open_doors);
    REQUIRE_FALSE(d.is_unique);
    REQUIRE(d.mon_shock_lvl == MonShockLvl::none);
    REQUIRE(d.can_bleed);
    REQUIRE(d.native_rooms.empty());
    REQUIRE(d.starting_allies.empty());
    REQUIRE(d.intr_attacks.empty());
    REQUIRE(d.spells.empty());
}

TEST_CASE("AbilityValues default state is zeroed")
{
    AbilityValues av;

    for (size_t i = 0; i < (size_t)AbilityId::END; ++i) {
        REQUIRE(av.raw_val((AbilityId)i) == 0);
    }
}

TEST_CASE("AbilityValues::reset() zeros a populated instance")
{
    AbilityValues av;

    av.set_val(AbilityId::melee, 30);
    av.set_val(AbilityId::ranged, -10);
    av.set_val(AbilityId::dodging, 99);
    av.set_val(AbilityId::stealth, 1);
    av.set_val(AbilityId::searching, 5);

    av.reset();

    for (size_t i = 0; i < (size_t)AbilityId::END; ++i) {
        REQUIRE(av.raw_val((AbilityId)i) == 0);
    }
}

TEST_CASE("MoveRules::reset() clears walkable flag and both prop vectors")
{
    terrain::MoveRules mr;

    mr.is_walkable = true;
    mr.props_allow_move.push_back(prop::Id::ethereal);
    mr.props_prevent_move.push_back(prop::Id::blessed);

    mr.reset();

    REQUIRE_FALSE(mr.is_walkable);
    REQUIRE(mr.props_allow_move.empty());
    REQUIRE(mr.props_prevent_move.empty());
}
