// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "actor_factory.hpp"

#include <algorithm>

#include "actor.hpp"
#include "actor_data.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "feature_door.hpp"
#include "feature_rigid.hpp"
#include "init.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static Actor* make_actor_from_id(const ActorId id)
{
        switch (id)
        {
        case ActorId::player:
                return new Player();

        case ActorId::khephren:
                return new Khephren();

        case ActorId::ape:
                return new Ape();

        case ActorId::strange_color:
                return new StrangeColor();

        case ActorId::spectral_wpn:
                return new SpectralWpn();

        default:
                break;
        }

        return new Mon();
}

static std::vector<P> free_spawn_positions(const R& area)
{
        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::yes)
                .run(blocked,
                     area,
                     MapParseMode::overwrite);

        const auto free_positions = to_vec(blocked, false, area);

        return free_positions;
}

static Mon* spawn_at(const P& pos, const ActorId id)
{
        ASSERT(map::is_pos_inside_outer_walls(pos));

        Actor* const actor = actor_factory::make(id, pos);

        Mon* const mon = static_cast<Mon*>(actor);

        if (map::g_player->can_see_actor(*mon))
        {
                mon->set_player_aware_of_me();
        }

        return mon;
}

static MonSpawnResult spawn_at_positions(const std::vector<P>& positions,
                                         const std::vector<ActorId>& ids)
{
        MonSpawnResult result;

        const size_t nr_to_spawn = std::min(positions.size(), ids.size());

        for (size_t i = 0; i < nr_to_spawn; ++i)
        {
                const P& pos = positions[i];

                const ActorId id = ids[i];

                result.m_monsters.emplace_back(
                        spawn_at(pos, id));
        }

        return result;
}

// -----------------------------------------------------------------------------
// MonSpawnResult
// -----------------------------------------------------------------------------
MonSpawnResult& MonSpawnResult::set_leader(Actor* const leader)
{
        std::for_each(
                std::begin(m_monsters),
                std::end(m_monsters),
                [leader](auto mon)
                {
                        mon->m_leader = leader;
                });

        return *this;
}

MonSpawnResult& MonSpawnResult::make_aware_of_player()
{
        std::for_each(
                std::begin(m_monsters),
                std::end(m_monsters),
                [](auto mon)
                {
                        mon->m_aware_of_player_counter =
                                mon->m_data->nr_turns_aware;
                });

        return *this;
}

// -----------------------------------------------------------------------------
// actor_factory
// -----------------------------------------------------------------------------
namespace actor_factory
{

Actor* make(const ActorId id, const P& pos)
{
        Actor* const actor = make_actor_from_id(id);

        actor::init_actor(*actor, pos, actor_data::g_data[(size_t)id]);

        if (actor->m_data->nr_left_allowed_to_spawn > 0)
        {
                --actor->m_data->nr_left_allowed_to_spawn;
        }

        game_time::add_actor(actor);

        actor->m_properties.on_placed();

#ifndef NDEBUG
        if (map::nr_cells() != 0)
        {
                const auto* const f = map::g_cells.at(pos).rigid;

                if (f->id() == FeatureId::door)
                {
                        const auto* const door = static_cast<const Door*>(f);

                        ASSERT(
                                door->is_open() ||
                                actor->m_properties.has(PropId::ooze) ||
                                actor->m_properties.has(PropId::ethereal));
                }
        }
#endif // NDEBUG

        return actor;
}

void delete_all_mon()
{
        std::vector<Actor*>& actors = game_time::g_actors;

        for (auto it = begin(actors); it != end(actors);)
        {
                Actor* const actor = *it;

                if (actor->is_player())
                {
                        ++it;
                }
                else // Is monster
                {
                        delete actor;

                        it = actors.erase(it);
                }
        }
}

MonSpawnResult spawn(const P& origin,
                     const std::vector<ActorId>& monster_ids,
                     const R& area_allowed)
{
        TRACE_FUNC_BEGIN;

        auto free_positions = free_spawn_positions(area_allowed);

        if (free_positions.empty())
        {
                return MonSpawnResult();
        }

        std::sort(begin(free_positions),
                  end(free_positions),
                  IsCloserToPos(origin));

        const auto result = spawn_at_positions(free_positions, monster_ids);

        TRACE_FUNC_END;

        return result;
}

MonSpawnResult spawn_random_position(const std::vector<ActorId>& monster_ids,
                                     const R& area_allowed)
{
        TRACE_FUNC_BEGIN;

        auto free_positions = free_spawn_positions(area_allowed);

        if (free_positions.empty())
        {
                return MonSpawnResult();
        }

        rnd::shuffle(free_positions);

        const auto result = spawn_at_positions(free_positions, monster_ids);

        TRACE_FUNC_END;

        return result;
}

} // actor_factory
