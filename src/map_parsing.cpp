// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "map_parsing.hpp"

#include <algorithm>
#include <climits>

#include "actor_player.hpp"
#include "feature_mob.hpp"
#include "feature_rigid.hpp"
#include "flood.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "map.hpp"
#include "misc.hpp"

#ifndef NDEBUG
#include "io.hpp"
#include "sdl_base.hpp"
#include "viewport.hpp"
#endif // NDEBUG

namespace map_parsers
{

// -----------------------------------------------------------------------------
// Base class
// -----------------------------------------------------------------------------
void MapParser::run(Array2<bool>& out,
                    const R& area_to_parse_cells,
                    const MapParseMode write_rule)
{
        ASSERT(m_parse_cells == ParseCells::yes ||
               m_parse_mobs == ParseMobs::yes ||
               m_parse_actors == ParseActors::yes);

        const bool allow_write_false =
                write_rule == MapParseMode::overwrite;

        if (m_parse_cells == ParseCells::yes)
        {
                for (int x = area_to_parse_cells.p0.x;
                     x <= area_to_parse_cells.p1.x;
                     ++x)
                {
                        for (int y = area_to_parse_cells.p0.y;
                             y <= area_to_parse_cells.p1.y;
                             ++y)
                        {
                                const auto& c = map::g_cells.at(x, y);

                                const bool is_match = parse(c, P(x, y));

                                if (is_match || allow_write_false)
                                {
                                        out.at(x, y) = is_match;
                                }
                        }
                }
        }

        if (m_parse_mobs == ParseMobs::yes)
        {
                for (Mob* mob : game_time::g_mobs)
                {
                        const P& p = mob->pos();

                        if (area_to_parse_cells.is_pos_inside(p))
                        {
                                const bool is_match = parse(*mob);

                                if (is_match || allow_write_false)
                                {
                                        bool& v = out.at(p);

                                        if (!v)
                                        {
                                                v = is_match;
                                        }
                                }
                        }
                }
        }

        if (m_parse_actors == ParseActors::yes)
        {
                for (auto* actor : game_time::g_actors)
                {
                        const P& p = actor->m_pos;

                        if (area_to_parse_cells.is_pos_inside(p))
                        {
                                const bool is_match = parse(*actor);

                                if (is_match || allow_write_false)
                                {
                                        bool& v = out.at(p);

                                        if (!v)
                                        {
                                                v = is_match;
                                        }
                                }
                        }
                }
        }

} // run

bool MapParser::cell(const P& pos) const
{
        ASSERT(m_parse_cells == ParseCells::yes ||
               m_parse_mobs == ParseMobs::yes ||
               m_parse_actors == ParseActors::yes);

        bool r = false;

        if (m_parse_cells == ParseCells::yes)
        {
                const auto& c = map::g_cells.at(pos);

                const bool is_match = parse(c, pos);

                if (is_match)
                {
                        r = true;
                }
        }

        if (m_parse_mobs == ParseMobs::yes)
        {
                for (Mob* mob : game_time::g_mobs)
                {
                        const P& mob_p = mob->pos();

                        if (mob_p == pos)
                        {
                                const bool is_match = parse(*mob);

                                if (is_match)
                                {
                                        r = true;
                                        break;
                                }
                        }
                }
        }

        if (m_parse_actors == ParseActors::yes)
        {
                for (auto* actor : game_time::g_actors)
                {
                        const P& actor_pos = actor->m_pos;

                        if (actor_pos == pos)
                        {
                                const bool is_match = parse(*actor);

                                if (is_match)
                                {
                                        r = true;
                                        break;
                                }
                        }
                }
        }

        return r;

} // cell


// -----------------------------------------------------------------------------
// Map parsers
// -----------------------------------------------------------------------------
bool BlocksLos::parse(const Cell& c, const P& pos) const
{
        return
                !map::is_pos_inside_outer_walls(pos) ||
                !c.rigid->is_los_passable();
}

bool BlocksLos::parse(const Mob& f) const
{
        return !f.is_los_passable();
}

bool BlocksWalking::parse(const Cell& c, const P& pos) const
{
        return
                !map::is_pos_inside_outer_walls(pos) ||
                !c.rigid->is_walkable();
}

bool BlocksWalking::parse(const Mob& f) const
{
        return !f.is_walkable();
}

bool BlocksWalking::parse(const actor::Actor& a) const
{
        return a.is_alive();
}

bool BlocksActor::parse(const Cell& c, const P& pos) const
{
        return
                !map::is_pos_inside_outer_walls(pos) ||
                !c.rigid->can_move(m_actor);
}

bool BlocksActor::parse(const Mob& f) const
{
        return !f.can_move(m_actor);
}

bool BlocksActor::parse(const actor::Actor& a) const
{
        return a.is_alive();
}

bool BlocksProjectiles::parse(const Cell& c, const P& pos) const
{
        return
                !map::is_pos_inside_outer_walls(pos) ||
                !c.rigid->is_projectile_passable();
}

bool BlocksProjectiles::parse(const Mob& f) const
{
        return !f.is_projectile_passable();
}

bool BlocksSound::parse(const Cell& c, const P& pos) const
{
        return
                !map::is_pos_inside_outer_walls(pos) ||
                !c.rigid->is_sound_passable();
}

bool BlocksSound::parse(const Mob& f) const
{
        return !f.is_sound_passable();
}

bool LivingActorsAdjToPos::parse(const actor::Actor& a) const
{
        if (!a.is_alive())
        {
                return false;
        }

        return is_pos_adj(m_pos, a.m_pos, true);
}

bool BlocksItems::parse(const Cell& c, const P& pos) const
{
        return
                !map::is_pos_inside_outer_walls(pos) ||
                !c.rigid->can_have_item();
}

bool BlocksItems::parse(const Mob& f) const
{
        return !f.can_have_item();
}

bool BlocksRigid::parse(const Cell& c, const P& pos) const
{
        return
                !map::is_pos_inside_outer_walls(pos) ||
                !c.rigid->can_have_rigid();
}

bool IsNotFeature::parse(const Cell& c, const P& pos) const
{
        (void)pos;

        return c.rigid->id() != m_feature;
}

bool IsAnyOfFeatures::parse(const Cell& c, const P& pos) const
{
        (void)pos;

        for (auto f : m_features)
        {
                if (f == c.rigid->id())
                {
                        return true;
                }
        }

        return false;
}

bool AnyAdjIsAnyOfFeatures::parse(const Cell& c, const P& pos) const
{
        (void)c;

        if (!map::is_pos_inside_outer_walls(pos))
        {
                return false;
        }

        for (const auto& d : dir_utils::g_dir_list_w_center)
        {
                const auto id_here = map::g_cells.at(pos + d).rigid->id();

                const auto search_result =
                        std::find(
                                std::begin(m_features),
                                std::end(m_features),
                                id_here);

                if (search_result != std::end(m_features))
                {
                        return true;
                }
        }

        return false;
}

bool AllAdjIsFeature::parse(const Cell& c, const P& pos) const
{
        (void)c;

        if (!map::is_pos_inside_outer_walls(pos))
        {
                return false;
        }

        for (const auto& d : dir_utils::g_dir_list_w_center)
        {
                if (map::g_cells.at(pos + d).rigid->id() != m_feature)
                {
                        return false;
                }
        }

        return true;
}

bool AllAdjIsAnyOfFeatures::parse(const Cell& c, const P& pos) const
{
        (void)c;

        if (!map::is_pos_inside_outer_walls(pos))
        {
                return false;
        }

        for (const auto& d : dir_utils::g_dir_list_w_center)
        {
                const auto current_id = map::g_cells.at(pos + d).rigid->id();

                bool is_match = false;

                for (auto f : m_features)
                {
                        if (f == current_id)
                        {
                                is_match = true;

                                break;
                        }
                }

                if (!is_match)
                {
                        return false;
                }
        }

        return true;
}

bool AllAdjIsNotFeature::parse(const Cell& c, const P& pos) const
{
        (void)c;

        if (pos.x <= 0 ||
            pos.x >= map::w() - 1 ||
            pos.y <= 0 ||
            pos.y >= map::h() - 1)
        {
                return false;
        }

        for (const auto& d : dir_utils::g_dir_list_w_center)
        {
                if (map::g_cells.at(pos + d).rigid->id() == m_feature)
                {
                        return false;
                }
        }

        return true;
}

bool AllAdjIsNoneOfFeatures::parse(const Cell& c, const P& pos) const
{
        (void)c;

        if (pos.x <= 0 ||
            pos.x >= map::w() - 1 ||
            pos.y <= 0 ||
            pos.y >= map::h() - 1)
        {
                return false;
        }

        for (const auto& d : dir_utils::g_dir_list_w_center)
        {
                const auto current_id = map::g_cells.at(pos + d).rigid->id();

                for (auto f : m_features)
                {
                        if (f == current_id)
                        {
                                return false;
                        }
                }
        }

        return true;
}

// -----------------------------------------------------------------------------
// Various utility algorithms
// -----------------------------------------------------------------------------
Array2<bool> cells_within_dist_of_others(
        const Array2<bool>& in,
        const Range& dist_interval)
{
        const P dims = in.dims();

        Array2<bool> result(dims);

        for (int x_outer = 0; x_outer < dims.x; x_outer++)
        {
                for (int y_outer = 0; y_outer < dims.y; y_outer++)
                {
                        if (result.at(x_outer, y_outer))
                        {
                                continue;
                        }

                        for (int d = dist_interval.min;
                             d <= dist_interval.max;
                             d++)
                        {
                                P p0(std::max(0, x_outer - d),
                                     std::max(0, y_outer - d));

                                P p1(std::min(dims.x - 1, x_outer + d),
                                     std::min(dims.y - 1, y_outer + d));

                                for (int x = p0.x; x <= p1.x; ++x)
                                {
                                        if (!in.at(x, p0.y) && !in.at(x, p1.y))
                                        {
                                                continue;
                                        }

                                        result.at(x_outer, y_outer) = true;
                                        break;
                                }

                                for (int y = p0.y; y <= p1.y; ++y)
                                {
                                        if (!in.at(p0.x, y) && !in.at(p1.x, y))
                                        {
                                                continue;
                                        }

                                        result.at(x_outer, y_outer) = true;
                                        break;
                                }
                        } // distance loop
                } // outer y loop
        } // outer x loop

        return result;

} // cells_within_dist_of_others


void append(Array2<bool>& base, const Array2<bool>& append)
{
        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                if (append.at(i))
                {
                        base.at(i) = true;
                }
        }
}

Array2<bool> expand(const Array2<bool>& in,
                    const R& area_allowed_to_modify)
{
        const P dims = in.dims();

        Array2<bool> result(dims);

        const int x0 = std::max(
                0,
                area_allowed_to_modify.p0.x);

        const int y0 = std::max(
                0,
                area_allowed_to_modify.p0.y);

        const int x1 = std::min(
                dims.x - 1,
                area_allowed_to_modify.p1.x);

        const int y1 = std::min(
                dims.y - 1,
                area_allowed_to_modify.p1.y);

        for (int x = x0; x <= x1; ++x)
        {
                for (int y = y0; y <= y1; ++y)
                {
                        result.at(x, y) = false;

                        // Search all cells adjacent to the current position for
                        // any cell which is "true" in the input arry.
                        const int cmp_x0 = std::max(x - 1, 0);
                        const int cmp_y0 = std::max(y - 1, 0);
                        const int cmp_x1 = std::min(x + 1, dims.x - 1);
                        const int cmp_y1 = std::min(y + 1, dims.y - 1);

                        for (int cmp_x = cmp_x0;
                             cmp_x <= cmp_x1;
                             ++cmp_x)
                        {
                                bool is_found = false;

                                for (int cmp_y = cmp_y0;
                                     cmp_y <= cmp_y1;
                                     ++cmp_y)
                                {
                                        if (in.at(cmp_x, cmp_y))
                                        {
                                                result.at(x, y) = true;

                                                is_found = true;

                                                break;
                                        }
                                } // Compare y loop

                                if (is_found)
                                {
                                        break;
                                }
                        } // Compare x loop
                } // y loop
        } // x loop

        return result;

} // expand


Array2<bool> expand(const Array2<bool>& in,
                    const int dist)
{
        const P dims = in.dims();

        Array2<bool> result(dims);

        for (int x = 0; x < dims.x; ++x)
        {
                for (int y = 0; y < dims.y; ++y)
                {
                        result.at(x, y) = false;

                        const int x0 = x - dist;
                        const int y0 = y - dist;
                        const int x1 = x + dist;
                        const int y1 = y + dist;

                        const int cmp_x0 = x0 < 0 ? 0 : x0;
                        const int cmp_y0 = y0 < 0 ? 0 : y0;
                        const int cmp_x1 = x1 > dims.x - 1 ? dims.x - 1 : x1;
                        const int cmp_y1 = y1 > dims.y - 1 ? dims.y - 1 : y1;

                        for (int cmp_y = cmp_y0;
                             cmp_y <= cmp_y1;
                             ++cmp_y)
                        {
                                bool is_found = false;

                                for (int cmp_x = cmp_x0;
                                     cmp_x <= cmp_x1;
                                     ++cmp_x)
                                {
                                        if (!in.at(cmp_x, cmp_y))
                                        {
                                                continue;
                                        }

                                        is_found = result.at(x, y) = true;

                                        break;
                                }

                                if (is_found)
                                {
                                        break;
                                }
                        }
                }
        }

        return result;

} // expand


bool is_map_connected(const Array2<bool>& blocked)
{
        P origin(-1, -1);

        const P dims = blocked.dims();

        for (int x = 1; x < dims.x - 1; ++x)
        {
                for (int y = 1; y < dims.y - 1; ++y)
                {
                        if (!blocked.at(x, y))
                        {
                                origin.set(x, y);
                                break;
                        }
                }

                if (origin.x != -1)
                {
                        break;
                }
        }

        ASSERT(map::is_pos_inside_outer_walls(origin));

        const auto flood = floodfill(
                origin,
                blocked,
                INT_MAX,
                P(-1, -1),
                true);

        // NOTE: We can skip to origin.x immediately, since this is guaranteed
        // to be the leftmost non-blocked cell.
        for (int x = origin.x; x < dims.x - 1; ++x)
        {
                for (int y = 1; y < dims.y - 1; ++y)
                {
                        if (flood.at(x, y) == 0 &&
                            !blocked.at(x, y) &&
                            P(x, y) != origin)
                        {
#ifndef NDEBUG
                                if (init::g_is_demo_mapgen)
                                {
                                        if (!viewport::is_in_view(P(x, y)))
                                        {
                                                viewport::focus_on(P(x, y));
                                        }

                                        states::draw();

                                        io::draw_symbol(
                                                TileId::excl_mark,
                                                'X',
                                                Panel::map,
                                                viewport::to_view_pos(P(x, y)),
                                                colors::light_red());

                                        io::update_screen();

                                        sdl_base::sleep(3);
                                }
#endif // NDEBUG

                                return false;
                        }
                }
        }

        return true;

} // is_map_connected


} // map_parsers


// -----------------------------------------------------------------------------
// Is closer to pos
// -----------------------------------------------------------------------------
bool IsCloserToPos::operator()(const P& p1, const P& p2)
{
        const int king_dist1 = king_dist(m_pos.x, m_pos.y, p1.x, p1.y);
        const int king_dist2 = king_dist(m_pos.x, m_pos.y, p2.x, p2.y);

        return king_dist1 < king_dist2;
}

// -----------------------------------------------------------------------------
// Is further from pos
// -----------------------------------------------------------------------------
bool IsFurtherFromPos::operator()(const P& p1, const P& p2)
{
        const int king_dist1 = king_dist(m_pos.x, m_pos.y, p1.x, p1.y);
        const int king_dist2 = king_dist(m_pos.x, m_pos.y, p2.x, p2.y);

        return king_dist1 > king_dist2;
}
