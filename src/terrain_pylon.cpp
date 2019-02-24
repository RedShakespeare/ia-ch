// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "terrain_pylon.hpp"

#include "actor.hpp"
#include "flood.hpp"
#include "game_time.hpp"
#include "knockback.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "random.hpp"
#include "sound.hpp"
#include "teleport.hpp"


namespace terrain
{

// -----------------------------------------------------------------------------
// Pylon
// -----------------------------------------------------------------------------
Pylon::Pylon(const P& p, PylonId id) :
        Terrain(p),
        m_pylon_impl(nullptr),
        m_is_activated(false),
        m_nr_turns_active(0)
{
        if (id == PylonId::any)
        {
                if (rnd::coin_toss())
                {
                        id = PylonId::burning;
                }
                else // Pick randomly
                {
                        id = (PylonId)rnd::range(0, (int)PylonId::END - 1);
                }
        }

        m_pylon_impl.reset(make_pylon_impl_from_id(id));
}

PylonImpl* Pylon::make_pylon_impl_from_id(const PylonId id)
{
        switch(id)
        {
        case PylonId::burning:
                return new PylonBurning(m_pos, this);

        case PylonId::invis:
                return new PylonInvis(m_pos, this);

        case PylonId::slow:
                return new PylonSlow(m_pos, this);

        case PylonId::knockback:
                return new PylonKnockback(m_pos, this);

        case PylonId::teleport:
                return new PylonTeleport(m_pos, this);

        case PylonId::terrify:
                return new PylonTerrify(m_pos, this);

        case PylonId::any:
        case PylonId::END:
                break;
        }

        TRACE << "Bad PylonId: " << (int)id << std::endl;

        ASSERT(false);

        return nullptr;
}

std::string Pylon::name(const Article article) const
{
        std::string str =
                ((article == Article::a) ?
                 (m_is_activated ? "an " : "a ") :
                 "the ");

        str +=
                m_is_activated ?
                "activated " :
                "deactivated ";

        str += "Pylon";

        return str;
}

Color Pylon::color_default() const
{
        return
                m_is_activated ?
                colors::light_red() :
                colors::gray();
}

void Pylon::on_hit(const int dmg,
                   const DmgType dmg_type,
                   const DmgMethod dmg_method,
                   actor::Actor* const actor)
{
        (void)dmg;
        (void)dmg_type;
        (void)dmg_method;
        (void)actor;


        // TODO

}

void Pylon::on_new_turn_hook()
{
        if (!m_is_activated)
        {
                return;
        }

        m_pylon_impl->on_new_turn_activated();

        ++m_nr_turns_active;

        // After a being active for a while, deactivate the pylon by
        // toggling the linked lever
        const int max_nr_turns_active = 300;

        if (m_nr_turns_active < max_nr_turns_active)
        {
                return;
        }

        TRACE << "Pylon timed out, deactivating by toggling lever"
              << std::endl;

        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                auto* const terrain = map::g_cells.at(i).terrain;

                if (!terrain || (terrain->id() != terrain::Id::lever))
                {
                        continue;
                }

                auto* const lever =
                        static_cast<Lever*>(terrain);

                if (lever->is_linked_to(*this))
                {
                        lever->toggle();

                        break;
                }
        }
}

void Pylon::on_lever_pulled(Lever* const lever)
{
        (void)lever;

        m_is_activated = !m_is_activated;

        m_nr_turns_active = 0;

        const bool is_seen_by_player =
                map::g_cells.at(m_pos).is_seen_by_player;

        if (m_is_activated)
        {
                std::string msg =
                        is_seen_by_player ?
                        "The pylon makes " :
                        "I hear ";

                msg += "a droning sound.";

                Snd snd(msg,
                        SfxId::END, // TODO: Add a sound effect
                        IgnoreMsgIfOriginSeen::no,
                        m_pos,
                        nullptr,
                        SndVol::low,
                        AlertsMon::no);

                snd.run();
        }
        // Deactivated
        else if (is_seen_by_player)
        {
                msg_log::add("The Pylon shuts down");
        }
}

void Pylon::add_light_hook(Array2<bool>& light) const
{
        if (m_is_activated)
        {
                for (const P& d : dir_utils::g_dir_list_w_center)
                {
                        const P p(m_pos + d);

                        light.at(p) = true;
                }
        }
}

// -----------------------------------------------------------------------------
// Pylon implementation
// -----------------------------------------------------------------------------
// void PylonImpl::emit_trigger_snd() const
// {
//     const std::string msg =
//         map::g_cells.at(m_pos).is_seen_by_player ?
//         "The pylon makes a buzzing sound." :
//         "I hear a buzzing sound.";

//     Snd snd(msg,
//             SfxId::END, // TODO: Add a sound effect
//             IgnoreMsgIfOriginSeen::no,
//             m_pos,
//             nullptr,
//             SndVol::high,
//             AlertsMon::no);

//     snd.run();
// }

std::vector<actor::Actor*> PylonImpl::living_actors_reached() const
{
        std::vector<actor::Actor*> actors;

        for (auto* const actor : game_time::g_actors)
        {
                // Actor is dead?
                if (actor->m_state != ActorState::alive)
                {
                        continue;
                }

                const P& p = actor->m_pos;

                const int d = 1;

                // Actor is out of range?
                if (king_dist(m_pos, p) > d)
                {
                        continue;
                }

                actors.push_back(actor);
        }

        return actors;
}

actor::Actor* PylonImpl::rnd_reached_living_actor() const
{
        auto actors = living_actors_reached();

        if (actors.empty())
        {
                return nullptr;
        }

        auto* actor = rnd::element(living_actors_reached());

        return actor;
}

// -----------------------------------------------------------------------------
// Burning pylon
// -----------------------------------------------------------------------------
void PylonBurning::on_new_turn_activated()
{
        // Do a floodfill from the pylon, and apply burning on each terrain
        // reached.  The flood distance is increased every N turn.
        Array2<bool> blocks_flood(map::dims());

        map_parsers::BlocksProjectiles()
                .run(blocks_flood, blocks_flood.rect());

        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                const auto t = map::g_cells.at(i).terrain;

                if (t->id() == terrain::Id::chasm ||
                    t->id() == terrain::Id::liquid_deep)
                {
                        blocks_flood.at(i) = true;
                }

                // Doors must not block the floodfill, otherwise if a door is
                // opened, the flood can suddenly reach much further and
                // completely surround the player - which is very unfair.  Note
                // however that this could also happen if e.g. a piece of wall
                // is destroyed, then it will seem like the fire is rushing in.
                // It's questionable if this is a good behavior or not, but
                // keeping it like this for now...
                else if (t->id() == terrain::Id::door)
                {
                        blocks_flood.at(i) = false;
                }
        }

        const int nr_turns_active = m_pylon->nr_turns_active();

        const int nr_turns_per_flood_step = 10;

        // NOTE: The distance may also be limited by the number of turns which
        // the Pylon remains active (it shuts itself down after a while)
        const int flood_max_dist = 10;

        const int flood_dist =
                std::min(
                        flood_max_dist,
                        (nr_turns_active / nr_turns_per_flood_step) + 1);

        const auto flood = floodfill(m_pos, blocks_flood, flood_dist);

        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                if (flood.at(i) > 0)
                {
                        map::g_cells.at(i).terrain->hit(
                                1, // Doesn't matter
                                DmgType::fire,
                                DmgMethod::elemental);
                }
        }

        // Occasionally also directly burn adjacent actors
        if (!rnd::fraction(2, 3))
        {
                return;
        }

        // emit_trigger_snd();

        auto actors = living_actors_reached();

        for (auto actor : actors)
        {
                actor->m_properties.apply(new PropBurning());
        }
}

// -----------------------------------------------------------------------------
// Invisibility Pylon
// -----------------------------------------------------------------------------
void PylonInvis::on_new_turn_activated()
{
        // emit_trigger_snd();

        auto actors = living_actors_reached();

        for (auto actor : actors)
        {
                actor->m_properties.apply(new PropInvisible());
        }
}

// -----------------------------------------------------------------------------
// Burning pylon
// -----------------------------------------------------------------------------
void PylonSlow::on_new_turn_activated()
{
        // emit_trigger_snd();

        auto actors = living_actors_reached();

        for (auto actor : actors)
        {
                actor->m_properties.apply(new PropSlowed());
        }
}

// -----------------------------------------------------------------------------
// Knockback pylon
// -----------------------------------------------------------------------------
void PylonKnockback::on_new_turn_activated()
{
        if (!rnd::fraction(2, 3))
        {
                return;
        }

        // emit_trigger_snd();

        auto actors = living_actors_reached();

        for (auto actor : actors)
        {
                knockback::run(
                        *actor,
                        m_pos,
                        false, // Not spike gun
                        Verbosity::verbose,
                        2); // Extra paralyze turns
        }
}

// -----------------------------------------------------------------------------
// Knockback pylon
// -----------------------------------------------------------------------------
void PylonTeleport::on_new_turn_activated()
{
        if (rnd::coin_toss())
        {
                return;
        }

        // emit_trigger_snd();

        auto actors = living_actors_reached();

        for (auto actor : actors)
        {
                teleport(*actor);
        }
}

// -----------------------------------------------------------------------------
// Invisibility Pylon
// -----------------------------------------------------------------------------
void PylonTerrify::on_new_turn_activated()
{
        // emit_trigger_snd();

        auto actors = living_actors_reached();

        for (auto actor : actors)
        {
                actor->m_properties.apply(new PropTerrified());
        }
}

} // terrain
