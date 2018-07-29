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

void Feature::bump(Actor& actor_bumping)
{
        if (!can_move(actor_bumping) && actor_bumping.is_player())
        {
                if (map::cells.at(pos_).is_seen_by_player)
                {
                        msg_log::add(data().msg_on_player_blocked);
                }
                else
                {
                        msg_log::add(data().msg_on_player_blocked_blind);
                }
        }
}

void Feature::on_leave(Actor& actor_leaving)
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

bool Feature::can_move_common() const
{
        return data().move_rules.can_move_common();
}

bool Feature::can_move(const Actor& actor) const
{
        return data().move_rules.can_move(actor);
}

void Feature::hit(const int dmg,
                  const DmgType dmg_type,
                  const DmgMethod dmg_method,
                  Actor* const actor)
{
        (void)dmg;
        (void)dmg_type;
        (void)dmg_method;
        (void)actor;
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

bool Feature::is_bottomless() const
{
        return data().is_bottomless;
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
