// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "map.hpp"

#include <climits>

#include "actor_factory.hpp"
#include "actor_player.hpp"
#include "debug.hpp"
#include "terrain_mob.hpp"
#include "terrain.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_factory.hpp"
#include "mapgen.hpp"
#include "misc.hpp"
#include "pos.hpp"
#include "random.hpp"
#include "saving.hpp"

#ifndef NDEBUG
#include "sdl_base.hpp"
#include "viewport.hpp"
#endif // NDEBUG

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static P s_dims(0, 0);

// -----------------------------------------------------------------------------
// Cell
// -----------------------------------------------------------------------------
Cell::Cell() :
        is_explored(false),
        is_seen_by_player(false),
        player_los(),
        item(nullptr),
        terrain(nullptr) {}

Cell::~Cell()
{
        delete terrain;
        delete item;
}

void Cell::reset()
{
        is_explored = false;

        is_seen_by_player = false;

        // Init player los as if all cells have walls
        player_los.is_blocked_hard = true;

        player_los.is_blocked_by_dark = false;

        delete terrain;
        terrain = nullptr;

        delete item;
        item = nullptr;
}

// -----------------------------------------------------------------------------
// map
// -----------------------------------------------------------------------------
namespace map
{

actor::Player* g_player = nullptr;

int g_dlvl = 0;

Color g_wall_color;

Array2<Cell> g_cells(0, 0);

Array2<bool> g_light(0, 0);
Array2<bool> g_dark(0, 0);

Array2<Color> g_minimap(0, 0);

std::vector<Room*> g_room_list;

Array2<Room*> g_room_map(0, 0);

std::vector<ChokePointData> g_choke_point_data;

void init()
{
        g_dlvl = 0;

        g_room_list.clear();

        actor::Actor* actor = actor::make(actor::Id::player, {0, 0});

        g_player = static_cast<actor::Player*>(actor);
}

void cleanup()
{
        reset(P(0, 0));

        // NOTE: The player object is deleted elsewhere
        g_player = nullptr;
}

void save()
{
        saving::put_int(g_dlvl);
}

void load()
{
        g_dlvl = saving::get_int();
}

void reset(const P& dims)
{
        actor::delete_all_mon();

        game_time::erase_all_mobs();

        game_time::reset_turn_type_and_actor_counters();

        for (auto* room : g_room_list)
        {
                delete room;
        }

        g_room_list.clear();

        g_choke_point_data.clear();

        for (auto& cell : g_cells)
        {
                cell.reset();
        }

        s_dims = dims;

        g_cells.resize_no_init(dims);

        for (auto& cell : g_cells)
        {
                cell.reset();
        }

        g_light.resize(s_dims);
        g_dark.resize(s_dims);
        g_room_map.resize(s_dims);

        for (int x = 0; x < w(); ++x)
        {
                for (int y = 0; y < h(); ++y)
                {
                        put(new terrain::Wall(P(x, y)));
                }
        }

        // Occasionally set wall color to something unusual
        if (rnd::one_in(3))
        {
                std::vector<Color> wall_color_bucket = {
                        colors::red(),
                        colors::sepia(),
                        colors::dark_sepia(),
                        colors::dark_brown(),
                        colors::gray_brown(),
                };

                g_wall_color = rnd::element(wall_color_bucket);
        }
        else // Standard wall color
        {
                g_wall_color = colors::gray();
        }
}

int w()
{
        return s_dims.x;
}

int h()
{
        return s_dims.y;
}

P dims()
{
        return s_dims;
}

R rect()
{
        return R({0, 0}, s_dims - 1);
}

size_t nr_cells()
{
        return g_cells.length();
}

terrain::Terrain* put(terrain::Terrain* const t)
{
        ASSERT(t);

        const P p = t->pos();

        Cell& cell = g_cells.at(p);

        delete cell.terrain;

        cell.terrain = t;

#ifndef NDEBUG
        if (init::g_is_demo_mapgen)
        {
                if (t->id() == terrain::Id::floor)
                {
                        if (!viewport::is_in_view(p))
                        {
                                viewport::focus_on(p);
                        }

                        for (auto& shown_cell : g_cells)
                        {
                                shown_cell.is_seen_by_player = true;
                                shown_cell.is_explored = true;
                        }

                        states::draw();

                        io::draw_symbol(
                                TileId::aim_marker_line,
                                'X',
                                Panel::map,
                                viewport::to_view_pos(p),
                                colors::yellow());

                        io::update_screen();

                        // NOTE: Delay must be > 1 for user input to be read
                        sdl_base::sleep(3);
                }
        }
#endif // NDEBUG

        t->on_placed();

        return t;
}

void update_vision()
{
        game_time::update_light_map();

        map::g_player->update_fov();

        map::g_player->update_mon_awareness();

        states::draw();
}

void make_blood(const P& origin)
{
        for (int dx = -1; dx <= 1; ++dx)
        {
                for (int dy = -1; dy <= 1; ++dy)
                {
                        const P c = origin + P(dx, dy);

                        auto* const t = g_cells.at(c).terrain;

                        if (t->can_have_blood())
                        {
                                if (rnd::one_in(3))
                                {
                                        t->make_bloody();
                                }
                        }
                }
        }
}

void make_gore(const P& origin)
{
        for (int dx = -1; dx <= 1; ++dx)
        {
                for (int dy = -1; dy <= 1; ++dy)
                {
                        const P c = origin + P(dx, dy);

                        if (rnd::one_in(3))
                        {
                                g_cells.at(c).terrain->try_put_gore();
                        }
                }
        }
}

void delete_and_remove_room_from_list(Room* const room)
{
        for (size_t i = 0; i < g_room_list.size(); ++i)
        {
                if (g_room_list[i] == room)
                {
                        delete room;
                        g_room_list.erase(g_room_list.begin() + i);
                        return;
                }
        }

        ASSERT(false && "Tried to remove non-existing room");
}

bool is_pos_seen_by_player(const P& p)
{
        ASSERT(is_pos_inside_map(p));

        return g_cells.at(p).is_seen_by_player;
}

actor::Actor* first_actor_at_pos(const P& pos, ActorState state)
{
        for (auto* const actor : game_time::g_actors)
        {
                if ((actor->m_pos == pos) && (actor->m_state == state))
                {
                        return actor;
                }
        }

        return nullptr;
}

terrain::Terrain* first_mob_at_pos(const P& pos)
{
        for (auto* const mob : game_time::g_mobs)
        {
                if (mob->pos() == pos)
                {
                        return mob;
                }
        }

        return nullptr;
}

void actor_cells(
        const std::vector<actor::Actor*>& actors,
        std::vector<P>& out)
{
        out.clear();

        for (const auto* const a : actors)
        {
                out.push_back(a->m_pos);
        }
}

Array2<std::vector<actor::Actor*>> get_actor_array()
{
        Array2<std::vector<actor::Actor*>> a(dims());

        for (auto* actor : game_time::g_actors)
        {
                const P& p = actor->m_pos;

                a.at(p).push_back(actor);
        }

        return a;
}

actor::Actor* random_closest_actor(
        const P& c,
        const std::vector<actor::Actor*>& actors)
{
        if (actors.empty())
        {
                return nullptr;
        }

        if (actors.size() == 1)
        {
                return actors[0];
        }

        // Find distance to nearest actor(s)
        int dist_to_nearest = INT_MAX;

        for (auto* actor : actors)
        {
                const int current_dist = king_dist(c, actor->m_pos);

                if (current_dist < dist_to_nearest)
                {
                        dist_to_nearest = current_dist;
                }
        }

        ASSERT(dist_to_nearest != INT_MAX);

        // Store all actors with distance equal to the nearest distance
        std::vector<actor::Actor*> closest_actors;

        for (auto* actor : actors)
        {
                if (king_dist(c, actor->m_pos) == dist_to_nearest)
                {
                        closest_actors.push_back(actor);
                }
        }

        ASSERT(!closest_actors.empty());

        const int element = rnd::range(0, closest_actors.size() - 1);

        return closest_actors[element];
}

bool is_pos_inside_map(const P& pos)
{
        return
                (pos.x >= 0) &&
                (pos.y >= 0) &&
                (pos.x < w()) &&
                (pos.y < h());
}

bool is_pos_inside_outer_walls(const P& pos)
{
        return
                (pos.x > 0) &&
                (pos.y > 0) &&
                (pos.x < (w() - 1)) &&
                (pos.y < (h() - 1));
}

bool is_area_inside_map(const R& area)
{
        return
                is_pos_inside_map(area.p0) &&
                is_pos_inside_map(area.p1);
}

} // map
