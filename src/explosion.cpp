// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "explosion.hpp"

#include "actor_hit.hpp"
#include "actor_player.hpp"
#include "feature_mob.hpp"
#include "feature_rigid.hpp"
#include "game.hpp"
#include "io.hpp"
#include "line_calc.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "property.hpp"
#include "property_factory.hpp"
#include "sdl_base.hpp"
#include "viewport.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector< std::vector<P> > cells_reached(
        const R& area,
        const P& origin,
        const ExplExclCenter exclude_center,
        const Array2<bool>& blocked)
{
        std::vector< std::vector<P> > out;

        for (int y = area.p0.y; y <= area.p1.y; ++y)
        {
                for (int x = area.p0.x; x <= area.p1.x; ++x)
                {
                        const P pos(x, y);

                        if (exclude_center == ExplExclCenter::yes &&
                            pos == origin)
                        {
                                continue;
                        }

                        const int dist = king_dist(pos, origin);

                        bool is_reached = true;

                        if (dist > 1)
                        {
                                const auto path =
                                        line_calc::calc_new_line(
                                                origin,
                                                pos,
                                                true,
                                                999,
                                                false);

                                for (const P& path_pos : path)
                                {
                                        if (blocked.at(path_pos))
                                        {
                                                is_reached = false;
                                                break;
                                        }
                                }
                        }

                        if (is_reached)
                        {
                                if ((int)out.size() <= dist)
                                {
                                        out.resize(dist + 1);
                                }

                                out[dist].push_back(pos);
                        }
                }
        }

        return out;
}

static void draw(const std::vector< std::vector<P> >& pos_lists,
                 const Array2<bool>& blocked,
                 const Color color_override)
{
        states::draw();

        const Color& color_inner =
                color_override.is_defined() ?
                color_override :
                colors::yellow();

        const Color& color_outer =
                color_override.is_defined() ?
                color_override :
                colors::light_red();

        const bool is_tiles = config::is_tiles_mode();

        const int nr_anim_steps = is_tiles ? 2 : 1;

        bool is_any_cell_seen_by_player = false;

        for (int i_anim = 0; i_anim < nr_anim_steps; i_anim++)
        {
                const TileId tile =
                        (i_anim == 0) ?
                        TileId::blast1 : TileId::blast2;

                const int nr_outer = pos_lists.size();

                for (int i_outer = 0; i_outer < nr_outer; i_outer++)
                {
                        const Color& color =
                                (i_outer == nr_outer - 1) ?
                                color_outer : color_inner;

                        const std::vector<P>& inner = pos_lists[i_outer];

                        for (const P& pos : inner)
                        {
                                const auto& cell = map::g_cells.at(pos);

                                if (cell.is_seen_by_player &&
                                    !blocked.at(pos) &&
                                    viewport::is_in_view(pos))
                                {
                                        is_any_cell_seen_by_player = true;

                                        io::draw_symbol(
                                                tile,
                                                '*',
                                                Panel::map,
                                                viewport::to_view_pos(pos),
                                                color);
                                }
                        }
                }

                if (is_any_cell_seen_by_player)
                {
                        io::update_screen();

                        sdl_base::sleep(
                                config::delay_explosion() / nr_anim_steps);
                }
        }
}

static void apply_explosion_on_pos(
        const P& pos,
        const int current_radi,
        Actor* living_actor,
        std::vector<Actor*> corpses_here)
{
        const int rolls = g_expl_dmg_rolls - current_radi;

        const int dmg =
                rnd::dice(rolls, g_expl_dmg_sides) +
                g_expl_dmg_plus;

        // Damage environment
        Cell& cell = map::g_cells.at(pos);

        cell.rigid->hit(
                dmg,
                DmgType::physical,
                DmgMethod::explosion,
                nullptr);

        // Damage living actor
        if (living_actor)
        {
                if (living_actor->is_player())
                {
                        msg_log::add(
                                "I am hit by an explosion!",
                                colors::msg_bad());
                }

                actor::hit(*living_actor, dmg, DmgType::physical);

                if (living_actor->is_alive() && living_actor->is_player())
                {
                        // Player survived being hit by an explosion, that's
                        // pretty cool!
                        game::add_history_event("Survived an explosion.");
                }
        }

        // Damage dead actors
        for (Actor* corpse : corpses_here)
        {
                actor::hit(*corpse, dmg, DmgType::physical);
        }

        // Add smoke
        if (rnd::fraction(6, 10))
        {
                game_time::add_mob(new Smoke(pos, rnd::range(2, 4)));
        }
}

static void apply_explosion_property_on_pos(
        const P& pos,
        Prop* property,
        Actor* living_actor,
        std::vector<Actor*> corpses_here,
        const ExplIsGas is_gas)
{
        bool should_apply_on_living_actor = true;

        if (living_actor)
        {
                if (is_gas == ExplIsGas::yes)
                {
                        if (living_actor->m_properties.has(PropId::r_breath))
                        {
                                should_apply_on_living_actor = false;
                        }

                        if (living_actor == map::g_player)
                        {
                                // Do not apply effect if wearing Gas Mask, and
                                // this is a gas explosion
                                const Item* const head_item =
                                        map::g_player->m_inv.item_in_slot(
                                                SlotId::head);

                                if ((is_gas == ExplIsGas::yes) &&
                                    head_item &&
                                    (head_item->id() == ItemId::gas_mask))
                                {
                                        should_apply_on_living_actor = false;
                                }
                        }
                }
        }
        else // No living actor here
        {
                should_apply_on_living_actor = false;
        }

        if (should_apply_on_living_actor)
        {
                Prop* const prop_cpy = property_factory::make(property->id());

                prop_cpy->set_duration(property->nr_turns_left());

                living_actor->m_properties.apply(prop_cpy);
        }

        // If property is burning, also apply it to corpses and the
        // environment
        if (property->id() == PropId::burning)
        {
                Cell& cell = map::g_cells.at(pos);

                cell.rigid->hit(1, // Doesn't matter
                                DmgType::fire,
                                DmgMethod::elemental,
                                nullptr);

                for (Actor* corpse : corpses_here)
                {
                        Prop* const prop_cpy =
                                property_factory::make(property->id());

                        prop_cpy->set_duration(property->nr_turns_left());

                        corpse->m_properties.apply(prop_cpy);
                }
        }
}

// -----------------------------------------------------------------------------
// explosion
// -----------------------------------------------------------------------------
namespace explosion
{

void run(const P& origin,
         const ExplType expl_type,
         const EmitExplSnd emit_expl_snd,
         const int radi_change,
         const ExplExclCenter exclude_center,
         std::vector<Prop*> properties_applied,
         const Color color_override,
         const ExplIsGas is_gas)
{
        const int radi = g_expl_std_radi + radi_change;

        const R area = explosion_area(origin, radi);

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksProjectiles().
                run(blocked, area);

        auto pos_lists =
                cells_reached(
                        area,
                        origin,
                        exclude_center,
                        blocked);

        if (emit_expl_snd == EmitExplSnd::yes)
        {
                Snd snd("I hear an explosion!",
                        SfxId::explosion,
                        IgnoreMsgIfOriginSeen::yes,
                        origin,
                        nullptr,
                        SndVol::high,
                        AlertsMon::yes);

                snd_emit::run(snd);
        }

        draw(pos_lists, blocked, color_override);

        // Do damage, apply effect
        Array2<Actor*> living_actors(map::dims());

        Array2< std::vector<Actor*> > corpses(map::dims());

        const size_t len = map::nr_cells();

        for (size_t i = 0; i < len; ++i)
        {
                living_actors.at(i) = nullptr;

                corpses.at(i).clear();
        }

        for (Actor* actor : game_time::g_actors)
        {
                const P& pos = actor->m_pos;

                if (actor->is_alive())
                {
                        living_actors.at(pos) = actor;
                }
                else if (actor->is_corpse())
                {
                        corpses.at(pos).push_back(actor);
                }
        }

        const int nr_outer = pos_lists.size();

        for (int dist = 0; dist < nr_outer; ++dist)
        {
                const std::vector<P>& positions_at_dist = pos_lists[dist];

                for (const P& pos : positions_at_dist)
                {
                        Actor* living_actor = living_actors.at(pos);

                        std::vector<Actor*> corpses_here = corpses.at(pos);

                        if (expl_type == ExplType::expl)
                        {
                                apply_explosion_on_pos(
                                        pos,
                                        dist,
                                        living_actor,
                                        corpses_here);
                        }

                        for (auto* property : properties_applied)
                        {
                                apply_explosion_property_on_pos(
                                        pos,
                                        property,
                                        living_actor,
                                        corpses_here,
                                        is_gas);
                        }
                }
        }

        for (auto* prop : properties_applied)
        {
                delete prop;
        }

        map::update_vision();

} // run

void run_smoke_explosion_at(const P& origin, const int radi_change)
{
        const int radi = g_expl_std_radi + radi_change;

        const R area = explosion_area(origin, radi);

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksProjectiles()
                .run(blocked, area);

        auto pos_lists = cells_reached(
                area,
                origin,
                ExplExclCenter::no,
                blocked);

        // TODO: Sound message?
        Snd snd("",
                SfxId::END,
                IgnoreMsgIfOriginSeen::yes,
                origin,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);

        for (const std::vector<P>& inner : pos_lists)
        {
                for (const P& pos : inner)
                {
                        if (!blocked.at(pos))
                        {
                                game_time::add_mob(
                                        new Smoke(pos, rnd::range(25, 30)));
                        }
                }
        }

        map::update_vision();
}

R explosion_area(const P& c, const int radi)
{
        return R(P(std::max(c.x - radi, 1),
                   std::max(c.y - radi, 1)),
                 P(std::min(c.x + radi, map::w() - 2),
                   std::min(c.y + radi, map::h() - 2)));
}

} // explosion
