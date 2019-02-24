// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "terrain_monolith.hpp"

#include "msg_log.hpp"
#include "game.hpp"
#include "map.hpp"
#include "actor.hpp"
#include "actor_mon.hpp"
#include "actor_factory.hpp"
#include "actor_player.hpp"
#include "property_handler.hpp"


namespace terrain
{

Monolith::Monolith(const P& p) :
        Terrain(p),
        m_is_activated(false) {}

void Monolith::on_hit(
        const int dmg,
        const DmgType dmg_type,
        const DmgMethod dmg_method,
        actor::Actor* const actor)
{
        (void)dmg;
        (void)dmg_type;
        (void)dmg_method;
        (void)actor;
}

std::string Monolith::name(const Article article) const
{
        std::string ret =
                article == Article::a
                ? "a "
                : "the ";

        return ret + "carved monolith";
}

Color Monolith::color_default() const
{
        return
                m_is_activated
                ? colors::gray()
                : colors::light_cyan();
}

void Monolith::bump(actor::Actor& actor_bumping)
{
        if (!actor_bumping.is_player())
        {
                return;
        }

        if (!map::g_player->m_properties.allow_see())
        {
                msg_log::add("There is a carved rock here.");

                return;
        }

        msg_log::add("I recite the inscriptions on the Monolith...");

        if (m_is_activated)
        {
                msg_log::add("Nothing happens.");
        }
        else // Not yet activated
        {
                activate();
        }
}

void Monolith::activate()
{
        msg_log::add("I feel powerful!");

        audio::play(SfxId::monolith);

        game::incr_player_xp(20);

        m_is_activated = true;

        map::g_player->incr_shock(
                ShockLvl::terrifying,
                ShockSrc::misc);
}

} // terrain
