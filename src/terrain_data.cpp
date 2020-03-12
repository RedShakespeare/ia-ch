// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "terrain_data.hpp"

#include "init.hpp"

#include "actor.hpp"
#include "game_time.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "terrain.hpp"
#include "terrain_door.hpp"
#include "terrain_event.hpp"
#include "terrain_gong.hpp"
#include "terrain_mob.hpp"
#include "terrain_monolith.hpp"
#include "terrain_pylon.hpp"
#include "terrain_trap.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static terrain::TerrainData s_data[(size_t)terrain::Id::END];

static void reset_data(terrain::TerrainData& d)
{
        d.make_obj = [](const P & p) {(void)p; return nullptr;};
        d.id = terrain::Id::END;
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
        d.can_have_terrain = true;
        d.can_have_item = true;
        d.matl_type = Matl::stone;
        d.msg_on_player_blocked = "The way is blocked.";
        d.msg_on_player_blocked_blind = "I bump into something.";
        d.shock_when_adjacent = 0;
}

static void add_to_list_and_reset(terrain::TerrainData& d)
{
        s_data[(size_t)d.id] = d;

        reset_data(d);
}

static void init_data_list()
{
        terrain::TerrainData d;
        reset_data(d);

        d.id = terrain::Id::floor;
        d.make_obj = [](const P & p) {
                return new terrain::Floor(p);
        };
        d.character = '.';
        d.tile = TileId::floor;
        d.move_rules.set_walkable();
        d.matl_type = Matl::stone;
        d.can_have_gore = true;
        add_to_list_and_reset(d);


        d.id = terrain::Id::bridge;
        d.make_obj = [](const P & p) {
                return new terrain::Bridge(p);
        };
        d.move_rules.set_walkable();
        d.matl_type = Matl::wood;
        add_to_list_and_reset(d);

        d.id = terrain::Id::wall;
        d.make_obj = [](const P & p) {
                return new terrain::Wall(p);
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
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        add_to_list_and_reset(d);

        d.id = terrain::Id::tree;
        d.make_obj = [](const P & p) {
                return new terrain::Tree(p);
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
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.msg_on_player_blocked = "There is a tree in the way.";
        d.matl_type = Matl::wood;
        d.shock_when_adjacent = 1;
        add_to_list_and_reset(d);

        d.id = terrain::Id::grass;
        d.make_obj = [](const P & p) {
                return new terrain::Grass(p);
        };
        d.character = '.';
        d.tile = TileId::floor;
        d.move_rules.set_walkable();
        d.matl_type = Matl::plant;
        d.can_have_gore = true;
        add_to_list_and_reset(d);

        d.id = terrain::Id::bush;
        d.make_obj = [](const P & p) {
                return new terrain::Bush(p);
        };
        d.character = '"';
        d.tile = TileId::bush;
        d.move_rules.set_walkable();
        d.is_los_passable = false;
        d.matl_type = Matl::plant;
        add_to_list_and_reset(d);

        d.id = terrain::Id::vines;
        d.make_obj = [](const P & p) {
                return new terrain::Vines(p);
        };
        d.character = '"';
        d.tile = TileId::vines;
        d.move_rules.set_walkable();
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.matl_type = Matl::plant;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::chains;
        d.make_obj = [](const P & p) {
                return new terrain::Chains(p);
        };
        d.character = '"';
        d.tile = TileId::chains;
        d.move_rules.set_walkable();
        d.is_los_passable = true;
        d.is_projectile_passable = true;
        d.can_have_blood = true;
        d.matl_type = Matl::metal;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::grate;
        d.make_obj = [](const P & p) {
                return new terrain::Grate(p);
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
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::metal;
        add_to_list_and_reset(d);

        d.id = terrain::Id::stairs;
        d.make_obj = [](const P & p) {
                return new terrain::Stairs(p);
        };
        d.character = '>';
        d.tile = TileId::stairs_down;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        add_to_list_and_reset(d);

        d.id = terrain::Id::monolith;
        d.make_obj = [](const P & p) {
                return new terrain::Monolith(p);
        };
        d.character = '|';
        d.tile = TileId::monolith;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false; // We don't want to mess with the color
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 10;
        d.matl_type = Matl::stone;
        add_to_list_and_reset(d);

        d.id = terrain::Id::pylon;
        d.make_obj = [](const P & p) {
                return new terrain::Pylon(p, terrain::PylonId::any);
        };
        d.character = '|';
        d.tile = TileId::pylon;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false; // We don't want to mess with the color
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 10;
        d.matl_type = Matl::metal;
        add_to_list_and_reset(d);

        d.id = terrain::Id::lever;
        d.make_obj = [](const P & p) {
                return new terrain::Lever(p);
        };
        d.character = '%';
        d.tile = TileId::lever_left;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::metal;
        add_to_list_and_reset(d);

        d.id = terrain::Id::brazier;
        d.make_obj = [](const P & p) {
                return new terrain::Brazier(p);
        };
        d.character = '0';
        d.tile = TileId::brazier;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::metal;
        d.auto_spawn_placement = terrain::TerrainPlacement::away_from_walls;
        add_to_list_and_reset(d);

        d.id = terrain::Id::liquid_shallow;
        d.make_obj = [](const P & p) {
                return new terrain::LiquidShallow(p);
        };
        d.character = '~';
        d.tile = TileId::water;
        d.move_rules.set_walkable();
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_terrain = false;
        d.matl_type = Matl::fluid;
        add_to_list_and_reset(d);

        d.id = terrain::Id::liquid_deep;
        d.make_obj = [](const P & p) {
                return new terrain::LiquidDeep(p);
        };
        d.character = '~';
        d.tile = TileId::water;
        d.can_have_item = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_terrain = false;
        d.matl_type = Matl::fluid;
        add_to_list_and_reset(d);

        d.id = terrain::Id::chasm;
        d.make_obj = [](const P & p) {
                return new terrain::Chasm(p);
        };
        d.character = '.';
        d.tile = TileId::floor;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.move_rules.set_prop_can_move(PropId::flying);
        d.can_have_item = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.msg_on_player_blocked = "A chasm lies in my way.";
        d.msg_on_player_blocked_blind =
                "I realize I am standing on the edge of a chasm.";
        d.matl_type = Matl::empty;
        d.shock_when_adjacent = 3;
        add_to_list_and_reset(d);

        d.id = terrain::Id::gravestone;
        d.make_obj = [](const P & p) {
                return new terrain::GraveStone(p);
        };
        d.character = ']';
        d.tile = TileId::grave_stone;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.move_rules.set_prop_can_move(PropId::flying);
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 2;
        d.matl_type = Matl::stone;
        add_to_list_and_reset(d);

        d.id = terrain::Id::church_bench;
        d.make_obj = [](const P & p) {
                return new terrain::ChurchBench(p);
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
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::wood;
        add_to_list_and_reset(d);

        d.id = terrain::Id::carpet;
        d.make_obj = [](const P & p) {
                return new terrain::Carpet(p);
        };
        d.character = '.';
        d.tile = TileId::floor;
        d.can_have_terrain = false;
        d.move_rules.set_walkable();
        d.matl_type = Matl::cloth;
        add_to_list_and_reset(d);

        d.id = terrain::Id::rubble_high;
        d.make_obj = [](const P & p) {
                return new terrain::RubbleHigh(p);
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
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        add_to_list_and_reset(d);

        d.id = terrain::Id::rubble_low;
        d.make_obj = [](const P & p) {
                return new terrain::RubbleLow(p);
        };
        d.character = ',';
        d.tile = TileId::rubble_low;
        d.move_rules.set_walkable();
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::bones;
        d.make_obj = [](const P & p) {
                return new terrain::Bones(p);
        };
        d.character = '&';
        d.tile = TileId::corpse2;
        d.move_rules.set_walkable();
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::statue;
        d.make_obj = [](const P & p) {
                return new terrain::Statue(p);
        };
        d.character = 5; //Paragraph sign
        d.tile = TileId::witch_or_warlock;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::cocoon;
        d.make_obj = [](const P & p) {
                return new terrain::Cocoon(p);
        };
        d.character = '8';
        d.tile = TileId::cocoon_closed;
        d.is_projectile_passable = true;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 3;
        d.matl_type = Matl::cloth;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::chest;
        d.make_obj = [](const P & p) {
                return new terrain::Chest(p);
        };
        d.character = '+';
        d.tile = TileId::chest_closed;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.auto_spawn_placement = terrain::TerrainPlacement::adj_to_walls;
        add_to_list_and_reset(d);

        d.id = terrain::Id::cabinet;
        d.make_obj = [](const P & p) {
                return new terrain::Cabinet(p);
        };
        d.character = '7';
        d.tile = TileId::cabinet_closed;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::wood;
        d.auto_spawn_placement = terrain::TerrainPlacement::adj_to_walls;
        add_to_list_and_reset(d);

        d.id = terrain::Id::bookshelf;
        d.make_obj = [](const P & p) {
                return new terrain::Bookshelf(p);
        };
        d.character = '7';
        d.tile = TileId::bookshelf_full;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::wood;
        d.auto_spawn_placement = terrain::TerrainPlacement::adj_to_walls;
        add_to_list_and_reset(d);

        d.id = terrain::Id::alchemist_bench;
        d.make_obj = [](const P & p) {
                return new terrain::AlchemistBench(p);
        };
        d.character = '7';
        d.tile = TileId::alchemist_bench_full;
        d.is_projectile_passable = false;
        d.is_los_passable = true;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::wood;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::fountain;
        d.make_obj = [](const P & p) {
                return new terrain::Fountain(p);
        };
        d.character = '1';
        d.tile = TileId::fountain;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = terrain::TerrainPlacement::away_from_walls;
        add_to_list_and_reset(d);

        d.id = terrain::Id::stalagmite;
        d.make_obj = [](const P & p) {
                return new terrain::Stalagmite(p);
        };
        d.character = ':';
        d.tile = TileId::stalagmite;
        d.is_projectile_passable = false;
        d.is_los_passable = false;
        d.can_have_blood = true;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::altar;
        d.make_obj = [](const P & p) {
                return new terrain::Altar(p);
        };
        d.character = '_';
        d.tile = TileId::altar;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 10;
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::gong;
        d.make_obj = [](const P & p) {
                return new terrain::Gong(p);
        };
        d.character = '_';
        d.tile = TileId::gong;
        d.is_los_passable = true;
        d.is_projectile_passable = true;
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 5;
        d.matl_type = Matl::metal;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::tomb;
        d.make_obj = [](const P & p) {
                return new terrain::Tomb(p);
        };
        d.character = ']';
        d.tile = TileId::tomb_closed;
        d.move_rules.set_prop_can_move(PropId::ethereal);
        d.move_rules.set_prop_can_move(PropId::flying);
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.shock_when_adjacent = 10;
        d.matl_type = Matl::stone;
        d.auto_spawn_placement = terrain::TerrainPlacement::either;
        add_to_list_and_reset(d);

        d.id = terrain::Id::door;
        d.make_obj = [](const P & p) {
                return new terrain::Door(p);
        };
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_terrain = false;
        d.can_have_item = false;
        add_to_list_and_reset(d);

        d.id = terrain::Id::trap;
        d.make_obj = [](const P & p) {
                return new terrain::Trap(p);
        };
        d.move_rules.set_walkable();
        d.can_have_terrain = false;
        add_to_list_and_reset(d);

        d.id = terrain::Id::lit_dynamite;
        d.make_obj = [](const P & p) {
                return new terrain::LitDynamite(p);
        };
        d.character = '/';
        d.tile = TileId::dynamite_lit;
        d.move_rules.set_walkable();
        d.can_have_blood = false;
        d.can_have_gore = false;
        d.can_have_corpse = false;
        d.can_have_item = false;
        add_to_list_and_reset(d);

        d.id = terrain::Id::lit_flare;
        d.make_obj = [](const P & p) {
                return new terrain::LitFlare(p);
        };
        d.character = '/';
        d.tile = TileId::flare_lit;
        d.move_rules.set_walkable();
        add_to_list_and_reset(d);

        d.id = terrain::Id::smoke;
        d.make_obj = [](const P & p) {
                return new terrain::Smoke(p);
        };
        d.character = '*';
        d.tile = TileId::smoke;
        d.move_rules.set_walkable();
        d.is_los_passable = false;
        add_to_list_and_reset(d);

        d.id = terrain::Id::force_field;
        d.make_obj = [](const P & p) {
                return new terrain::ForceField(p);
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
        d.can_have_terrain = false;
        d.can_have_item = false;
        d.matl_type = Matl::metal;
        add_to_list_and_reset(d);

        d.id = terrain::Id::event_wall_crumble;
        d.make_obj = [](const P & p) {
                return new terrain::EventWallCrumble(p);
        };
        d.move_rules.set_walkable();
        add_to_list_and_reset(d);

        d.id = terrain::Id::event_snake_emerge;
        d.make_obj = [](const P & p) {
                return new terrain::EventSnakeEmerge(p);
        };
        d.move_rules.set_walkable();
        add_to_list_and_reset(d);

        d.id = terrain::Id::event_rat_cave_discovery;
        d.make_obj = [](const P & p) {
                return new terrain::EventRatsInTheWallsDiscovery(p);
        };
        d.move_rules.set_walkable();
        add_to_list_and_reset(d);
}

// -----------------------------------------------------------------------------
// terrain
// -----------------------------------------------------------------------------
namespace terrain
{

bool MoveRules::can_move(const actor::Actor& actor) const
{
        if (m_is_walkable)
        {
                return true;
        }

        // This terrain blocks walking, check if any property overrides this
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

void init()
{
        TRACE_FUNC_BEGIN;

        init_data_list();

        TRACE_FUNC_END;
}

const TerrainData& data(const Id id)
{
        ASSERT(id != terrain::Id::END);

        return s_data[int(id)];
}

} // namespace terrain
