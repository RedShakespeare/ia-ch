// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "init.hpp"

#include <algorithm>
#include <vector>

#include "actor.hpp"
#include "actor_player.hpp"
#include "explosion.hpp"
#include "terrain_mob.hpp"
#include "terrain.hpp"
#include "fov.hpp"
#include "game_time.hpp"
#include "inventory.hpp"
#include "item.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "property.hpp"
#include "property_handler.hpp"


namespace terrain
{

// -----------------------------------------------------------------------------
// Smoke
// -----------------------------------------------------------------------------
void Smoke::on_placed()
{
        // Remove any other smoke in the same position (this is so that for
        // example the gas mask is not drained much faster if there is multiple
        // smoke objects stacked on the same position)
        for (auto it = std::begin(game_time::g_mobs);
             it != std::end(game_time::g_mobs); )
        {
                const auto terrain = static_cast<const Terrain*>(*it);

                if ((terrain == this) ||
                    (terrain->id() != Id::smoke) ||
                    (terrain->pos() != m_pos))
                {
                        ++it;

                        continue;
                }

                // This is another smoke object in the same position

                const auto other_smoke = static_cast<const Smoke*>(terrain);

                // Use the longest duration of the new or old smoke
                m_nr_turns_left = std::max(
                        m_nr_turns_left,
                        other_smoke->m_nr_turns_left);

                delete other_smoke;

                game_time::g_mobs.erase(it);
        }
}

void Smoke::on_new_turn()
{
        auto* actor = map::actor_at_pos(m_pos);

        if (actor)
        {
                const bool is_player = actor == map::g_player;

                // TODO: There needs to be some criteria here, so that e.g. a
                // statue-monster or a very alien monster can't get blinded by
                // smoke (but do not use is_humanoid - rats, wolves etc should
                // definitely be blinded by smoke).

                // Perhaps add some variable like "has_eyes"?

                bool is_blind_prot = false;

                bool is_breath_prot = actor->m_properties.has(PropId::r_breath);

                if (is_player)
                {
                        auto* const player_head_item =
                                map::g_player->m_inv
                                .m_slots[(size_t)SlotId::head].item;

                        auto* const player_body_item =
                                map::g_player->m_inv
                                .m_slots[(size_t)SlotId::body].item;

                        if (player_head_item &&
                            (player_head_item->data().id == item::Id::gas_mask))
                        {
                                is_blind_prot = true;

                                is_breath_prot = true;

                                // This may destroy the gasmask
                                static_cast<item::GasMask*>(player_head_item)
                                        ->decr_turns_left(map::g_player->m_inv);
                        }

                        if (player_body_item &&
                            (player_body_item->data().id ==
                             item::Id::armor_asb_suit))
                        {
                                is_blind_prot = true;

                                is_breath_prot = true;
                        }
                }

                // Blinded?
                if (!is_blind_prot && rnd::one_in(4))
                {
                        if (is_player)
                        {
                                msg_log::add("I am getting smoke in my eyes.");
                        }

                        auto prop = new PropBlind();

                        prop->set_duration(rnd::range(1, 3));

                        actor->m_properties.apply(prop);
                }

                // Coughing?
                if (!is_breath_prot &&
                    rnd::one_in(4))
                {
                        std::string snd_msg = "";

                        if (is_player)
                        {
                                msg_log::add("I cough.");
                        }
                        else // Is monster
                        {
                                if (actor->m_data->is_humanoid)
                                {
                                        snd_msg = "I hear coughing.";
                                }
                        }

                        const auto alerts =
                                is_player
                                ? AlertsMon::yes
                                : AlertsMon::no;

                        Snd snd(
                                snd_msg,
                                SfxId::END,
                                IgnoreMsgIfOriginSeen::yes,
                                actor->m_pos,
                                actor,
                                SndVol::low,
                                alerts);

                        snd.run();
                }
        }

        // If not permanent, count down turns left and possibly erase self
        if (m_nr_turns_left > -1)
        {
                --m_nr_turns_left;

                if (m_nr_turns_left <= 0)
                {
                        game_time::erase_mob(this, true);
                }
        }
}

std::string Smoke::name(const Article article)  const
{
        std::string name = "";

        if (article == Article::the)
        {
                name = "the ";
        }

        return name + "smoke";
}

Color Smoke::color() const
{
        return colors::gray();
}

// -----------------------------------------------------------------------------
// Force Field
// -----------------------------------------------------------------------------
void ForceField::on_new_turn()
{
        // If not permanent, count down turns left and possibly erase self
        if (m_nr_turns_left <= -1)
        {
                return;
        }

        --m_nr_turns_left;

        if (m_nr_turns_left <= 0)
        {
                game_time::erase_mob(this, true);
        }
}

std::string ForceField::name(const Article article)  const
{
        std::string name =
                (article == Article::a)
                ? "a"
                : "the";

        name += " force field";

        return name;
}

Color ForceField::color() const
{
        return colors::orange();
}

// -----------------------------------------------------------------------------
// Dynamite
// -----------------------------------------------------------------------------
void LitDynamite::on_new_turn()
{
        --m_nr_turns_left;

        if (m_nr_turns_left <= 0)
        {
                const P p(m_pos);

                // Removing the dynamite before the explosion, so it can't be
                // rendered after the explosion (e.g. if there are "more"
                // prompts).
                game_time::erase_mob(this, true);

                // NOTE: This object is now deleted

                explosion::run(p,
                               ExplType::expl,
                               EmitExplSnd::yes);
        }
}

std::string LitDynamite::name(const Article article)  const
{
        std::string name =
                (article == Article::a)
                ? "a"
                : "the";

        return name + " lit stick of dynamite";
}

Color LitDynamite::color() const
{
        return colors::light_red();
}

// -----------------------------------------------------------------------------
// Flare
// -----------------------------------------------------------------------------
void LitFlare::on_new_turn()
{
        --m_nr_turns_left;

        if (m_nr_turns_left <= 0)
        {
                game_time::erase_mob(this, true);
        }
}

void LitFlare::add_light(Array2<bool>& light) const
{
        const int radi = g_fov_radi_int;

        P p0(std::max(0, m_pos.x - radi),
             std::max(0, m_pos.y - radi));

        P p1(std::min(map::w() - 1, m_pos.x + radi),
             std::min(map::h() - 1, m_pos.y + radi));

        Array2<bool> hard_blocked(map::dims());

        map_parsers::BlocksLos()
                .run(hard_blocked,
                     R(p0, p1),
                     MapParseMode::overwrite);

        FovMap fov_map;
        fov_map.hard_blocked = &hard_blocked;
        fov_map.light = &map::g_light;
        fov_map.dark = &map::g_dark;

        const auto light_fov = fov::run(m_pos, fov_map);

        for (int y = p0.y; y <= p1.y; ++y)
        {
                for (int x = p0.x; x <= p1.x; ++x)
                {
                        if (!light_fov.at(x, y).is_blocked_hard)
                        {
                                light.at(x, y) = true;
                        }
                }
        }
}

std::string LitFlare::name(const Article article)  const
{
        std::string name =
                (article == Article::a)
                ? "a"
                : "the";

        name += " lit flare";

        return name;
}

Color LitFlare::color() const
{
        return colors::yellow();
}

} // terrain
