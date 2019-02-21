// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "feature.hpp"

#include "init.hpp"
#include "actor.hpp"
#include "actor_player.hpp"
#include "msg_log.hpp"
#include "io.hpp"
#include "map_parsing.hpp"
#include "game_time.hpp"
#include "map_travel.hpp"
#include "query.hpp"
#include "saving.hpp"
#include "popup.hpp"
#include "map.hpp"
#include "feature_data.hpp"

const FeatureData& Feature::data() const
{
        return feature_data::data(id());
}

void Feature::bump(actor::Actor& actor_bumping)
{
        if (!can_move(actor_bumping) && actor_bumping.is_player())
        {
                if (map::g_cells.at(m_pos).is_seen_by_player)
                {
                        msg_log::add(data().msg_on_player_blocked);
                }
                else
                {
                        msg_log::add(data().msg_on_player_blocked_blind);
                }
        }
}

void Feature::on_leave(actor::Actor& actor_leaving)
{
        (void)actor_leaving;
}

void Feature::add_light(Array2<bool>& light) const
{
        (void)light;
}

void Feature::reveal(const Verbosity verbosity)
{
        (void)verbosity;
}

bool Feature::is_walkable() const
{
        return data().move_rules.is_walkable();
}

bool Feature::can_move(const actor::Actor& actor) const
{
        return data().move_rules.can_move(actor);
}

bool Feature::is_sound_passable() const
{
        return data().is_sound_passable;
}

bool Feature::is_los_passable() const
{
        return data().is_los_passable;
}

bool Feature::is_projectile_passable() const
{
        return data().is_projectile_passable;
}

bool Feature::is_smoke_passable() const
{
        return data().is_smoke_passable;
}

char Feature::character() const
{
        return data().character;
}

TileId Feature::tile() const
{
        return data().tile;
}

bool Feature::can_have_corpse() const
{
        return data().can_have_corpse;
}

bool Feature::can_have_rigid() const
{
        return data().can_have_rigid;
}

bool Feature::can_have_blood() const
{
        return data().can_have_blood;
}

bool Feature::can_have_gore() const
{
        return data().can_have_gore;
}

bool Feature::can_have_item() const
{
        return data().can_have_item;
}

FeatureId Feature::id() const
{
        return data().id;
}

int Feature::shock_when_adj() const
{
        return data().shock_when_adjacent;
}

Matl Feature::matl() const
{
        return data().matl_type;
}
