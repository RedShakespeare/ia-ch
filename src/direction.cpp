// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "direction.hpp"

#include "debug.hpp"
#include "pos.hpp"
#include "random.hpp"

namespace dir_utils
{
const std::vector<P> g_cardinal_list {
        { -1, 0 },
        { 1, 0 },
        { 0, -1 },
        { 0, 1 } };

const std::vector<P> g_cardinal_list_w_center {
        { 0, 0 },
        { -1, 0 },
        { 1, 0 },
        { 0, -1 },
        { 0, 1 } };

const std::vector<P> g_dir_list {
        { -1, 0 },
        { 1, 0 },
        { 0, -1 },
        { 0, 1 },
        { -1, -1 },
        { -1, 1 },
        { 1, -1 },
        { 1, 1 } };

const std::vector<P> g_dir_list_w_center {
        { 0, 0 },
        { -1, 0 },
        { 1, 0 },
        { 0, -1 },
        { 0, 1 },
        { -1, -1 },
        { -1, 1 },
        { 1, -1 },
        { 1, 1 } };

static const std::string g_compass_dir_names[ 3 ][ 3 ] =
        {
                { "NW", "N", "NE" },
                {
                        "W",
                        "",
                        "E",
                },
                { "SW", "S", "SE" } };

static const double s_pi_db = 3.14159265;

static const double s_angle_45_db = 2 * s_pi_db / 8;

static const double s_angle_45_half_db = s_angle_45_db / 2.0;

static const double edge[ 4 ] =
        {
                s_angle_45_half_db + ( s_angle_45_db * 0 ),
                s_angle_45_half_db + ( s_angle_45_db * 1 ),
                s_angle_45_half_db + ( s_angle_45_db * 2 ),
                s_angle_45_half_db + ( s_angle_45_db * 3 ) };

Dir dir( const P& offset )
{
        ASSERT( offset.x >= -1 &&
                offset.y >= -1 &&
                offset.x <= 1 &&
                offset.y <= 1 );

        if ( offset.y == -1 )
        {
                return offset.x == -1 ? Dir::up_left : offset.x == 0 ? Dir::up : offset.x == 1 ? Dir::up_right : Dir::END;
        }

        if ( offset.y == 0 )
        {
                return offset.x == -1 ? Dir::left : offset.x == 0 ? Dir::center : offset.x == 1 ? Dir::right : Dir::END;
        }

        if ( offset.y == 1 )
        {
                return offset.x == -1 ? Dir::down_left : offset.x == 0 ? Dir::down : offset.x == 1 ? Dir::down_right : Dir::END;
        }

        return Dir::END;
}

P offset( const Dir dir )
{
        ASSERT( dir != Dir::END );

        switch ( dir )
        {
        case Dir::down_left:
                return P( -1, 1 );

        case Dir::down:
                return P( 0, 1 );

        case Dir::down_right:
                return P( 1, 1 );

        case Dir::left:
                return P( -1, 0 );

        case Dir::center:
                return P( 0, 0 );

        case Dir::right:
                return P( 1, 0 );

        case Dir::up_left:
                return P( -1, -1 );

        case Dir::up:
                return P( 0, -1 );

        case Dir::up_right:
                return P( 1, -1 );

        case Dir::END:
                return P( 0, 0 );
        }

        return P( 0, 0 );
}

P rnd_adj_pos( const P& origin, const bool is_center_allowed )
{
        const std::vector<P>* vec = nullptr;

        if ( is_center_allowed )
        {
                vec = &g_dir_list_w_center;
        }
        else
        {
                // Center not allowed
                vec = &g_dir_list;
        }

        return origin + rnd::element( *vec );
}

std::string compass_dir_name( const P& from_pos, const P& to_pos )
{
        std::string name;

        const P offset( to_pos - from_pos );

        const double angle_db = atan2( -offset.y, offset.x );

        if ( angle_db < -edge[ 2 ] && angle_db > -edge[ 3 ] )
        {
                name = "SW";
        }
        else if ( angle_db <= -edge[ 1 ] && angle_db >= -edge[ 2 ] )
        {
                name = "S";
        }
        else if ( angle_db < -edge[ 0 ] && angle_db > -edge[ 1 ] )
        {
                name = "SE";
        }
        else if ( angle_db >= -edge[ 0 ] && angle_db <= edge[ 0 ] )
        {
                name = "E";
        }
        else if ( angle_db > edge[ 0 ] && angle_db < edge[ 1 ] )
        {
                name = "NE";
        }
        else if ( angle_db >= edge[ 1 ] && angle_db <= edge[ 2 ] )
        {
                name = "N";
        }
        else if ( angle_db > edge[ 2 ] && angle_db < edge[ 3 ] )
        {
                name = "NW";
        }
        else
        {
                name = "W";
        }

        return name;
}

std::string compass_dir_name( const Dir dir )
{
        const P& o = offset( dir );

        return g_compass_dir_names[ o.x + 1 ][ o.y + 1 ];
}

std::string compass_dir_name( const P& offs )
{
        return g_compass_dir_names[ offs.x + 1 ][ offs.y + 1 ];
}

}  // namespace dir_utils
