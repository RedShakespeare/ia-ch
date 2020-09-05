// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "populate_monsters.hpp"

#include <algorithm>

#include "actor_factory.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "debug.hpp"
#include "flood.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "random.hpp"
#include "terrain_trap.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
enum class AllowSpawnUniqueMon
{
        no,
        yes
};

static const int s_min_dist_to_player = g_fov_radi_int + 4;

static int random_out_of_depth()
{
        int nr_levels = 0;

        if ( ( map::g_dlvl > 0 ) && rnd::one_in( 14 ) )
        {
                nr_levels = 3;
        }

        return nr_levels;
}

static WeightedItems<actor::Id> valid_auto_spawn_monsters(
        const int nr_lvls_out_of_depth,
        const AllowSpawnUniqueMon allow_spawn_unique )
{
        WeightedItems<actor::Id> weighted_ids;

        int effective_dlvl =
                std::clamp(
                        map::g_dlvl + nr_lvls_out_of_depth,
                        1,
                        g_dlvl_last );

        if ( config::is_gj_mode() )
        {
                effective_dlvl = rnd::range( effective_dlvl, g_dlvl_last );
        }

        // Get list of actors currently on the level to help avoid spawning
        // multiple unique monsters of the same id
        bool spawned_ids[ (size_t)actor::Id::END ] = {};

        for ( const auto* const actor : game_time::g_actors )
        {
                spawned_ids[ (size_t)actor->id() ] = true;
        }

        for ( const auto& d : actor::g_data )
        {
                if ( d.id == actor::Id::player )
                {
                        continue;
                }

                if ( ! d.is_auto_spawn_allowed )
                {
                        continue;
                }

                if ( d.nr_left_allowed_to_spawn == 0 )
                {
                        continue;
                }

                if ( effective_dlvl < d.spawn_min_dlvl )
                {
                        continue;
                }

                if ( ( d.spawn_max_dlvl != -1 ) &&
                     ( effective_dlvl > d.spawn_max_dlvl ) )
                {
                        continue;
                }

                if ( d.is_unique && spawned_ids[ (size_t)d.id ] )
                {
                        continue;
                }

                if ( d.is_unique &&
                     ( allow_spawn_unique == AllowSpawnUniqueMon::no ) )
                {
                        continue;
                }

                weighted_ids.items.push_back( d.id );

                weighted_ids.weights.push_back( d.spawn_weight );
        }

        return weighted_ids;
}

static bool make_random_group_for_room(
        const RoomType room_type,
        const std::vector<P>& sorted_free_cells,
        Array2<bool>& blocked_out )
{
        TRACE_FUNC_BEGIN_VERBOSE;

        const int nr_lvls_out_of_depth_allowed = random_out_of_depth();

        auto id_bucket =
                valid_auto_spawn_monsters(
                        nr_lvls_out_of_depth_allowed,
                        AllowSpawnUniqueMon::yes );

        // Remove monsters which do not belong in this room
        for ( size_t i = 0; i < id_bucket.items.size(); ++i )
        {
                // Ocassionally allow any monster type, to mix things up a bit
                const int allow_any_one_in_n = 20;

                if ( rnd::one_in( allow_any_one_in_n ) )
                {
                        // Any monster type allowed - keep the monster
                        continue;
                }

                const auto id = id_bucket.items[ i ];

                const auto& d = actor::g_data[ (size_t)id ];

                if ( std::find(
                             std::begin( d.native_rooms ),
                             std::end( d.native_rooms ),
                             room_type ) != std::end( d.native_rooms ) )
                {
                        // Monster is native to room - keep the monster
                        continue;
                }

                // Monster not allowed - erase it
                id_bucket.items.erase( id_bucket.items.begin() + i );

                id_bucket.weights.erase( id_bucket.weights.begin() + i );

                --i;
        }

        if ( id_bucket.items.empty() )
        {
                TRACE_VERBOSE
                        << "Found no valid monsters to spawn at room type ("
                        << std::to_string( int( room_type ) ) + ")"
                        << std::endl;

                TRACE_FUNC_END_VERBOSE;

                return false;
        }
        else
        {
                // Found valid monster IDs
                const auto id = rnd::weighted_choice( id_bucket );

                populate_mon::make_group_at(
                        id,
                        sorted_free_cells,
                        &blocked_out,
                        MonRoamingAllowed::yes );

                TRACE_FUNC_END_VERBOSE;

                return true;
        }
}

static void make_random_group_at(
        const std::vector<P>& sorted_free_cells,
        Array2<bool>& blocked_out,
        const int nr_lvls_out_of_depth_allowed,
        const MonRoamingAllowed is_roaming_allowed,
        const AllowSpawnUniqueMon allow_spawn_unique )
{
        const auto id_bucket =
                valid_auto_spawn_monsters(
                        nr_lvls_out_of_depth_allowed,
                        allow_spawn_unique );

        if ( id_bucket.items.empty() )
        {
                return;
        }

        const auto id = rnd::weighted_choice( id_bucket );

        populate_mon::make_group_at(
                id,
                sorted_free_cells,
                &blocked_out,
                is_roaming_allowed );
}

// -----------------------------------------------------------------------------
// populate_mon
// -----------------------------------------------------------------------------
namespace populate_mon
{
void make_group_at(
        const actor::Id id,
        const std::vector<P>& sorted_free_cells,
        Array2<bool>* const blocked_out,
        const MonRoamingAllowed is_roaming_allowed )
{
        const auto& d = actor::g_data[ (size_t)id ];

        int max_nr_in_group = 1;

        // First, determine the type of group by a weighted choice
        std::vector<int> weights;

        for ( const auto& rule : d.group_sizes )
        {
                weights.push_back( rule.weight );
        }

        const int rnd_choice = rnd::weighted_choice( weights );

        const auto group_size = d.group_sizes[ rnd_choice ].group_size;

        // Determine the number of monsters to spawn based on the group type
        switch ( group_size )
        {
        case actor::MonGroupSize::few:
                max_nr_in_group = rnd::range( 2, 3 );
                break;

        case actor::MonGroupSize::pack:
                max_nr_in_group = rnd::range( 4, 5 );
                break;

        case actor::MonGroupSize::swarm:
                max_nr_in_group = rnd::range( 9, 11 );
                break;

        default:
                break;
        }

        actor::Actor* origin_actor = nullptr;

        const int nr_free_cells = sorted_free_cells.size();

        const int nr_can_be_spawned = std::min( nr_free_cells, max_nr_in_group );

        for ( int i = 0; i < nr_can_be_spawned; ++i )
        {
                const P& p = sorted_free_cells[ i ];

                if ( blocked_out )
                {
                        ASSERT( ! blocked_out->at( p ) );
                }

                auto* const actor = actor::make( id, p );

                actor->m_ai_state.is_roaming_allowed = is_roaming_allowed;

                if ( i == 0 )
                {
                        origin_actor = actor;
                }
                else
                {
                        // Not origin actor

                        // The monster may have been assigned a leader when
                        // placed (e.g. Ghouls allied to a player Ghoul, or
                        // other special cases). If not, we assign the origin
                        // monster as leader of this group.
                        if ( ! actor->m_leader )
                        {
                                actor->m_leader = origin_actor;
                        }
                }

                if ( blocked_out )
                {
                        blocked_out->at( p ) = true;
                }
        }
}  // make_group_at

std::vector<P> make_sorted_free_cells(
        const P& origin,
        const Array2<bool>& blocked )
{
        std::vector<P> out;

        const int radi = 10;

        const P dims = blocked.dims();

        const int x0 = std::clamp( origin.x - radi, 1, dims.x - 2 );
        const int y0 = std::clamp( origin.y - radi, 1, dims.y - 2 );
        const int x1 = std::clamp( origin.x + radi, 1, dims.x - 2 );
        const int y1 = std::clamp( origin.y + radi, 1, dims.y - 2 );

        for ( int x = x0; x <= x1; ++x )
        {
                for ( int y = y0; y <= y1; ++y )
                {
                        if ( ! blocked.at( x, y ) )
                        {
                                out.emplace_back( x, y );
                        }
                }
        }

        IsCloserToPos sorter( origin );

        std::sort( std::begin( out ), std::end( out ), sorter );

        return out;
}

Array2<bool> forbidden_spawn_positions()
{
        Array2<bool> blocked( map::dims() );

        map_parsers::BlocksWalking( ParseActors::yes )
                .run( blocked, blocked.rect() );

        const P& player_p = map::g_player->m_pos;

        {
                // Checking which cells projectiles can travel through, as a
                // general way of blocking cells within a certain number of
                // steps to the player (We cannot just check for cells blocking
                // walking, as that could for example spawn monsters on the
                // other side of water very close to the player)
                Array2<bool> blocks_projectiles( map::dims() );

                map_parsers::BlocksProjectiles()
                        .run( blocks_projectiles, blocks_projectiles.rect() );

                const auto flood = floodfill( player_p, blocks_projectiles );

                for ( size_t i = 0; i < map::nr_cells(); ++i )
                {
                        const int v = flood.at( i );

                        if ( ( v > 0 ) && ( v < s_min_dist_to_player ) )
                        {
                                blocked.at( i ) = true;
                        }
                }
        }

        blocked.at( player_p ) = true;

        return blocked;
}

void spawn_for_repopulate_over_time()
{
        TRACE_FUNC_BEGIN;

        if ( game_time::g_actors.size() >= g_max_nr_actors_on_map )
        {
                return;
        }

        Array2<bool> blocked( map::dims() );

        map_parsers::BlocksWalking( ParseActors::yes )
                .run( blocked, blocked.rect() );

        const P& player_pos = map::g_player->m_pos;

        const int x0 = std::max(
                0,
                player_pos.x - s_min_dist_to_player );

        const int y0 = std::max(
                0,
                player_pos.y - s_min_dist_to_player );

        const int x1 = std::min(
                map::w() - 1,
                player_pos.x + s_min_dist_to_player );

        const int y1 = std::min(
                map::h() - 1,
                player_pos.y + s_min_dist_to_player );

        for ( int x = x0; x <= x1; ++x )
        {
                for ( int y = y0; y <= y1; ++y )
                {
                        blocked.at( x, y ) = true;
                }
        }

        std::vector<P> free_cells_vector;

        for ( int x = 1; x < map::w() - 2; ++x )
        {
                for ( int y = 1; y < map::h() - 2; ++y )
                {
                        if ( ! blocked.at( x, y ) )
                        {
                                free_cells_vector.emplace_back( x, y );
                        }
                }
        }

        if ( free_cells_vector.empty() )
        {
                TRACE_FUNC_END;

                return;
        }

        const P origin = rnd::element( free_cells_vector );

        free_cells_vector = make_sorted_free_cells( origin, blocked );

        if ( free_cells_vector.empty() )
        {
                TRACE_FUNC_END;

                return;
        }

        if ( map::g_cells.at( origin ).is_explored )
        {
                const int nr_ood = random_out_of_depth();

                make_random_group_at(
                        free_cells_vector,
                        blocked,
                        nr_ood,
                        MonRoamingAllowed::yes,
                        AllowSpawnUniqueMon::no );
        }

        TRACE_FUNC_END;
}  // spawn_for_repopulate_over_time

void populate_std_lvl()
{
        // TODO: This function seems weird and unnecessarily complicated,
        // consider just picking random rooms and dropping a monster group for
        // that room in a random room position - or even just pick free cells on
        // the map at random and place a monster group there for whatever room
        // it happens to be?

        TRACE_FUNC_BEGIN;

        const int nr_groups_to_spawn = rnd::range( 5, 7 );

        int nr_groups_spawned = 0;

        auto blocked = forbidden_spawn_positions();

        // First, attempt to populate all non-plain standard rooms
        for ( Room* const room : map::g_room_list )
        {
                if ( ( room->m_type == RoomType::plain ) ||
                     ( room->m_type >= RoomType::END_OF_STD_ROOMS ) )
                {
                        continue;
                }

                // TODO: This is not a good method to calculate the number of
                // room cells (the room may be irregularly shaped), parse the
                // room map instead
                const int room_w = room->m_r.p1.x - room->m_r.p0.x + 1;
                const int room_h = room->m_r.p1.y - room->m_r.p0.y + 1;

                const int nr_cells_in_room = room_w * room_h;

                const int max_nr_groups_in_room =
                        room->max_nr_mon_groups_spawned();

                const int nr_groups_to_try =
                        rnd::range( 1, max_nr_groups_in_room );

                for ( int i = 0; i < nr_groups_to_try; ++i )
                {
                        // Randomly pick a free position inside the room
                        std::vector<P> origin_bucket;

                        origin_bucket.reserve( room->m_r.w() * room->m_r.h() );

                        for ( int y = room->m_r.p0.y;
                              y <= room->m_r.p1.y;
                              ++y )
                        {
                                for ( int x = room->m_r.p0.x;
                                      x <= room->m_r.p1.x;
                                      ++x )
                                {
                                        const bool is_current_room =
                                                map::g_room_map.at( x, y ) ==
                                                room;

                                        if ( is_current_room &&
                                             ! blocked.at( x, y ) )
                                        {
                                                origin_bucket.emplace_back( x, y );
                                        }
                                }
                        }

                        // If room is too full (due to spawned monsters
                        // and terrains), stop spawning in this room
                        const int nr_origin_candidates = origin_bucket.size();

                        if ( nr_origin_candidates < ( nr_cells_in_room / 3 ) )
                        {
                                break;
                        }

                        // Spawn monsters in room
                        if ( nr_origin_candidates > 0 )
                        {
                                const P origin = rnd::element( origin_bucket );

                                const auto sorted_free_cells =
                                        make_sorted_free_cells( origin, blocked );

                                const bool did_make_group =
                                        make_random_group_for_room(
                                                room->m_type,
                                                sorted_free_cells,
                                                blocked );

                                if ( did_make_group )
                                {
                                        ++nr_groups_spawned;

                                        if ( nr_groups_spawned >=
                                             nr_groups_to_spawn )
                                        {
                                                TRACE_FUNC_END;
                                                return;
                                        }
                                }
                        }
                }

                // After attempting to populate a non-plain themed room,
                // mark that area as forbidden
                for ( int y = room->m_r.p0.y; y <= room->m_r.p1.y; ++y )
                {
                        for ( int x = room->m_r.p0.x; x <= room->m_r.p1.x; ++x )
                        {
                                if ( map::g_room_map.at( x, y ) == room )
                                {
                                        blocked.at( x, y ) = true;
                                }
                        }
                }
        }

        // Second, place groups randomly in plain-themed areas until no more
        // groups to place
        std::vector<P> origin_bucket;

        origin_bucket.reserve( map::w() * map::h() );

        for ( int y = 1; y < map::h() - 1; ++y )
        {
                for ( int x = 1; x < map::w() - 1; ++x )
                {
                        Room* const room = map::g_room_map.at( x, y );

                        if ( ! blocked.at( x, y ) &&
                             room &&
                             ( room->m_type == RoomType::plain ) )
                        {
                                origin_bucket.emplace_back( x, y );
                        }
                }
        }

        if ( ! origin_bucket.empty() )
        {
                while ( nr_groups_spawned < nr_groups_to_spawn )
                {
                        const P origin = rnd::element( origin_bucket );

                        const auto sorted_free_cells =
                                make_sorted_free_cells( origin, blocked );

                        const bool did_make_group =
                                make_random_group_for_room(
                                        RoomType::plain,
                                        sorted_free_cells,
                                        blocked );

                        if ( did_make_group )
                        {
                                ++nr_groups_spawned;
                        }
                }
        }

        TRACE_FUNC_END;
}  // populate_std_lvl

void populate_lvl_as_room_types( const std::vector<RoomType>& room_types )
{
        TRACE_FUNC_BEGIN;

        if ( room_types.empty() )
        {
                ASSERT( false );

                return;
        }

        auto blocked = forbidden_spawn_positions();

        std::vector<P> origin_bucket;

        origin_bucket.reserve( map::w() * map::h() );

        for ( int y = 1; y < map::h() - 1; ++y )
        {
                for ( int x = 1; x < map::w() - 1; ++x )
                {
                        if ( ! blocked.at( x, y ) )
                        {
                                origin_bucket.emplace_back( x, y );
                        }
                }
        }

        if ( origin_bucket.empty() )
        {
                return;
        }

        const int nr_groups_to_spawn = rnd::range( 5, 7 );

        int nr_groups_spawned = 0;
        int nr_failed = 0;

        while ( nr_groups_spawned < nr_groups_to_spawn )
        {
                const auto origin = rnd::element( origin_bucket );

                const auto sorted_free_cells =
                        make_sorted_free_cells( origin, blocked );

                const auto room_type = rnd::element( room_types );

                const bool did_make_group =
                        make_random_group_for_room(
                                room_type,
                                sorted_free_cells,
                                blocked );

                if ( did_make_group )
                {
                        ++nr_groups_spawned;
                }
                else
                {
                        // Give up after too many failed attempts - it must not
                        // be possible to loop forever
                        ++nr_failed;

                        // Just a random large number
                        const int nr_tries_allowed = 10000;

                        if ( nr_failed >= nr_tries_allowed )
                        {
                                break;
                        }
                }
        }

        TRACE_FUNC_END;
}  // populate_lvl_as_room_types

}  // namespace populate_mon
