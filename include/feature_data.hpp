// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef FEATURE_DATA_HPP
#define FEATURE_DATA_HPP

#include <functional>

#include "gfx.hpp"
#include "property_data.hpp"
#include "global.hpp"

enum class FeatureId
{
        floor,
        bridge,
        wall,
        tree,
        grass,
        bush,
        vines,
        chains,
        grate,
        stairs,
        lever,
        brazier,
        gravestone,
        tomb,
        church_bench,
        altar,
        carpet,
        rubble_high,
        rubble_low,
        bones,
        statue,
        cocoon,
        chest,
        cabinet,
        bookshelf,
        alchemist_bench,
        fountain,
        monolith,
        pylon,
        stalagmite,
        chasm,
        liquid_shallow,
        liquid_deep,
        door,
        lit_dynamite,
        lit_flare,
        trap,
        smoke,
        force_field,
        event_wall_crumble,
        event_snake_emerge,
        event_rat_cave_discovery,

        END
};

enum class FeaturePlacement
{
        adj_to_walls,
        away_from_walls,
        either
};

class Actor;

class MoveRules
{
public:
        MoveRules()
        {
                reset();
        }

        ~MoveRules() {}

        void reset()
        {
                m_is_walkable = false;

                m_props_allow_move.clear();
        }

        void set_prop_can_move(const PropId id)
        {
                m_props_allow_move.push_back(id);
        }

        void set_walkable()
        {
                m_is_walkable = true;
        }

        bool is_walkable() const
        {
                return m_is_walkable;
        }

        bool can_move(const Actor& actor) const;

private:
        bool m_is_walkable;
        std::vector<PropId> m_props_allow_move;
};

class Feature;

struct FeatureData
{
        std::function<Feature*(const P& p)> make_obj;
        FeatureId id;
        char character;
        TileId tile;
        MoveRules move_rules;
        bool is_sound_passable;
        bool is_projectile_passable;
        bool is_los_passable;
        bool is_smoke_passable;
        bool can_have_blood;
        bool can_have_gore;
        bool can_have_corpse;
        bool can_have_rigid;
        bool can_have_item;
        Matl matl_type;
        std::string msg_on_player_blocked;
        std::string msg_on_player_blocked_blind;
        int shock_when_adjacent;
        FeaturePlacement auto_spawn_placement;
};

namespace feature_data
{

void init();

const FeatureData& data(const FeatureId id);

} // feature_data

#endif // FEATURE_DATA_HPP
