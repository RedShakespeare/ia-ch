#include "feature_event.hpp"

#include "actor_player.hpp"
#include "actor_mon.hpp"
#include "map.hpp"
#include "actor_factory.hpp"
#include "msg_log.hpp"
#include "io.hpp"
#include "feature_rigid.hpp"
#include "popup.hpp"
#include "sdl_base.hpp"
#include "init.hpp"
#include "property.hpp"
#include "property_handler.hpp"

// -----------------------------------------------------------------------------
// Event
// -----------------------------------------------------------------------------
Event::Event(const P& feature_pos) :
        Mob(feature_pos) {}

// -----------------------------------------------------------------------------
// Wall crumble
// -----------------------------------------------------------------------------
EventWallCrumble::EventWallCrumble(
        const P& p,
        std::vector<P>& walls,
        std::vector<P>& inner) :
        Event(p),
        wall_cells_(walls),
        inner_cells_(inner) {}

void EventWallCrumble::on_new_turn()
{
        if (!is_pos_adj(map::player->pos, pos_, true))
        {
                return;
        }

        auto is_wall = [](const P& p) {
                const auto id = map::cells.at(p).rigid->id();

                return
                (id == FeatureId::wall) ||
                (id == FeatureId::rubble_high);
        };

        // Check that it still makes sense to run the crumbling
        auto has_only_walls = [is_wall](const std::vector<P>& positions) {
                for (const P& p : positions)
                {
                        if (!is_wall(p))
                        {
                                return false;
                        }
                }

                return true;
        };

        const bool edge_ok = has_only_walls(wall_cells_);
        const bool inner_ok = has_only_walls(inner_cells_);

        if (!edge_ok || !inner_ok)
        {
                // This area is (no longer) covered by walls (perhaps walls have
                // been destroyed by an explosion for example), remove this
                // crumble event
                game_time::erase_mob(this, true);

                return;
        }

        const bool event_is_on_wall = is_wall(pos_);

        ASSERT(event_is_on_wall);

        // Release mode robustness
        if (!event_is_on_wall)
        {
                game_time::erase_mob(this, true);

                return;
        }

        const bool event_is_on_edge =
                std::find(begin(wall_cells_), end(wall_cells_), pos_) !=
                end(wall_cells_);

        ASSERT(event_is_on_edge);

        // Release mode robustness
        if (!event_is_on_edge)
        {
                game_time::erase_mob(this, true);

                return;
        }

        // OK, everything seems to be in a good state, go!

        if (map::player->properties().allow_see())
        {
                msg_log::add("Suddenly, the walls collapse!",
                             colors::msg_note(),
                             false,
                             MorePromptOnMsg::yes);
        }

        bool should_make_dark = false;

        // Check if any cell adjacent to the destroyed walls is dark
        for (const P& p : wall_cells_)
        {
                for (const P& d : dir_utils::dir_list_w_center)
                {
                        const P p_adj(p + d);

                        if (!map::is_pos_inside_outer_walls(p_adj))
                        {
                                continue;
                        }

                        if (map::dark.at(p_adj))
                        {
                                should_make_dark = true;

                                break;
                        }
                }

                if (should_make_dark)
                {
                        break;
                }
        }

        // Destroy the outer walls
        for (const P& p : wall_cells_)
        {
                if (!map::is_pos_inside_outer_walls(p))
                {
                        continue;
                }

                if (should_make_dark)
                {
                        map::dark.at(p) = true;
                }

                auto* const f = map::cells.at(p).rigid;

                f->hit(1, // Doesn't matter
                       DmgType::physical,
                       DmgMethod::forced,
                       nullptr);
        }

        // Destroy the inner walls
        for (const P& p : inner_cells_)
        {
                if (should_make_dark)
                {
                        map::dark.at(p) = true;
                }

                Rigid* const f = map::cells.at(p).rigid;

                f->hit(1, // Doesn't matter
                       DmgType::physical,
                       DmgMethod::forced,
                       nullptr);

                if (rnd::one_in(8))
                {
                        map::make_gore(p);
                        map::make_blood(p);
                }
        }

        // Spawn monsters

        // Actor id, and corresponding maximum number of monsters allowed
        std::vector< std::pair<ActorId, size_t> > spawn_bucket;

        if (map::dlvl <= dlvl_last_early_game)
        {
                spawn_bucket.push_back({ActorId::rat, 24});
                spawn_bucket.push_back({ActorId::rat_thing, 16});
        }

        spawn_bucket.push_back({ActorId::zombie, 4});
        spawn_bucket.push_back({ActorId::bloated_zombie, 1});

        const auto spawn_data = rnd::element(spawn_bucket);

        const auto actor_id = spawn_data.first;

        const auto nr_mon_limit_except_adj_to_entry = spawn_data.second;

        rnd::shuffle(inner_cells_);

        std::vector<Mon*> mon_spawned;

        for (const P& p : inner_cells_)
        {
                if ((mon_spawned.size() <  nr_mon_limit_except_adj_to_entry) ||
                    is_pos_adj(p, pos_, false))
                {
                        Actor* const actor = actor_factory::make(actor_id, p);

                        Mon* const mon = static_cast<Mon*>(actor);

                        mon_spawned.push_back(mon);
                }
        }

        map::update_vision();

        // Make the monsters aware of the player
        for (auto* const mon : mon_spawned)
        {
                mon->become_aware_player(false);

        }

        map::player->incr_shock(ShockLvl::terrifying,
                                ShockSrc::see_mon);

        game_time::erase_mob(this, true);
}

// -----------------------------------------------------------------------------
// Snake emerge
// -----------------------------------------------------------------------------
EventSnakeEmerge::EventSnakeEmerge() :
        Event (P(-1, -1)) {}

bool EventSnakeEmerge::try_find_p()
{
        const auto blocked = blocked_cells(map::rect());

        auto p_bucket = to_vec(blocked, false, blocked.rect());

        if (p_bucket.empty())
        {
                return false;
        }

        rnd::shuffle(p_bucket);

        for (const P& p : p_bucket)
        {
                const R r = allowed_emerge_rect(p);

                const auto emerge_bucket = emerge_p_bucket(p, blocked, r);

                if (emerge_bucket.size() >= min_nr_snakes_)
                {
                        pos_ = p;

                        return true;
                }
        }

        return false;
}

R EventSnakeEmerge::allowed_emerge_rect(const P& p) const
{
        const int max_d = allowed_emerge_dist_range.max;

        const int x0 = std::max(1, p.x - max_d);
        const int y0 = std::max(1, p.y - max_d);
        const int x1 = std::min(map::w() - 2,  p.x + max_d);
        const int y1 = std::min(map::h() - 2,  p.y + max_d);

        return R(x0, y0, x1, y1);
}

bool EventSnakeEmerge::is_ok_feature_at(const P& p) const
{
        ASSERT(map::is_pos_inside_map(p));

        const FeatureId id = map::cells.at(p).rigid->id();

        return id == FeatureId::floor ||
                id == FeatureId::rubble_low;
}

std::vector<P> EventSnakeEmerge::emerge_p_bucket(
        const P& p,
        const Array2<bool>& blocked,
        const R& allowed_area) const
{
        if (!allowed_area.is_pos_inside(p))
        {
                ASSERT(false);

                return {};
        }

        const auto fov = fov::run(p, blocked);

        std::vector<P> result;

        result.reserve(allowed_area.w() * allowed_area.h());

        for (int x = allowed_area.p0.x; x <= allowed_area.p1.x; ++x)
        {
                for (int y = allowed_area.p0.y; y <= allowed_area.p1.y; ++y)
                {
                        const P tgt_p(x, y);

                        const int min_d = allowed_emerge_dist_range.min;

                        if (!blocked.at(x, y) &&
                            !fov.at(x, y).is_blocked_hard &&
                            king_dist(p, tgt_p) >= min_d)
                        {
                                result.push_back(tgt_p);
                        }
                }
        }

        return result;
}

Array2<bool> EventSnakeEmerge::blocked_cells(const R& r) const
{
        Array2<bool> result(map::dims());

        for (int x = r.p0.x; x <= r.p1.x; ++x)
        {
                for (int y = r.p0.y; y <= r.p1.y; ++y)
                {
                        const P p(x, y);

                        result.at(p) = !is_ok_feature_at(p);
                }
        }

        for (Actor* const actor : game_time::actors)
        {
                const P& p = actor->pos;

                result.at(p) = true;
        }

        return result;
}

void EventSnakeEmerge::on_new_turn()
{
        if (map::player->pos != pos_)
        {
                return;
        }

        const R r = allowed_emerge_rect(pos_);

        const auto blocked = blocked_cells(r);

        auto tgt_bucket = emerge_p_bucket(pos_, blocked, r);

        if (tgt_bucket.size() < min_nr_snakes_)
        {
                // Not possible to spawn at least minimum number
                return;
        }

        int max_nr_snakes = min_nr_snakes_ + (map::dlvl / 4);

        // Cap max number of snakes to the size of the target bucket

        // NOTE: The target bucket is at least as big as the minimum number
        max_nr_snakes = std::min(max_nr_snakes, int(tgt_bucket.size()));

        rnd::shuffle(tgt_bucket);

        std::vector<ActorId> id_bucket;

        for (ActorData d : actor_data::data)
        {
                if (d.is_snake)
                {
                        id_bucket.push_back(d.id);
                }
        }

        const size_t idx = rnd::range(0, id_bucket.size() - 1);

        const ActorId id = id_bucket[idx];

        const size_t nr_summoned = rnd::range(min_nr_snakes_, max_nr_snakes);

        std::vector<P> seen_tgt_positions;

        for (size_t i = 0; i < nr_summoned; ++i)
        {
                ASSERT(i < tgt_bucket.size());

                const P& p(tgt_bucket[i]);

                if (map::cells.at(p).is_seen_by_player)
                {
                        seen_tgt_positions.push_back(p);
                }
        }

        if (!seen_tgt_positions.empty())
        {
                msg_log::add(
                        "Suddenly, vicious snakes slither up from cracks in "
                        "the floor!",
                        colors::msg_note(),
                        true,
                        MorePromptOnMsg::yes);

                io::draw_blast_at_cells(seen_tgt_positions, colors::magenta());

                ShockLvl shock_lvl = ShockLvl::unsettling;

                if (insanity::has_sympt(InsSymptId::phobia_reptile_and_amph))
                {
                        shock_lvl = ShockLvl::terrifying;
                }

                map::player->incr_shock(shock_lvl,
                                        ShockSrc::see_mon);
        }

        for (size_t i = 0; i < nr_summoned; ++i)
        {
                const P& p(tgt_bucket[i]);

                Actor* const actor = actor_factory::make(id, p);

                auto prop = new PropWaiting();

                prop->set_duration(2);

                actor->apply_prop(prop);

                static_cast<Mon*>(actor)->become_aware_player(false);
        }

        game_time::erase_mob(this, true);
}

// -----------------------------------------------------------------------------
// Rats in the walls discovery
// -----------------------------------------------------------------------------
EventRatsInTheWallsDiscovery::EventRatsInTheWallsDiscovery(
        const P& feature_pos) :
        Event(feature_pos) {}

void EventRatsInTheWallsDiscovery::on_new_turn()
{
        // Run the event if player is at the event position or to the right of
        // it. If it's the latter case, it means the player somehow bypassed it
        // (e.g. teleport or dynamite), it should not be possible to use this as
        // a "cheat" to avoid the shock.
        if ((map::player->pos == pos_) ||
            (map::player->pos.x > pos_.x))
        {
                map::update_vision();

                const std::string str =
                        "Before me lies a twilit grotto of enormous height. "
                        "An insane tangle of human bones extends for yards "
                        "like a foamy sea - invariably in postures of demoniac "
                        "frenzy, either fighting off some menace or clutching "
                        "other forms with cannibal intent.";

                popup::msg(str, "A gruesome discovery...");

                map::player->incr_shock(
                        ShockLvl::mind_shattering,
                        ShockSrc::misc);

                for (Actor* const actor : game_time::actors)
                {
                        if (!actor->is_player())
                        {
                                static_cast<Mon*>(actor)->is_roaming_allowed_ =
                                        MonRoamingAllowed::yes;
                        }
                }

                game_time::erase_mob(this, true);
        }
}
