#include "teleport.hpp"

#include <memory>

#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "feature_door.hpp"
#include "feature_mob.hpp"
#include "game_time.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "marker.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static bool is_void_traveler_affecting_player_teleport(const Actor& actor)
{
        const auto actor_id = actor.id();

        const bool is_void_traveler =
                (actor_id == ActorId::void_traveler) ||
                (actor_id == ActorId::elder_void_traveler);

        return
                is_void_traveler &&
                (actor.state == ActorState::alive) &&
                actor.properties.allow_act() &&
                !actor.is_actor_my_leader(map::player) &&
                (static_cast<const Mon&>(actor).aware_of_player_counter_ > 0);
}

static std::vector<P> get_free_positions_around_pos(
        const P& p,
        const Array2<bool>& blocked)
{
        std::vector<P> free_positions;

        for (const P& d : dir_utils::dir_list)
        {
                const P adj_p(p + d);

                if (!blocked.at(adj_p))
                {
                        free_positions.push_back(adj_p);
                }
        }

        return free_positions;
}

static void make_all_mon_not_seeing_player_unaware()
{
        Array2<bool> blocks_los(map::dims());

        const R r = fov::fov_rect(map::player->pos);

        map_parsers::BlocksLos()
                .run(blocks_los,
                     r,
                     MapParseMode::overwrite);

        for (auto* const other_actor : game_time::actors)
        {
                if (other_actor == map::player)
                {
                        continue;
                }

                auto* const mon = static_cast<Mon*>(other_actor);

                if (!mon->can_see_actor(*map::player, blocks_los))
                {
                        mon->aware_of_player_counter_ = 0;
                }
        }
}

static void make_player_aware_of_all_seen_mon()
{
        const auto player_seen_actors = map::player->seen_actors();

        for (Actor* const actor : player_seen_actors)
        {
                static_cast<Mon*>(actor)->set_player_aware_of_me();
        }
}

static void confuse_player()
{
        msg_log::add("I suddenly find myself in a different location!");

        auto prop = new PropConfused();

        prop->set_duration(8);

        map::player->properties.apply(prop);
}

static bool should_player_ctrl_tele(const ShouldCtrlTele ctrl_tele)
{
        switch (ctrl_tele)
        {
        case ShouldCtrlTele::always:
        {
                return true;
        }

        case ShouldCtrlTele::never:
        {
                return false;
        }

        case ShouldCtrlTele::if_tele_ctrl_prop:
        {
                const bool has_tele_ctrl =
                        map::player->properties.has(PropId::tele_ctrl);

                const bool is_confused =
                        map::player->properties.has(PropId::confused);

                return has_tele_ctrl && !is_confused;
        }
        }

        ASSERT(false);

        return false;
}

// -----------------------------------------------------------------------------
// teleport
// -----------------------------------------------------------------------------
void teleport(Actor& actor, const ShouldCtrlTele ctrl_tele)
{
        Array2<bool> blocks_flood(map::dims());

        map_parsers::BlocksActor(actor, ParseActors::no)
                .run(blocks_flood, blocks_flood.rect());

        const size_t len = map::nr_cells();

        // Allow teleporting past non-metal doors for the player, and past any
        // door for monsters
        for (size_t i = 0; i < len; ++i)
        {
                const auto* const r = map::cells.at(i).rigid;

                if (r->id() == FeatureId::door)
                {
                        const auto* const door = static_cast<const Door*>(r);

                        if ((door->type() != DoorType::metal) ||
                            !actor.is_player())
                        {
                                blocks_flood.at(i) = false;
                        }
                }
        }

        // Allow teleporting past Force Fields, since they are temporary
        for (const auto* const mob : game_time::mobs)
        {
                if (mob->id() == FeatureId::force_field)
                {
                        blocks_flood.at(mob->pos()) = false;
                }
        }

        const auto flood = floodfill(actor.pos, blocks_flood);

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksActor(actor, ParseActors::yes)
                .run(blocked, blocked.rect());

        for (size_t i = 0; i < len; ++i)
        {
                if (flood.at(i) <= 0)
                {
                        blocked.at(i) = true;
                }
        }

        blocked.at(actor.pos) = false;

        // Teleport control?
        if (actor.is_player() && should_player_ctrl_tele(ctrl_tele))
        {
                auto tele_ctrl_state =
                        std::make_unique<CtrlTele>(
                                actor.pos,
                                blocked);

                states::push(std::move(tele_ctrl_state));

                return;
        }

        // No teleport control - teleport randomly
        const auto pos_bucket = to_vec(blocked, false, blocked.rect());

        if (pos_bucket.empty())
        {
                return;
        }

        const P tgt_pos = rnd::element(pos_bucket);

        teleport(actor, tgt_pos, blocked);
}

void teleport(Actor& actor, P p, const Array2<bool>& blocked)
{
        if (!actor.is_player() && map::player->can_see_actor(actor))
        {
                const std::string actor_name_the =
                        text_format::first_to_upper(
                                actor.name_the());

                msg_log::add(actor_name_the + mon_disappear_msg);
        }

        if (!actor.is_player())
        {
                static_cast<Mon&>(actor).player_aware_of_me_counter_ = 0;
        }

        actor.properties.end_prop_silent(PropId::entangled);

        // Hostile void travelers "intercepts" players teleporting, and calls
        // the player to them
        bool is_affected_by_void_traveler = false;

        if (actor.is_player())
        {
                for (Actor* const other_actor : game_time::actors)
                {
                        if (!is_void_traveler_affecting_player_teleport(
                                    *other_actor))
                        {
                                continue;
                        }

                        const std::vector<P> p_bucket =
                                get_free_positions_around_pos(
                                        other_actor->pos,
                                        blocked);

                        if (p_bucket.empty())
                        {
                                continue;
                        }

                        // Set new teleport destination
                        p = rnd::element(p_bucket);

                        const std::string actor_name_a =
                                text_format::first_to_upper(
                                        other_actor->name_a());

                        msg_log::add(
                                actor_name_a +
                                " intercepts my teleportation!");

                        static_cast<Mon*>(other_actor)
                                ->become_aware_player(false);

                        is_affected_by_void_traveler = true;

                        break;
                }
        }

        // Leave current cell
        map::cells.at(actor.pos).rigid->on_leave(actor);

        // Update actor position to new position
        actor.pos = p;

        map::update_vision();

        if (actor.is_player())
        {
                make_all_mon_not_seeing_player_unaware();
        }

        make_player_aware_of_all_seen_mon();

        const bool has_tele_ctrl = actor.properties.has(PropId::tele_ctrl);

        const bool is_confused = actor.properties.has(PropId::confused);

        if (actor.is_player() &&
            (!has_tele_ctrl ||
             is_confused ||
             is_affected_by_void_traveler))
        {
                confuse_player();
        }

        // Bump the target feature, so that we for example start swimming if
        // teleporting into water
        map::cells.at(p).rigid->bump(actor);
}
