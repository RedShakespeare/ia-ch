// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "populate_traps.hpp"

#include <algorithm>

#include "actor_player.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "mapgen.hpp"
#include "terrain_data.hpp"
#include "terrain_trap.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static Fraction chance_for_trapped_room(const RoomType type)
{
        Fraction chance(-1, -1);

        switch (type)
        {
        case RoomType::plain:
                chance = {1, 17};
                break;

        case RoomType::human:
                chance = {1, 8};
                break;

        case RoomType::ritual:
                chance = {1, 8};
                break;

        case RoomType::spider:
                chance = {2, 3};
                break;

        case RoomType::crypt:
                chance = {1, 30};
                break;

        case RoomType::monster:
                chance = {1, 12};
                break;

        case RoomType::chasm:
                chance = {1, 8};
                break;

        case RoomType::damp:
                chance = {1, 17};
                break;

        case RoomType::pool:
                chance = {1, 17};
                break;

        case RoomType::jail:
                chance = {1, 12};
                break;

        case RoomType::corr_link:
                chance = {1, 30};
                break;

        case RoomType::snake_pit:
        case RoomType::forest:
        case RoomType::cave:
        case RoomType::END_OF_STD_ROOMS:
        case RoomType::river:
        case RoomType::crumble_room:
                break;
        }

        return chance;
}

static std::vector<P> find_allowed_cells_in_room(
        const Room& room,
        const Array2<bool>& blocked)
{
        std::vector<P> positions;

        const auto r = room.m_r;

        positions.reserve(r.area());

        for (int x = r.p0.x; x <= r.p1.x; ++x)
        {
                for (int y = r.p0.y; y <= r.p1.y; ++y)
                {
                        const P p(x, y);

                        if (!blocked.at(p) &&
                            map::g_cells.at(p).terrain->can_have_trap() &&
                            (map::g_room_map.at(p) == &room))
                        {
                                positions.push_back(p);
                        }
                }
        }

        return positions;
}

static terrain::Trap* make_trap(const terrain::TrapId id, const P& pos)
{
        const auto* const t = map::g_cells.at(pos).terrain;

        const auto& d = terrain::data(t->id());

        auto* const mimic = static_cast<terrain::Terrain*>(d.make_obj(pos));

        if (!t->can_have_trap())
        {
                TRACE << "Cannot place trap on terrain id: "
                      << (int)t->id() << std::endl
                      << "Trap id: "
                      << int(id) << std::endl;

                ASSERT(false);

                return nullptr;
        }

        auto* const trap = new terrain::Trap(pos, mimic, id);

        return trap;
}

// -----------------------------------------------------------------------------
// populate_std_lvl
// -----------------------------------------------------------------------------
namespace populate_traps
{
void populate_std_lvl()
{
        TRACE_FUNC_BEGIN;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::no)
                .run(blocked, blocked.rect());

        const P& player_p = map::g_player->m_pos;

        blocked.at(player_p) = true;

        for (Room* const room : map::g_room_list)
        {
                const Fraction chance_trapped =
                        chance_for_trapped_room(room->m_type);

                if ((chance_trapped.num == -1) || !chance_trapped.roll())
                {
                        continue;
                }

                auto trap_pos_bucket =
                        find_allowed_cells_in_room(
                                *room,
                                blocked);

                rnd::shuffle(trap_pos_bucket);

                const int nr_traps =
                        std::min(
                                rnd::range(1, 3),
                                (int)trap_pos_bucket.size());

                for (int i = 0; i < nr_traps; ++i)
                {
                        const terrain::TrapId trap_type =
                                (room->m_type == RoomType::spider)
                                ? terrain::TrapId::web
                                : terrain::TrapId::any;

                        const auto pos = trap_pos_bucket[i];

                        auto* const trap = make_trap(trap_type, pos);

                        if (!trap->valid())
                        {
                                delete trap;

                                continue;
                        }

                        map::put(trap);
                }
        }  // room loop

        TRACE_FUNC_END;
}

}  // namespace populate_traps
