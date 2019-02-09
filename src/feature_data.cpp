// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "feature_data.hpp"

#include "init.hpp"

#include "actor.hpp"
#include "feature_rigid.hpp"
#include "feature_mob.hpp"
#include "feature_trap.hpp"
#include "feature_event.hpp"
#include "feature_door.hpp"
#include "feature_monolith.hpp"
#include "feature_pylon.hpp"
#include "game_time.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static FeatureData s_data[(size_t)FeatureId::END];

static void reset_data(FeatureData& d)
{
        d.make_obj = [](const P & p) {(void)p; return nullptr;};
        d.id = FeatureId::END;
        d.character = ' ';
        d.tile = TileId::END;
        d.move_rules.reset();
        d.is_sound_passable = true;
        d.is_projectile_passable = true;
        d.is_los_passable = true;
        d.is_smoke_passable = true;
        d.can_have_blood = true;
        d.can_have_gore = false;
        d.can_have_corpse = true;
        d.can_have_rigid = true;
        d.can_have_item = true;
        d.matl_type = Matl::stone;
        d.msg_on_player_blocked = "The way is blocked.";
        d.msg_on_player_blocked_blind = "I bump into something.";
        d.shock_when_adjacent = 0;
}

static void add_to_list_and_reset(FeatureData& d)
{
        s_data[(size_t)d.id] = d;

        reset_data(d);
}

static void init_data_list()
{
        FeatureData d;
        reset_data(d);

        d.id = FeatureId::floor;
        d.make_obj = [](const P & p) {
                return new Floor(p);
        };
        d.character = '.';
        d.tile = TileId::floor;
        d.move_rules.set_walkable();
        d.matl_type = Matl::stone;
        d.can_have_gore = true;
        add_to_list_and_reset(d);


        d.id = FeatureId::bridge;
        d.make_obj = [](const P & p) {
                return new Bridge(p);
        };
        d.move_rules.set_walkable();
        d.matl_type = Matl::wood;
        add_to_list_and_reset(d);

        d.id = FeatureId::wall;
        d.make_obj = [](const P & p) {
                return new Wall(p);
        };
        d.character = config::is_text_mode_wall_full_square() ? 10 : '#';
        d.tile = TileId::wall_top;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.move_rules.set_prop_can_move(PropId::burrowing);
        d.is_sound_passable = false;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.is_smoke_passable = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        add_to_list_and_reset(d);

        d.id = FeatureId::tree;
        d.make_obj = [](const P & p) {
                return new Tree(p);
        };
        d.character = '|';
        d.tile = TileId::tree;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.is_sound_passable = false;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.msg_on_player_blocked = "There is a tree in the way.";
        d.matl_type = Matl::wood;
        d.shock_when_adjacent = 1;
        add_to_list_and_reset(d);

        d.id = FeatureId::grass;
        d.make_obj = [](const P & p) {
                return new Grass(p);
        };
        d.character = '.';
        d.tile = TileId::floor;
        d.move_rules.set_walkable();
        d.matl_type = Matl::plant;
        d.can_have_gore = true;
        add_to_list_and_reset(d);

        d.id = FeatureId::bush;
        d.make_obj = [](const P & p) {
                return new Bush(p);
        };
        d.character = '"';
        d.tile = TileId::bush;
        d.move_rules.set_walkable();
        d.is_los_passable = false;
        d.matl_type = Matl::plant;
        add_to_list_and_reset(d);

        d.id = FeatureId::vines;
        d.make_obj = [](const P & p) {
                return new Vines(p);
        };
        d.character = '"';
        d.tile = TileId::vines;
        d.move_rules.set_walkable();
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.matl_type = Matl::plant;
        d.auto_spawn_placement = FeaturePlacement::either;
        add_to_list_and_reset(d);

        d.id = FeatureId::chains;
        d.make_obj = [](const P & p) {
                return new Chains(p);
        };
        d.character = '"';
        d.tile = TileId::chains;
        d.move_rules.set_walkable();
        d.is_los_passable = true;
        d.is_projectile_passable = true;
        d.can_have_blood = true;
        d.matl_type = Matl::metal;
        d.auto_spawn_placement = FeaturePlacement::either;
        add_to_list_and_reset(d);

        d.id = FeatureId::grate;
        d.make_obj = [](const P & p) {
                return new Grate(p);
        };
        d.character = '#';
        d.tile = TileId::grate;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.move_rules.set_prop_can_move(PropId::burrowing);
        d.move_rules.set_prop_can_move(PropId::ooze);
        d.is_los_passable = true;
        d.can_have_blood = false; // Looks weird
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::metal;
        add_to_list_and_reset(d);

        d.id = FeatureId::stairs;
        d.make_obj = [](const P & p) {
                return new Stairs(p);
        };
        d.character = '>';
        d.tile = TileId::stairs_down;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        add_to_list_and_reset(d);

        d.id = FeatureId::monolith;
        d.make_obj = [](const P & p) {
                return new Monolith(p);
        };
        d.character = '|';
        d.tile = TileId::monolith;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false; // We don't want to mess with the color
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 10;
        d.matl_type = Matl::stone;
        add_to_list_and_reset(d);

        d.id = FeatureId::pylon;
        d.make_obj = [](const P & p) {
                return new Pylon(p, PylonId::any);
        };
        d.character = '|';
        d.tile = TileId::pylon;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false; // We don't want to mess with the color
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 10;
        d.matl_type = Matl::metal;
        add_to_list_and_reset(d);

        d.id = FeatureId::lever;
        d.make_obj = [](const P & p) {
                return new Lever(p);
        };
        d.character = '%';
        d.tile = TileId::lever_left;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::metal;
        add_to_list_and_reset(d);

        d.id = FeatureId::brazier;
        d.make_obj = [](const P & p) {
                return new Brazier(p);
        };
        d.character = '0';
        d.tile = TileId::brazier;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::metal;
        d.auto_spawn_placement = FeaturePlacement::away_from_walls;
        add_to_list_and_reset(d);

        d.id = FeatureId::liquid_shallow;
        d.make_obj = [](const P & p) {
                return new LiquidShallow(p);
        };
        d.character = '~';
        d.tile = TileId::water;
        d.move_rules.set_walkable();
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_rigid = false;
        d.matl_type = Matl::fluid;
        add_to_list_and_reset(d);

        d.id = FeatureId::liquid_deep;
        d.make_obj = [](const P & p) {
                return new LiquidDeep(p);
        };
        d.character = '~';
        d.tile = TileId::water;
        d.can_have_item = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_rigid = false;
        d.matl_type = Matl::fluid;
        add_to_list_and_reset(d);

        d.id = FeatureId::chasm;
        d.make_obj = [](const P & p) {
                return new Chasm(p);
        };
        d.character = '.';
        d.tile = TileId::floor;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.move_rules.set_prop_can_move(PropId::flying);
        d.can_have_item = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.msg_on_player_blocked = "A chasm lies in my way.";
        d.msg_on_player_blocked_blind =
                "I realize I am standing on the edge of a chasm.";
        d.matl_type = Matl::empty;
        d.shock_when_adjacent = 3;
        add_to_list_and_reset(d);

        d.id = FeatureId::gravestone;
        d.make_obj = [](const P & p) {
                return new GraveStone(p);
        };
        d.character = ']';
        d.tile = TileId::grave_stone;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.move_rules.set_prop_can_move(PropId::flying);
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 2;
        d.matl_type = Matl::stone;
        add_to_list_and_reset(d);

        d.id = FeatureId::church_bench;
        d.make_obj = [](const P & p) {
                return new ChurchBench(p);
        };
        d.character = '[';
        d.tile = TileId::church_bench;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.move_rules.set_prop_can_move(PropId::flying);
        d.move_rules.set_prop_can_move(PropId::ooze);
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::wood;
        add_to_list_and_reset(d);

        d.id = FeatureId::carpet;
        d.make_obj = [](const P & p) {
                return new Carpet(p);
        };
        d.character = '.';
        d.tile = TileId::floor;
        d.can_have_rigid = false;
        d.move_rules.set_walkable();
        d.matl_type = Matl::cloth;
        add_to_list_and_reset(d);

        d.id = FeatureId::rubble_high;
        d.make_obj = [](const P & p) {
                return new RubbleHigh(p);
        };
        d.character = 8;
        d.tile = TileId::rubble_high;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.move_rules.set_prop_can_move(PropId::ooze);
        d.move_rules.set_prop_can_move(PropId::burrowing);
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.is_smoke_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        add_to_list_and_reset(d);

        d.id = FeatureId::rubble_low;
        d.make_obj = [](const P & p) {
                return new RubbleLow(p);
        };
        d.character = ',';
        d.tile = TileId::rubble_low;
        d.move_rules.set_walkable();
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = FeaturePlacement::either;
        add_to_list_and_reset(d);

        d.id = FeatureId::bones;
        d.make_obj = [](const P & p) {
                return new Bones(p);
        };
        d.character = '&';
        d.tile = TileId::corpse2;
        d.move_rules.set_walkable();
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = FeaturePlacement::either;
        add_to_list_and_reset(d);

        d.id = FeatureId::statue;
        d.make_obj = [](const P & p) {
                return new Statue(p);
        };
        d.character = 5; //Paragraph sign
        d.tile = TileId::witch_or_warlock;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = FeaturePlacement::either;
        add_to_list_and_reset(d);

        d.id = FeatureId::cocoon;
        d.make_obj = [](const P & p) {
                return new Cocoon(p);
        };
        d.character = '8';
        d.tile = TileId::cocoon_closed;
        d.is_projectile_passable = true;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 3;
        d.matl_type = Matl::cloth;
        d.auto_spawn_placement = FeaturePlacement::either;
        add_to_list_and_reset(d);

        d.id = FeatureId::chest;
        d.make_obj = [](const P & p) {
                return new Chest(p);
        };
        d.character = '+';
        d.tile = TileId::chest_closed;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.auto_spawn_placement = FeaturePlacement::adj_to_walls;
        add_to_list_and_reset(d);

        d.id = FeatureId::cabinet;
        d.make_obj = [](const P & p) {
                return new Cabinet(p);
        };
        d.character = '7';
        d.tile = TileId::cabinet_closed;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::wood;
        d.auto_spawn_placement = FeaturePlacement::adj_to_walls;
        add_to_list_and_reset(d);

        d.id = FeatureId::bookshelf;
        d.make_obj = [](const P & p) {
                return new Bookshelf(p);
        };
        d.character = '7';
        d.tile = TileId::bookshelf_full;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::wood;
        d.auto_spawn_placement = FeaturePlacement::adj_to_walls;
        add_to_list_and_reset(d);

        d.id = FeatureId::alchemist_bench;
        d.make_obj = [](const P & p) {
                return new AlchemistBench(p);
        };
        d.character = '7';
        d.tile = TileId::alchemist_bench_full;
        d.is_projectile_passable = false;
        d.is_los_passable = true;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::wood;
        d.auto_spawn_placement = FeaturePlacement::either;
        add_to_list_and_reset(d);

        d.id = FeatureId::fountain;
        d.make_obj = [](const P & p) {
                return new Fountain(p);
        };
        d.character = '1';
        d.tile = TileId::fountain;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = FeaturePlacement::away_from_walls;
        add_to_list_and_reset(d);

        d.id = FeatureId::stalagmite;
        d.make_obj = [](const P & p) {
                return new Stalagmite(p);
        };
        d.character = ':';
        d.tile = TileId::stalagmite;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = true;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = FeaturePlacement::either;
        add_to_list_and_reset(d);

        d.id = FeatureId::altar;
        d.make_obj = [](const P & p) {
                return new Altar(p);
        };
        d.character = '_';
        d.tile = TileId::altar;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 10;
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = FeaturePlacement::either;
        add_to_list_and_reset(d);

        d.id = FeatureId::tomb;
        d.make_obj = [](const P & p) {
                return new Tomb(p);
        };
        d.character = ']';
        d.tile = TileId::tomb_closed;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.move_rules.set_prop_can_move(PropId::flying);
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 10;
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = FeaturePlacement::either;
        add_to_list_and_reset(d);

        d.id = FeatureId::door;
        d.make_obj = [](const P & p) {
                return new Door(p);
        };
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        add_to_list_and_reset(d);

        d.id = FeatureId::trap;
        d.make_obj = [](const P & p) {
                return new Trap(p);
        };
        d.move_rules.set_walkable();
        d.can_have_rigid = false;
        add_to_list_and_reset(d);

        d.id = FeatureId::lit_dynamite;
        d.make_obj = [](const P & p) {
                return new LitDynamite(p);
        };
        d.character = '/';
        d.tile = TileId::dynamite_lit;
        d.move_rules.set_walkable();
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_item = false;
        add_to_list_and_reset(d);

        d.id = FeatureId::lit_flare;
        d.make_obj = [](const P & p) {
                return new LitFlare(p);
        };
        d.character = '/';
        d.tile = TileId::flare_lit;
        d.move_rules.set_walkable();
        add_to_list_and_reset(d);

        d.id = FeatureId::smoke;
        d.make_obj = [](const P & p) {
                return new Smoke(p);
        };
        d.character = '*';
        d.tile = TileId::smoke;
        d.move_rules.set_walkable();
        d.is_los_passable = false;
        add_to_list_and_reset(d);

        d.id = FeatureId::force_field;
        d.make_obj = [](const P & p) {
                return new ForceField(p);
        };
        d.character = '#';
        d.tile = TileId::square_checkered;
        d.move_rules.reset();
        d.is_sound_passable = false;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.is_smoke_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_rigid = false;
        d.can_have_item = false;
        d.matl_type = Matl::metal;
        add_to_list_and_reset(d);

        d.id = FeatureId::event_wall_crumble;
        d.make_obj = [](const P & p) {
                return new EventWallCrumble(p);
        };
        d.move_rules.set_walkable();
        add_to_list_and_reset(d);

        d.id = FeatureId::event_snake_emerge;
        d.make_obj = [](const P & p) {
                return new EventSnakeEmerge(p);
        };
        d.move_rules.set_walkable();
        add_to_list_and_reset(d);

        d.id = FeatureId::event_rat_cave_discovery;
        d.make_obj = [](const P & p) {
                return new EventRatsInTheWallsDiscovery(p);
        };
        d.move_rules.set_walkable();
        add_to_list_and_reset(d);
}

// -----------------------------------------------------------------------------
// Move rules
// -----------------------------------------------------------------------------
bool MoveRules::can_move(const Actor& actor) const
{
        if (m_is_walkable)
        {
                return true;
        }

        // This feature blocks walking, check if any property overrides this
        // (e.g. flying)

        for (const auto id : m_props_allow_move)
        {
                if (actor.m_properties.has(id))
                {
                        return true;
                }
        }

        return false;
}

// -----------------------------------------------------------------------------
// Feature data
// -----------------------------------------------------------------------------
namespace feature_data
{

void init()
{
        TRACE_FUNC_BEGIN;

        init_data_list();

        TRACE_FUNC_END;
}

const FeatureData& data(const FeatureId id)
{
        ASSERT(id != FeatureId::END);

        return s_data[int(id)];
}

} // feature_data
