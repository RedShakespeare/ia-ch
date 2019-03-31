// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "room.hpp"

#include <algorithm>
#include <climits>
#include <numeric>

#include "actor_factory.hpp"
#include "actor_player.hpp"
#include "terrain.hpp"
#include "flood.hpp"
#include "game_time.hpp"
#include "gods.hpp"
#include "init.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "mapgen.hpp"
#include "misc.hpp"
#include "populate_monsters.hpp"

#ifndef NDEBUG
#include "io.hpp"
#include "sdl_base.hpp"
#endif // NDEBUG

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<RoomType> s_room_bucket;


static void add_to_room_bucket(const RoomType type, const size_t nr)
{
        if (nr > 0)
        {
                s_room_bucket.reserve(s_room_bucket.size() + nr);

                for (size_t i = 0; i < nr; ++i)
                {
                        s_room_bucket.push_back(type);
                }
        }
}

// NOTE: This cannot be a virtual class method, since a room of a certain
// RoomType doesn't have to be an instance of the corresponding Room child class
// (it could be for example a TemplateRoom object with RoomType "ritual", in
// that case we still want the same chance to make it dark).
static int base_pct_chance_dark(const RoomType room_type)
{
        switch (room_type)
        {
        case RoomType::plain:
                return 5;

        case RoomType::human:
                return 10;

        case RoomType::ritual:
                return 15;

        case RoomType::jail:
                return 60;

        case RoomType::spider:
                return 50;

        case RoomType::snake_pit:
                return 75;

        case RoomType::crypt:
                return 60;

        case RoomType::monster:
                return 80;

        case RoomType::damp:
                return 25;

        case RoomType::pool:
                return 25;

        case RoomType::cave:
                return 30;

        case RoomType::chasm:
                return 25;

        case RoomType::forest:
                return 10;

        case RoomType::END_OF_STD_ROOMS:
        case RoomType::corr_link:
        case RoomType::crumble_room:
        case RoomType::river:
                break;
        }

        return 0;
}

static int walk_blockers_in_dir(const Dir dir, const P& pos)
{
        int nr_blockers = 0;

        switch (dir)
        {
        case Dir::right:
                for (int dy = -1; dy <= 1; ++dy)
                {
                        const auto* const t =
                                map::g_cells.at(pos.x + 1, pos.y + dy).terrain;

                        if (!t->is_walkable())
                        {
                                nr_blockers += 1;
                        }
                }
                break;

        case Dir::down:
                for (int dx = -1; dx <= 1; ++dx)
                {
                        const auto* const t =
                                map::g_cells.at(pos.x + dx, pos.y + 1).terrain;

                        if (!t->is_walkable())
                        {
                                nr_blockers += 1;
                        }
                }
                break;

        case Dir::left:
                for (int dy = -1; dy <= 1; ++dy)
                {
                        const auto* const t =
                                map::g_cells.at(pos.x - 1, pos.y + dy).terrain;

                        if (!t->is_walkable())
                        {
                                nr_blockers += 1;
                        }
                }
                break;

        case Dir::up:
                for (int dx = -1; dx <= 1; ++dx)
                {
                        const auto* const t =
                                map::g_cells.at(pos.x + dx, pos.y - 1).terrain;

                        if (!t->is_walkable())
                        {
                                nr_blockers += 1;
                        }
                }
                break;

        case Dir::down_left:
        case Dir::down_right:
        case Dir::up_left:
        case Dir::up_right:
        case Dir::center:
        case Dir::END:
                break;
        }

        return nr_blockers;
} // walk_blockers_in_dir

static void get_positions_in_room_relative_to_walls(
        const Room& room,
        std::vector<P>& adj_to_walls,
        std::vector<P>& away_from_walls)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        adj_to_walls.clear();

        away_from_walls.clear();

        std::vector<P> pos_bucket;

        pos_bucket.clear();

        const auto& r = room.m_r;

        for (int x = r.p0.x; x <= r.p1.x; ++x)
        {
                for (int y = r.p0.y; y <= r.p1.y; ++y)
                {
                        if (map::g_room_map.at(x, y) != &room)
                        {
                                continue;
                        }

                        auto* const t = map::g_cells.at(x, y).terrain;

                        if (t->is_walkable() && t->can_have_terrain())
                        {
                                pos_bucket.push_back(P(x, y));
                        }
                }
        }

        for (P& pos : pos_bucket)
        {
                const int nr_r = walk_blockers_in_dir(Dir::right, pos);
                const int nr_d = walk_blockers_in_dir(Dir::down, pos);
                const int nr_l = walk_blockers_in_dir(Dir::left, pos);
                const int nr_u = walk_blockers_in_dir(Dir::up, pos);

                const bool is_zero_all_dir =
                        nr_r == 0 &&
                        nr_d == 0 &&
                        nr_l == 0 &&
                        nr_u == 0;

                if (is_zero_all_dir)
                {
                        away_from_walls.push_back(pos);
                        continue;
                }

                bool is_door_adjacent = false;

                for (int dx = -1; dx <= 1; ++dx)
                {
                        for (int dy = -1; dy <= 1; ++dy)
                        {
                                const auto* const t =
                                        map::g_cells.at(pos.x + dx, pos.y + dy)
                                        .terrain;

                                if (t->id() == terrain::Id::door)
                                {
                                        is_door_adjacent = true;
                                }
                        }
                }

                if (is_door_adjacent)
                {
                        continue;
                }

                if ((nr_r == 3 && nr_u == 1 && nr_d == 1 && nr_l == 0) ||
                    (nr_r == 1 && nr_u == 3 && nr_d == 0 && nr_l == 1) ||
                    (nr_r == 1 && nr_u == 0 && nr_d == 3 && nr_l == 1) ||
                    (nr_r == 0 && nr_u == 1 && nr_d == 1 && nr_l == 3))
                {
                        adj_to_walls.push_back(pos);

                        continue;
                }

        }

        TRACE_FUNC_END_VERBOSE;
} // cells_in_room

// -----------------------------------------------------------------------------
// Room factory
// -----------------------------------------------------------------------------
namespace room_factory
{

void init_room_bucket()
{
        TRACE_FUNC_BEGIN;

        s_room_bucket.clear();

        const int dlvl = map::g_dlvl;

        if (dlvl <= g_dlvl_last_early_game)
        {
                add_to_room_bucket(RoomType::human, rnd::range(4, 5));
                add_to_room_bucket(RoomType::jail, 1);
                add_to_room_bucket(RoomType::ritual, rnd::coin_toss() ? 1 : 0);
                add_to_room_bucket(RoomType::crypt, rnd::range(2, 3));
                add_to_room_bucket(RoomType::monster, 1);
                add_to_room_bucket(RoomType::damp, rnd::range(1, 2));
                add_to_room_bucket(RoomType::pool, rnd::range(2, 3));
                add_to_room_bucket(RoomType::snake_pit, 1);

                const size_t nr_plain_rooms = s_room_bucket.size() * 2;

                add_to_room_bucket(RoomType::plain, nr_plain_rooms);
        }
        else if (dlvl <= g_dlvl_last_mid_game)
        {
                add_to_room_bucket(RoomType::human, rnd::range(2, 3));
                add_to_room_bucket(RoomType::jail, rnd::range(1, 2));
                add_to_room_bucket(RoomType::ritual, rnd::coin_toss() ? 1 : 0);
                add_to_room_bucket(RoomType::spider, rnd::range(1, 3));
                add_to_room_bucket(RoomType::snake_pit, 1);
                add_to_room_bucket(RoomType::crypt, 4);
                add_to_room_bucket(RoomType::monster, 2);
                add_to_room_bucket(RoomType::damp, rnd::range(1, 3));
                add_to_room_bucket(RoomType::pool, rnd::range(2, 3));
                add_to_room_bucket(RoomType::cave, 2);
                add_to_room_bucket(RoomType::chasm, 1);
                add_to_room_bucket(RoomType::forest, 2);

                const size_t nr_plain_rooms = s_room_bucket.size() * 2;

                add_to_room_bucket(RoomType::plain, nr_plain_rooms);
        }
        else // Late game
        {
                add_to_room_bucket(RoomType::monster, 1);
                add_to_room_bucket(RoomType::spider, 1);
                add_to_room_bucket(RoomType::snake_pit, 1);
                add_to_room_bucket(RoomType::damp, rnd::range(1, 3));
                add_to_room_bucket(RoomType::pool, rnd::range(2, 3));
                add_to_room_bucket(RoomType::chasm, 2);
                add_to_room_bucket(RoomType::forest, 2);

                const size_t nr_cave_rooms = s_room_bucket.size() * 2;

                add_to_room_bucket(RoomType::cave, nr_cave_rooms);
        }

        rnd::shuffle(s_room_bucket);

        TRACE_FUNC_END;
}

Room* make(const RoomType type, const R& r)
{
        switch (type)
        {
        case RoomType::cave:
                return new CaveRoom(r);

        case RoomType::chasm:
                return new ChasmRoom(r);

        case RoomType::crypt:
                return new CryptRoom(r);

        case RoomType::damp:
                return new DampRoom(r);

        case RoomType::pool:
                return new PoolRoom(r);

        case RoomType::human:
                return new HumanRoom(r);

        case RoomType::jail:
                return new JailRoom(r);

        case RoomType::monster:
                return new MonsterRoom(r);

        case RoomType::snake_pit:
                return new SnakePitRoom(r);

        case RoomType::plain:
                return new PlainRoom(r);

        case RoomType::ritual:
                return new RitualRoom(r);

        case RoomType::spider:
                return new SpiderRoom(r);

        case RoomType::forest:
                return new ForestRoom(r);

        case RoomType::corr_link:
                return new CorrLinkRoom(r);

        case RoomType::crumble_room:
                return new CrumbleRoom(r);

        case RoomType::river:
                return new RiverRoom(r);

                // Does not have room classes
        case RoomType::END_OF_STD_ROOMS:
                TRACE << "Illegal room type id: " << (int)type << std::endl;

                ASSERT(false);

                return nullptr;
        }

        ASSERT(false);

        return nullptr;
}

Room* make_random_room(const R& r, const IsSubRoom is_subroom)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        auto s_room_bucketit = std::begin(s_room_bucket);

        Room* room = nullptr;

        while (true)
        {
                if (s_room_bucketit == std::end(s_room_bucket))
                {
                        // No more rooms to pick from, generate a new bucket
                        init_room_bucket();

                        s_room_bucketit = std::begin(s_room_bucket);
                }
                else // There are still room types in the bucket
                {
                        const RoomType room_type = *s_room_bucketit;

                        room = make(room_type, r);

                        // NOTE: This must be set before "is_allowed()" below is
                        // called (some room types should never exist as sub
                        // rooms)
                        room->m_is_sub_room = is_subroom == IsSubRoom::yes;

                        StdRoom* const std_room = static_cast<StdRoom*>(room);

                        if (std_room->is_allowed())
                        {
                                s_room_bucket.erase(s_room_bucketit);

                                TRACE_FUNC_END_VERBOSE << "Made room type: "
                                                       << (int)room_type
                                                       << std::endl;
                                break;
                        }
                        else // Room not allowed (e.g. wrong dimensions)
                        {
                                delete room;

                                // Try next room type in the bucket
                                ++s_room_bucketit;
                        }
                }
        }

        TRACE_FUNC_END_VERBOSE;

        return room;
}

} // room_factory

// -----------------------------------------------------------------------------
// Room
// -----------------------------------------------------------------------------
Room::Room(R r, RoomType type) :
        m_r(r),
        m_type(type),
        m_is_sub_room(false) {}

std::vector<P> Room::positions_in_room() const
{
        std::vector<P> positions;

        positions.reserve(m_r.w() * m_r.h());

        for (int x = m_r.p0.x; x <= m_r.p1.x; ++x)
        {
                for (int y = m_r.p0.y; y <= m_r.p1.y; ++y)
                {
                        if (map::g_room_map.at(x, y) == this)
                        {
                                positions.push_back(P(x, y));
                        }
                }
        }

        return positions;
}

void Room::make_dark() const
{
        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                if (map::g_room_map.at(i) == this)
                {
                        map::g_dark.at(i) = true;
                }
        }

        // Also make sub rooms dark
        for (Room* const sub_room : m_sub_rooms)
        {
                sub_room->make_dark();
        }
}

// -----------------------------------------------------------------------------
// Standard room
// -----------------------------------------------------------------------------
void StdRoom::on_pre_connect(Array2<bool>& door_proposals)
{
        on_pre_connect_hook(door_proposals);
}

void StdRoom::on_post_connect(Array2<bool>& door_proposals)
{
        place_auto_terrains();

        on_post_connect_hook(door_proposals);

        // Make the room dark?

        // Do not make the room dark if it has a light source

        bool has_light_source = false;

        for (int x = m_r.p0.x; x <= m_r.p1.x; ++x)
        {
                for (int y = m_r.p0.y; y <= m_r.p1.y; ++y)
                {
                        // TODO: This should really be handled in a more generic
                        // way, but currently the only map terrains that are
                        // light sources are braziers - so it works for now.

                        const auto id = map::g_cells.at(x, y).terrain->id();

                        if (id == terrain::Id::brazier)
                        {
                                has_light_source = true;
                        }
                }
        }

        if (!has_light_source)
        {
                int pct_chance_dark = base_pct_chance_dark(m_type) - 15;

                // Increase chance with deeper dungeon levels
                pct_chance_dark += map::g_dlvl;

                set_constr_in_range(0, pct_chance_dark, 100);

                if (rnd::percent(pct_chance_dark))
                {
                        make_dark();
                }
        }
}

P StdRoom::find_auto_terrain_placement(
        const std::vector<P>& adj_to_walls,
        const std::vector<P>& away_from_walls,
        const terrain::Id id) const
{
        TRACE_FUNC_BEGIN_VERBOSE;

        const bool is_adj_to_walls_avail = !adj_to_walls.empty();
        const bool is_away_from_walls_avail = !away_from_walls.empty();

        if (!is_adj_to_walls_avail &&
            !is_away_from_walls_avail)
        {
                TRACE_FUNC_END_VERBOSE << "No eligible cells found"
                                       << std::endl;

                return P(-1, -1);
        }

        // TODO: This method is crap, use a bucket instead!

        const int nr_attempts_to_find_pos = 100;

        for (int i = 0; i < nr_attempts_to_find_pos; ++i)
        {
                const auto& d = terrain::data(id);

                if (is_adj_to_walls_avail &&
                    (d.auto_spawn_placement ==
                     terrain::TerrainPlacement::adj_to_walls))
                {
                        TRACE_FUNC_END_VERBOSE;
                        return rnd::element(adj_to_walls);
                }

                if (is_away_from_walls_avail &&
                    (d.auto_spawn_placement ==
                     terrain::TerrainPlacement::away_from_walls))
                {
                        TRACE_FUNC_END_VERBOSE;
                        return rnd::element(away_from_walls);
                }

                if (d.auto_spawn_placement == terrain::TerrainPlacement::either)
                {
                        if (rnd::coin_toss())
                        {
                                if (is_adj_to_walls_avail)
                                {
                                        TRACE_FUNC_END_VERBOSE;
                                        return rnd::element(adj_to_walls);

                                }
                        }
                        else // Coint toss
                        {
                                if (is_away_from_walls_avail)
                                {
                                        TRACE_FUNC_END_VERBOSE;
                                        return rnd::element(away_from_walls);
                                }
                        }
                }
        }

        TRACE_FUNC_END_VERBOSE;
        return P(-1, -1);
}

void StdRoom::place_auto_terrains()
{
        TRACE_FUNC_BEGIN;

        // Make a terrain bucket
        std::vector<terrain::Id> terrain_bucket;

        const auto rules = auto_terrains_allowed();

        for (const auto& rule : rules)
        {
                // Insert N elements of the given Terrain ID
                terrain_bucket.insert(
                        std::end(terrain_bucket),
                        rule.nr_allowed,
                        rule.id);
        }

        std::vector<P> adj_to_walls_bucket;
        std::vector<P> away_from_walls_bucket;

        get_positions_in_room_relative_to_walls(
                *this,
                adj_to_walls_bucket,
                away_from_walls_bucket);

        while (!terrain_bucket.empty())
        {
                // TODO: Do a random shuffle of the bucket instead, and pop
                // elements
                const size_t terrain_idx =
                        rnd::range(0, terrain_bucket.size() - 1);

                const auto id = terrain_bucket[terrain_idx];

                terrain_bucket.erase(std::begin(terrain_bucket) + terrain_idx);

                const P p = find_auto_terrain_placement(
                        adj_to_walls_bucket,
                        away_from_walls_bucket,
                        id);

                if (p.x >= 0)
                {
                        // A good position was found

                        const auto& d = terrain::data(id);

                        TRACE_VERBOSE << "Placing terrain" << std::endl;

                        ASSERT(map::is_pos_inside_outer_walls(p));

                        map::put(static_cast<terrain::Terrain*>(d.make_obj(p)));

                        // Erase all adjacent positions
                        auto is_adj = [&](const P& other_p) {
                                return is_pos_adj(p, other_p, true);
                        };

                        adj_to_walls_bucket.erase(
                                std::remove_if(
                                        std::begin(adj_to_walls_bucket),
                                        std::end(adj_to_walls_bucket),
                                        is_adj),
                                std::end(adj_to_walls_bucket));

                        away_from_walls_bucket.erase(
                                std::remove_if(
                                        std::begin(away_from_walls_bucket),
                                        std::end(away_from_walls_bucket),
                                        is_adj),
                                std::end(away_from_walls_bucket));
                }
        }
}

// -----------------------------------------------------------------------------
// Plain room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> PlainRoom::auto_terrains_allowed() const
{
        const int fountain_one_in_n =
                (map::g_dlvl <= 4) ? 7 :
                (map::g_dlvl <= g_dlvl_last_mid_game) ? 10 :
                20;

        return
        {
                {terrain::Id::brazier, rnd::one_in(4) ? 1 : 0},
                {terrain::Id::statue, rnd::one_in(7) ? rnd::range(1, 2) : 0},
                {terrain::Id::fountain, rnd::one_in(fountain_one_in_n) ? 1 : 0},
                {terrain::Id::chains, rnd::one_in(7) ? rnd::range(1, 2) : 0}
        };
}

void PlainRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                mapgen::cut_room_corners(*this);
        }

        if (rnd::fraction(1, 4))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void PlainRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;
}

// -----------------------------------------------------------------------------
// Human room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> HumanRoom::auto_terrains_allowed() const
{
        std::vector<RoomAutoTerrainRule> result;

        result.push_back({terrain::Id::brazier,   rnd::range(0, 2)});
        result.push_back({terrain::Id::statue,    rnd::range(0, 2)});

        // Control how many item container terrains that can spawn in the room
        std::vector<terrain::Id> item_containers = {
                terrain::Id::chest,
                terrain::Id::cabinet,
                terrain::Id::bookshelf,
                terrain::Id::alchemist_bench
        };

        const int nr_item_containers = rnd::range(0, 2);

        for (int i = 0; i < nr_item_containers; ++i)
        {
                result.push_back({rnd::element(item_containers), 1});
        }

        return result;
}

bool HumanRoom::is_allowed() const
{
        return
                m_r.min_dim() >= 3 &&
                m_r.max_dim() <= 8;
}

void HumanRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                mapgen::cut_room_corners(*this);
        }

        if (rnd::fraction(1, 4))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void HumanRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                Array2<bool> blocked(map::dims());

                map_parsers::BlocksWalking(ParseActors::no)
                        .run(blocked, blocked.rect());

                for (int x = m_r.p0.x + 1; x <= m_r.p1.x - 1; ++x)
                {
                        for (int y = m_r.p0.y + 1; y <= m_r.p1.y - 1; ++y)
                        {
                                if (!blocked.at(x, y) &&
                                    map::g_room_map.at(x, y) == this)
                                {
                                        auto* const carpet =
                                                new terrain::Carpet(P(x, y));

                                        map::put(carpet);
                                }
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// Jail room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> JailRoom::auto_terrains_allowed() const
{
        return
        {
                {terrain::Id::chains, rnd::range(2, 8)},
                {terrain::Id::brazier, rnd::one_in(4) ? 1 : 0},
                {terrain::Id::rubble_low, rnd::range(1, 4)}
        };
}

void JailRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                mapgen::cut_room_corners(*this);
        }

        if (rnd::fraction(1, 4))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void JailRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;
}

// -----------------------------------------------------------------------------
// Ritual room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> RitualRoom::auto_terrains_allowed() const
{
        return
        {
                {terrain::Id::altar, 1},
                {terrain::Id::alchemist_bench, rnd::one_in(4) ? 1 : 0},
                {terrain::Id::brazier, rnd::range(2, 4)},
                {terrain::Id::chains, rnd::one_in(7) ? rnd::range(1, 2) : 0}
        };
}

bool RitualRoom::is_allowed() const
{
        return
                m_r.min_dim() >= 4 &&
                m_r.max_dim() <= 8;
}

void RitualRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                mapgen::cut_room_corners(*this);
        }

        if (rnd::fraction(1, 4))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void RitualRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::no)
                .run(blocked, blocked.rect());

        const int bloody_chamber_pct = 60;

        if (rnd::percent(bloody_chamber_pct))
        {
                P origin(-1, -1);
                std::vector<P> origin_bucket;

                for (int y = m_r.p0.y; y <= m_r.p1.y; ++y)
                {
                        for (int x = m_r.p0.x; x <= m_r.p1.x; ++x)
                        {
                                if (map::g_cells.at(x, y).terrain->id() ==
                                    terrain::Id::altar)
                                {
                                        origin = P(x, y);
                                        y = 999;
                                        x = 999;
                                }
                                else
                                {
                                        if (!blocked.at(x, y))
                                        {
                                                origin_bucket.push_back(
                                                        P(x, y));
                                        }
                                }
                        }
                }

                if (!origin_bucket.empty())
                {
                        if (origin.x == -1)
                        {
                                const int element =
                                        rnd::range(0, origin_bucket.size() - 1);

                                origin = origin_bucket[element];
                        }

                        for (const P& d : dir_utils::g_dir_list)
                        {
                                if (rnd::percent(bloody_chamber_pct / 2))
                                {
                                        const P pos = origin + d;

                                        if (!blocked.at(pos))
                                        {
                                                map::make_gore(pos);
                                                map::make_blood(pos);
                                        }
                                }
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// Spider room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> SpiderRoom::auto_terrains_allowed() const
{
        return
        {
                {terrain::Id::cocoon, rnd::range(0, 3)}
        };
}

bool SpiderRoom::is_allowed() const
{
        return
                m_r.min_dim() >= 3 &&
                m_r.max_dim() <= 8;
}

void SpiderRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        const bool is_early = map::g_dlvl <= g_dlvl_last_early_game;

        const bool is_mid = !is_early && (map::g_dlvl <= g_dlvl_last_mid_game);

        if (is_early || (is_mid && rnd::coin_toss()))
        {
                if (rnd::coin_toss())
                {
                        mapgen::cut_room_corners(*this);
                }
        }
        else
        {
                mapgen::cavify_room(*this);
        }

        if ((is_early || is_mid) && rnd::fraction(1, 4))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void SpiderRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;
}

// -----------------------------------------------------------------------------
// Snake pit room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> SnakePitRoom::auto_terrains_allowed() const
{
        return {};
}

bool SnakePitRoom::is_allowed() const
{
        return
                m_r.min_dim() >= 2 &&
                m_r.max_dim() <= 6;
}

void SnakePitRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if ((map::g_dlvl >= g_dlvl_first_late_game) || rnd::fraction(3, 4))
        {
                mapgen::cavify_room(*this);
        }
        else // Late game
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void SnakePitRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        // Put lots of rubble, to make the room more "pit like"
        for (int x = m_r.p0.x; x <= m_r.p1.x; ++x)
        {
                for (int y = m_r.p0.y; y <= m_r.p1.y; ++y)
                {
                        if (map::g_room_map.at(x, y) != this)
                        {
                                continue;
                        }

                        const P p(x, y);

                        if (map::g_cells.at(x, y).terrain->can_have_terrain() &&
                            rnd::coin_toss())
                        {
                                map::put(new terrain::RubbleLow(p));
                        }
                }
        }
}

void SnakePitRoom::populate_monsters() const
{
        std::vector<actor::Id> actor_id_bucket;

        for (size_t i = 0; i < (size_t)actor::Id::END; ++i)
        {
                const auto& d = actor::g_data[i];

                // NOTE: We do not allow Spitting Cobras in snake pits, because
                // it's VERY tedious to fight swarms of them (attack, get
                // blinded, back away, repeat...)
                if (d.is_snake && (d.id != actor::Id::spitting_cobra))
                {
                        actor_id_bucket.push_back(d.id);
                }
        }

        // Hijacking snake pit rooms to make a worm room...
        if (map::g_dlvl <= g_dlvl_last_mid_game)
        {
                actor_id_bucket.push_back(actor::Id::worm_mass);
        }

        if (map::g_dlvl >= 3)
        {
                actor_id_bucket.push_back(actor::Id::mind_worms);
        }

        const auto actor_id = rnd::element(actor_id_bucket);

        auto blocked = populate_mon::forbidden_spawn_positions();

        const int nr_groups = rnd::range(3, 4);

        for (int group_idx = 0; group_idx < nr_groups; ++group_idx)
        {
                // Find an origin
                std::vector<P> origin_bucket;

                for (int x = m_r.p0.x; x <= m_r.p1.x; ++x)
                {
                        for (int y = m_r.p0.y; y <= m_r.p1.y; ++y)
                        {
                                if (!blocked.at(x, y) &&
                                    (map::g_room_map.at(x, y) == this))
                                {
                                        origin_bucket.push_back(P(x, y));
                                }
                        }
                }

                if (origin_bucket.empty())
                {
                        return;
                }

                const P origin(rnd::element(origin_bucket));

                const auto sorted_free_cells =
                        populate_mon::make_sorted_free_cells(origin, blocked);

                if (sorted_free_cells.empty())
                {
                        return;
                }

                populate_mon::make_group_at(
                        actor_id,
                        sorted_free_cells,
                        &blocked, // New blocked cells (output)
                        MonRoamingAllowed::yes);
        }
}

// -----------------------------------------------------------------------------
// Crypt room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> CryptRoom::auto_terrains_allowed() const
{
        return
        {
                {terrain::Id::tomb, rnd::one_in(6) ? 2 : 1},
                {terrain::Id::rubble_low, rnd::range(1, 4)}
        };
}

bool CryptRoom::is_allowed() const
{
        return
                m_r.min_dim() >= 2 &&
                m_r.max_dim() <= 12;
}

void CryptRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                mapgen::cut_room_corners(*this);
        }

        if (rnd::fraction(1, 3))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void CryptRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;
}

// -----------------------------------------------------------------------------
// Monster room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> MonsterRoom::auto_terrains_allowed() const
{
        return
        {
                {terrain::Id::rubble_low, rnd::range(3, 6)}
        };
}

bool MonsterRoom::is_allowed() const
{
        return
                m_r.min_dim() >= 4 &&
                m_r.max_dim() <= 8;
}

void MonsterRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        const bool is_early =
                map::g_dlvl <= g_dlvl_last_early_game;

        const bool is_mid =
                !is_early &&
                (map::g_dlvl <= g_dlvl_last_mid_game);

        if (is_early || is_mid)
        {
                if (rnd::fraction(3, 4))
                {
                        mapgen::cut_room_corners(*this);
                }

                if (rnd::fraction(1, 3))
                {
                        mapgen::make_pillars_in_room(*this);
                }
        }
        else // Is late game
        {
                mapgen::cavify_room(*this);
        }
}

void MonsterRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::no)
                .run(blocked, blocked.rect());

        int nr_blood_put = 0;

        // TODO: Hacky, needs improving
        const int nr_tries = 1000;

        for (int i = 0; i < nr_tries; ++i)
        {
                for (int x = m_r.p0.x; x <= m_r.p1.x; ++x)
                {
                        for (int y = m_r.p0.y; y <= m_r.p1.y; ++y)
                        {
                                if (!blocked.at(x, y) &&
                                    map::g_room_map.at(x, y) == this &&
                                    rnd::fraction(2, 5))
                                {
                                        map::make_gore(P(x, y));
                                        map::make_blood(P(x, y));
                                        nr_blood_put++;
                                }
                        }
                }

                if (nr_blood_put > 0)
                {
                        break;
                }
        }
}

// -----------------------------------------------------------------------------
// Damp room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> DampRoom::auto_terrains_allowed() const
{
        return
        {
                {terrain::Id::vines, rnd::coin_toss() ? rnd::range(2, 8) : 0}
        };
}

bool DampRoom::is_allowed() const
{
        return true;
}

void DampRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        const bool is_early = map::g_dlvl <= g_dlvl_last_early_game;

        const bool is_mid = !is_early && (map::g_dlvl <= g_dlvl_last_mid_game);

        if (is_early || (is_mid && rnd::coin_toss()))
        {
                if (rnd::coin_toss())
                {
                        mapgen::cut_room_corners(*this);
                }
        }
        else
        {
                mapgen::cavify_room(*this);
        }

        if ((is_early || is_mid) && rnd::fraction(1, 3))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void DampRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::no)
                .run(blocked, blocked.rect());

        const int liquid_one_in_n = rnd::range(2, 5);

        const auto liquid_type =
                rnd::fraction(3, 4)
                ? LiquidType::water
                : LiquidType::mud;

        for (int x = m_r.p0.x; x <= m_r.p1.x; ++x)
        {
                for (int y = m_r.p0.y; y <= m_r.p1.y; ++y)
                {
                        if (!blocked.at(x, y) &&
                            map::g_room_map.at(x, y) == this &&
                            rnd::one_in(liquid_one_in_n))
                        {
                                auto* const liquid =
                                        new terrain::LiquidShallow(P(x, y));

                                liquid->m_type = liquid_type;

                                map::put(liquid);
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// Pool room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> PoolRoom::auto_terrains_allowed() const
{
        return
        {
                {terrain::Id::vines, rnd::coin_toss() ? rnd::range(2, 8) : 0}
        };
}

bool PoolRoom::is_allowed() const
{
        return m_r.min_dim() >= 5;
}

void PoolRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        const bool is_early = map::g_dlvl <= g_dlvl_last_early_game;

        const bool is_mid = !is_early && (map::g_dlvl <= g_dlvl_last_mid_game);

        if (is_early ||
            (is_mid && rnd::coin_toss()) ||
            m_is_sub_room)
        {
                if (rnd::coin_toss())
                {
                        mapgen::cut_room_corners(*this);
                }
        }
        else
        {
                mapgen::cavify_room(*this);
        }

        if ((is_early || is_mid) && rnd::fraction(1, 3))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void PoolRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::no)
                .run(blocked, blocked.rect());

        std::vector<P> origin_bucket;

        origin_bucket.reserve(m_r.w() * m_r.h());

        for (int x = 0; x < blocked.w(); ++x)
        {
                for (int y = 0; y < blocked.w(); ++y)
                {
                        if (blocked.at(x, y))
                        {
                                continue;
                        }

                        if (map::g_room_map.at(x, y) == this)
                        {
                                origin_bucket.push_back(P(x, y));
                        }
                        else
                        {
                                blocked.at(x, y) = true;
                        }
                }
        }

        if (origin_bucket.empty())
        {
                // There are no free room positions
                ASSERT(false);

                return;
        }

        const P origin = rnd::element(origin_bucket);

        const int flood_travel_limit = 12;

        const bool allow_diagonal_flood = true;

        const auto flood =
                floodfill(
                        origin,
                        blocked,
                        flood_travel_limit,
                        P(-1, -1), // Target cell
                        allow_diagonal_flood);

        // Do not place any liquid if we cannot place at least a certain amount
        {
                const int min_nr_liquid_cells = 9;

                int nr_liquid_cells = 0;

                for (int x = 0; x < flood.w(); ++x)
                {
                        for (int y = 0; y < flood.w(); ++y)
                        {
                                const P p(x, y);

                                const bool should_put_liquid =
                                        (flood.at(p) > 0) || (p == origin);

                                if (should_put_liquid)
                                {
                                        ++nr_liquid_cells;
                                }
                        }
                }

                if (nr_liquid_cells < min_nr_liquid_cells)
                {
                        return;
                }
        }

        // OK, we can place liquid

        bool is_natural_pool = false;

        if (map::g_dlvl >= g_dlvl_first_late_game)
        {
                is_natural_pool = true;
        }
        else if (map::g_dlvl >= g_dlvl_first_mid_game)
        {
                is_natural_pool = rnd::coin_toss();
        }
        else
        {
                is_natural_pool = rnd::fraction(1, 4);
        }

        for (int x = 0; x < flood.w(); ++x)
        {
                for (int y = 0; y < flood.w(); ++y)
                {
                        const P p(x, y);

                        const bool should_put_liquid =
                                (flood.at(p) > 0) || (p == origin);

                        if (!should_put_liquid)
                        {
                                continue;
                        }

                        if (!is_natural_pool ||
                            (flood.at(p) < (flood_travel_limit / 2)) ||
                            rnd::coin_toss())
                        {
                                auto* const liquid =
                                        new terrain::LiquidDeep(p);

                                liquid->m_type = LiquidType::water;

                                map::put(liquid);
                        }
                        else if (rnd::fraction(2, 3))
                        {
                                auto* const liquid =
                                        new terrain::LiquidShallow(p);

                                liquid->m_type = LiquidType::water;

                                map::put(liquid);
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// Cave room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> CaveRoom::auto_terrains_allowed() const
{
        return
        {
                {terrain::Id::rubble_low, rnd::range(2, 4)},
                {terrain::Id::stalagmite, rnd::range(1, 4)}
        };
}

bool CaveRoom::is_allowed() const
{
        return !m_is_sub_room;
}

void CaveRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        mapgen::cavify_room(*this);
}

void CaveRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;
}

// -----------------------------------------------------------------------------
// Forest room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> ForestRoom::auto_terrains_allowed() const
{
        return
        {
                {terrain::Id::brazier, rnd::range(0, 1)}
        };
}

bool ForestRoom::is_allowed() const
{
        // TODO: Also check sub_rooms_.empty() ?
        return
                !m_is_sub_room &&
                m_r.min_dim() >= 5;
}

void ForestRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        mapgen::cavify_room(*this);
}

void ForestRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::no)
                .run(blocked, blocked.rect());

        // Do not consider doors blocking
        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                if (map::g_cells.at(i).terrain->id() == terrain::Id::door)
                {
                        blocked.at(i) = false;
                }
        }

        std::vector<P> tree_pos_bucket;

        for (int x = m_r.p0.x; x <= m_r.p1.x; ++x)
        {
                for (int y = m_r.p0.y; y <= m_r.p1.y; ++y)
                {
                        if (!blocked.at(x, y) &&
                            map::g_room_map.at(x, y) == this)
                        {
                                const P p(x, y);

                                tree_pos_bucket.push_back(p);

                                if (rnd::one_in(10))
                                {
                                        map::put(new terrain::Bush(p));
                                }
                                else
                                {
                                        map::put(new terrain::Grass(p));
                                }
                        }
                }
        }

        rnd::shuffle(tree_pos_bucket);

        int nr_trees_placed = 0;

        const int tree_one_in_n = rnd::range(3, 10);

        while (!tree_pos_bucket.empty())
        {
                const P p = tree_pos_bucket.back();

                tree_pos_bucket.pop_back();

                if (rnd::one_in(tree_one_in_n))
                {
                        blocked.at(p) = true;

                        if (map_parsers::is_map_connected(blocked))
                        {
                                map::put(new terrain::Tree(p));

                                ++nr_trees_placed;
                        }
                        else
                        {
                                blocked.at(p) = false;
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// Chasm room
// -----------------------------------------------------------------------------
std::vector<RoomAutoTerrainRule> ChasmRoom::auto_terrains_allowed() const
{
        return {};
}

bool ChasmRoom::is_allowed() const
{
        return
                m_r.min_dim() >= 5 &&
                m_r.max_dim() <= 9;
}

void ChasmRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        mapgen::cavify_room(*this);
}

void ChasmRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::no)
                .run(blocked, blocked.rect());

        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                if (map::g_room_map.at(i) != this)
                {
                        blocked.at(i) = true;
                }
        }

        blocked = map_parsers::expand(blocked, blocked.rect());

        P origin;

        for (int x = m_r.p0.x; x <= m_r.p1.x; ++x)
        {
                for (int y = m_r.p0.y; y <= m_r.p1.y; ++y)
                {
                        if (!blocked.at(x, y))
                        {
                                origin.set(x, y);
                        }
                }
        }

        const auto flood = floodfill(
                origin,
                blocked,
                10000,
                P(-1, -1),
                false);

        for (int x = m_r.p0.x; x <= m_r.p1.x; ++x)
        {
                for (int y = m_r.p0.y; y <= m_r.p1.y; ++y)
                {
                        const P p(x, y);

                        if (p == origin || flood.at(x, y) != 0)
                        {
                                map::put(new terrain::Chasm(p));
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// River room
// -----------------------------------------------------------------------------
void RiverRoom::on_pre_connect(Array2<bool>& door_proposals)
{
        TRACE_FUNC_BEGIN;

        // Strategy: Expand the the river on both sides until parallel to the
        // closest center cell of another room

        const bool is_hor = m_axis == Axis::hor;

        TRACE << "Finding room centers" << std::endl;
        Array2<bool> centers(map::dims());

        for (Room* const room : map::g_room_list)
        {
                if (room != this)
                {
                        const P c_pos(room->m_r.center());

                        centers.at(c_pos) = true;
                }
        }

        TRACE <<
                "Finding closest room center coordinates on both sides "
                "(y coordinate if horizontal river, x if vertical)"
              << std::endl;

        int closest_center0 = -1;
        int closest_center1 = -1;

        // Using nestled scope to avoid declaring x and y at function scope
        {
                int x, y;

                // i_outer and i_inner should be references to x or y.
                auto find_closest_center0 =
                        [&](const Range & r_outer,
                            const Range & r_inner,
                            int& i_outer,
                            int& i_inner) {

                        for (i_outer = r_outer.min;
                             i_outer >= r_outer.max;
                             --i_outer)
                        {
                                for (i_inner = r_inner.min;
                                     i_inner <= r_inner.max;
                                     ++i_inner)
                                {
                                        if (centers.at(x, y))
                                        {
                                                closest_center0 = i_outer;
                                                break;
                                        }
                                }

                                if (closest_center0 != -1)
                                {
                                        break;
                                }
                        }
                };

                auto find_closest_center1 =
                        [&](const Range & r_outer,
                            const Range & r_inner,
                            int& i_outer,
                            int& i_inner) {

                        for (i_outer = r_outer.min;
                             i_outer <= r_outer.max;
                             ++i_outer)
                        {
                                for (i_inner = r_inner.min;
                                     i_inner <= r_inner.max;
                                     ++i_inner)
                                {
                                        if (centers.at(x, y))
                                        {
                                                closest_center1 = i_outer;
                                                break;
                                        }
                                }

                                if (closest_center1 != -1) {break;}
                        }
                };

                if (is_hor)
                {
                        const int river_y = m_r.p0.y;

                        find_closest_center0(
                                Range(river_y - 1, 1),
                                Range(1, map::w() - 2), y, x);

                        find_closest_center1(
                                Range(river_y + 1, map::h() - 2),
                                Range(1, map::w() - 2), y, x);
                }
                else // Vertical
                {
                        const int river_x = m_r.p0.x;

                        find_closest_center0(
                                Range(river_x - 1, 1),
                                Range(1, map::h() - 2), x, y);

                        find_closest_center1(
                                Range(river_x + 1, map::w() - 2),
                                Range(1, map::h() - 2), x, y);
                }
        }

        TRACE << "Expanding and filling river" << std::endl;

        Array2<bool> blocked(map::dims());

        // Within the expansion limits, mark all cells not belonging to another
        // room as free. All other cells are considered as blocking.
        for (int x = 0; x < map::w(); ++x)
        {
                for (int y = 0; y < map::h(); ++y)
                {
                        blocked.at(x, y) = true;

                        if ((m_axis == Axis::hor &&
                             (y >= closest_center0 && y <= closest_center1)) ||
                            (m_axis == Axis::ver &&
                             (x >= closest_center0 && x <= closest_center1)))
                        {
                                Room* r = map::g_room_map.at(x, y);

                                blocked.at(x, y) = r && r != this;
                        }
                }
        }

        blocked = map_parsers::expand(blocked, blocked.rect());

        const P origin(m_r.center());

        const auto flood = floodfill(
                origin,
                blocked,
                INT_MAX,
                P(-1, -1),
                true);

        for (int x = 0; x < map::w(); ++x)
        {
                for (int y = 0; y < map::h(); ++y)
                {
                        const P p(x, y);

                        if (flood.at(x, y) > 0 || p == origin)
                        {
                                map::put(new terrain::Chasm(p));

                                map::g_room_map.at(x, y) = this;

                                m_r.p0.x = std::min(m_r.p0.x, x);
                                m_r.p0.y = std::min(m_r.p0.y, y);
                                m_r.p1.x = std::max(m_r.p1.x, x);
                                m_r.p1.y = std::max(m_r.p1.y, y);
                        }
                }
        }

        TRACE << "Making bridge(s)" << std::endl;

        // Mark which side each cell belongs to
        enum Side
        {
                in_river,
                side0,
                side1
        };

        Array2<Side> sides(map::dims());

        // Scoping to avoid declaring x and y at function scope
        {
                int x, y;

                // i_outer and i_inner should be references to x or y.
                auto mark_sides =
                        [&](const Range & r_outer,
                            const Range & r_inner,
                            int& i_outer,
                            int& i_inner) {

                        for (i_outer = r_outer.min;
                             i_outer <= r_outer.max;
                             ++i_outer)
                        {
                                bool is_on_side0 = true;

                                for (i_inner = r_inner.min;
                                     i_inner <= r_inner.max;
                                     ++i_inner)
                                {
                                        if (map::g_room_map.at(x, y) == this)
                                        {
                                                is_on_side0 = false;
                                                sides.at(x, y) = in_river;
                                        }
                                        else
                                        {
                                                sides.at(x, y) =
                                                        is_on_side0
                                                        ? side0
                                                        : side1;
                                        }
                                }
                        }
                };

                if (m_axis == Axis::hor)
                {
                        mark_sides(Range(1, map::w() - 2),
                                   Range(1, map::h() - 2), x, y);
                }
                else
                {
                        mark_sides(Range(1, map::h() - 2),
                                   Range(1, map::w() - 2), y, x);
                }
        }

        Array2<bool> valid_room_entries0(map::dims());
        Array2<bool> valid_room_entries1(map::dims());

        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                valid_room_entries0.at(i) = valid_room_entries1.at(i) = false;
        }

        const int edge_d = 4;

        for (int x = edge_d; x < map::w() - edge_d; ++x)
        {
                for (int y = edge_d; y < map::h() - edge_d; ++y)
                {
                        const auto terrain_id =
                                map::g_cells.at(x, y).terrain->id();

                        if ((terrain_id != terrain::Id::wall) ||
                            map::g_room_map.at(x, y))
                        {
                                continue;
                        }

                        const P p(x, y);
                        int nr_cardinal_floor = 0;
                        int nr_cardinal_river = 0;

                        for (const auto& d : dir_utils::g_cardinal_list)
                        {
                                const auto p_adj(p + d);

                                const auto* const t =
                                        map::g_cells.at(p_adj).terrain;

                                if (t->id() == terrain::Id::floor)
                                {
                                        nr_cardinal_floor++;
                                }

                                if (map::g_room_map.at(p_adj) == this)
                                {
                                        nr_cardinal_river++;
                                }
                        }

                        if (nr_cardinal_floor == 1 &&
                            nr_cardinal_river == 1)
                        {
                                switch (sides.at(x, y))
                                {
                                case side0:
                                        valid_room_entries0.at(x, y) = true;
                                        break;

                                case side1:
                                        valid_room_entries1.at(x, y) = true;
                                        break;

                                case in_river:
                                        break;
                                }
                        }
                }
        }

#ifndef NDEBUG
        if (init::g_is_demo_mapgen)
        {
                states::draw();

                for (int y = 1; y < map::h() - 1; ++y)
                {
                        for (int x = 1; x < map::w() - 1; ++x)
                        {
                                P p(x, y);

                                if (valid_room_entries0.at(x, y))
                                {
                                        io::draw_character(
                                                '0',
                                                Panel::map,
                                                p,
                                                colors::light_red());
                                }

                                if (valid_room_entries1.at(x, y))
                                {
                                        io::draw_character(
                                                '1',
                                                Panel::map,
                                                p,
                                                colors::light_red());
                                }

                                if (valid_room_entries0.at(x, y) ||
                                    valid_room_entries1.at(x, y))
                                {
                                        io::update_screen();
                                        sdl_base::sleep(100);
                                }
                        }
                }
        }
#endif // NDEBUG

        std::vector<int> positions(
                is_hor
                ? map::w()
                : map::h());

        std::iota(std::begin(positions), std::end(positions), 0);

        rnd::shuffle(positions);

        std::vector<int> c_built;

        const int min_edge_dist = 6;

        const int max_nr_bridges = rnd::range(1, 3);

        for (const int bridge_n : positions)
        {
                if ((bridge_n < min_edge_dist) ||
                    (is_hor && (bridge_n > (map::w() - 1 - min_edge_dist))) ||
                    (!is_hor && (bridge_n > (map::h() - 1 - min_edge_dist))))
                {
                        continue;
                }

                bool is_too_close_to_other_bridge = false;

                const int min_d = 2;

                for (int c_other : c_built)
                {
                        const bool is_in_range = Range(
                                c_other - min_d,
                                c_other + min_d)
                                .is_in_range(bridge_n);

                        if (is_in_range)
                        {
                                is_too_close_to_other_bridge = true;
                                break;
                        }
                }

                if (is_too_close_to_other_bridge)
                {
                        continue;
                }

                // Check if current bridge coord would connect matching room
                // connections. If so both room_con0 and room_con1 will be set.
                P room_con0(-1, -1);
                P room_con1(-1, -1);

                const int c0_0 = is_hor ? m_r.p1.y : m_r.p1.x;
                const int c1_0 = is_hor ? m_r.p0.y : m_r.p0.x;

                for (int c = c0_0; c != c1_0; --c)
                {
                        if ((is_hor && sides.at(bridge_n, c) == side0) ||
                            (!is_hor && sides.at(c, bridge_n) == side0))
                        {
                                break;
                        }

                        const P p_nxt =
                                is_hor ?
                                P(bridge_n, c - 1) :
                                P(c - 1, bridge_n);

                        if (valid_room_entries0.at(p_nxt))
                        {
                                room_con0 = p_nxt;
                                break;
                        }
                }

                const int c0_1 = is_hor ? m_r.p0.y : m_r.p0.x;
                const int c1_1 = is_hor ? m_r.p1.y : m_r.p1.x;

                for (int c = c0_1; c != c1_1; ++c)
                {
                        if ((is_hor && sides.at(bridge_n, c) == side1) ||
                            (!is_hor && sides.at(c, bridge_n) == side1))
                        {
                                break;
                        }

                        const P p_nxt =
                                is_hor ?
                                P(bridge_n, c + 1) :
                                P(c + 1, bridge_n);

                        if (valid_room_entries1.at(p_nxt))
                        {
                                room_con1 = p_nxt;
                                break;
                        }
                }

                // Make the bridge if valid connection pairs found
                if (room_con0.x != -1 && room_con1.x != -1)
                {
#ifndef NDEBUG
                        if (init::g_is_demo_mapgen)
                        {
                                states::draw();

                                io::draw_character('0',
                                                   Panel::map,
                                                   room_con0,
                                                   colors::light_green());

                                io::draw_character('1',
                                                   Panel::map,
                                                   room_con1,
                                                   colors::yellow());

                                io::update_screen();

                                sdl_base::sleep(2000);
                        }
#endif // NDEBUG

                        TRACE << "Found valid connection pair at: "
                              << room_con0.x << "," << room_con0.y << " / "
                              << room_con1.x << "," << room_con1.y << std::endl
                              << "Making bridge at pos: " << bridge_n
                              << std::endl;

                        if (is_hor)
                        {
                                for (int y = room_con0.y; y <= room_con1.y; ++y)
                                {
                                        if (map::g_room_map.at(bridge_n, y) ==
                                            this)
                                        {
                                                auto* const floor =
                                                        new terrain::Floor(
                                                                {bridge_n, y});

                                                floor->m_type =
                                                        terrain::FloorType::common;

                                                map::put(floor);
                                        }
                                }
                        }
                        else // Vertical
                        {
                                for (int x = room_con0.x; x <= room_con1.x; ++x)
                                {
                                        if (map::g_room_map.at(x, bridge_n) ==
                                            this)
                                        {
                                                auto* const floor =
                                                        new terrain::Floor(
                                                                {x, bridge_n});

                                                floor->m_type =
                                                        terrain::FloorType::common;

                                                map::put(floor);
                                        }
                                }
                        }

                        map::put(new terrain::Floor(room_con0));
                        map::put(new terrain::Floor(room_con1));

                        door_proposals.at(room_con0) = true;
                        door_proposals.at(room_con1) = true;

                        c_built.push_back(bridge_n);
                }

                if (int (c_built.size()) >= max_nr_bridges)
                {
                        TRACE << "Enough bridges built" << std::endl;
                        break;
                }
        }

        TRACE << "Bridges built/attempted: "
              << c_built.size() << "/"
              << max_nr_bridges << std::endl;

        if (c_built.empty())
        {
                mapgen::g_is_map_valid = false;
        }
        else // Map is valid (at least one bridge was built)
        {
                Array2<bool> valid_room_entries(map::dims());

                for (int x = 0; x < map::w(); ++x)
                {
                        for (int y = 0; y < map::h(); ++y)
                        {
                                valid_room_entries.at(x, y) =
                                        valid_room_entries0.at(x, y) ||
                                        valid_room_entries1.at(x, y);

                                // Convert some remaining valid room entries
                                if (valid_room_entries.at(x, y) &&
                                    (std::find(
                                            std::begin(c_built),
                                            std::end(c_built),
                                            x) ==
                                     std::end(c_built)))
                                {
                                        map::put(new terrain::Floor(P(x, y)));

                                        map::g_room_map.at(x, y) = this;
                                }
                        }
                }

                // Convert wall cells adjacent to river cells to river
                valid_room_entries = map_parsers::expand(valid_room_entries, 2);

                for (int x = 2; x < map::w() - 2; ++x)
                {
                        for (int y = 2; y < map::h() - 2; ++y)
                        {
                                if (valid_room_entries.at(x, y) &&
                                    map::g_room_map.at(x, y) == this)
                                {
                                        auto* const floor = new terrain::Floor(P(x, y));

                                        floor->m_type = terrain::FloorType::common;

                                        map::put(floor);

                                        map::g_room_map.at(x, y) = nullptr;
                                }
                        }
                }
        }

        TRACE_FUNC_END;

} // RiverRoom::on_pre_connect
