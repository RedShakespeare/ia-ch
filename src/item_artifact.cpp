// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "init.hpp"

#include "item_artifact.hpp"

#include "actor_factory.hpp"
#include "actor_hit.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "common_text.hpp"
#include "explosion.hpp"
#include "terrain.hpp"
#include "fov.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "saving.hpp"
#include "text_format.hpp"


// -----------------------------------------------------------------------------
// item
// -----------------------------------------------------------------------------
namespace item
{

// -----------------------------------------------------------------------------
// Staff of the pharaohs
// -----------------------------------------------------------------------------
PharaohStaff::PharaohStaff(ItemData* const item_data) :
        Wpn(item_data)
{

}

void PharaohStaff::on_std_turn_in_inv(const InvType inv_type)
{
        (void)inv_type;

        if (actor_carrying() != map::g_player)
        {
                return;
        }

        Array2<bool> blocked_los(map::dims());

        map_parsers::BlocksLos()
                .run(blocked_los,
                     fov::fov_rect(map::g_player->m_pos, map::dims()),
                     MapParseMode::overwrite);

        for (auto* const actor : game_time::g_actors)
        {
                if (actor->is_player() || !actor->is_alive())
                {
                        continue;
                }

                auto* const mon = static_cast<actor::Mon*>(actor);

                if (mon->m_aware_of_player_counter <= 0)
                {
                        continue;
                }

                const bool mon_see_player =
                        mon->can_see_actor(*map::g_player, blocked_los);

                if (!mon_see_player)
                {
                        continue;
                }

                on_mon_see_player_carrying(*mon);
        }
}

void PharaohStaff::on_mon_see_player_carrying(actor::Mon& mon) const
{
        // TODO: Consider an "is_mummy" actor data field
        if (mon.id() != actor::Id::mummy &&
            mon.id() != actor::Id::croc_head_mummy)
        {
                return;
        }

        if (mon.is_actor_my_leader(map::g_player))
        {
                return;
        }

        const int convert_pct_chance = 10;

        if (rnd::percent(convert_pct_chance))
        {
                mon.m_leader = map::g_player;

                if (map::g_player->can_see_actor(mon))
                {
                        const auto name_the =
                                text_format::first_to_upper(
                                        mon.name_the());

                        msg_log::add(name_the + " bows before me.");
                }
        }
}

// -----------------------------------------------------------------------------
// Talisman of Reflection
// -----------------------------------------------------------------------------
ReflTalisman::ReflTalisman(ItemData* const item_data) :
        Item(item_data)
{

}

void ReflTalisman::on_pickup_hook()
{
        auto prop = new PropSpellReflect();

        prop->set_indefinite();

        add_carrier_prop(prop, Verbosity::silent);
}

void ReflTalisman::on_removed_from_inv_hook()
{
        clear_carrier_props();
}

// -----------------------------------------------------------------------------
// Talisman of Resurrection
// -----------------------------------------------------------------------------
ResurrectTalisman::ResurrectTalisman(ItemData* const item_data) :
        Item(item_data)
{

}

// -----------------------------------------------------------------------------
// Talisman of Teleporation Control
// -----------------------------------------------------------------------------
TeleCtrlTalisman::TeleCtrlTalisman(ItemData* const item_data) :
        Item(item_data)
{

}

void TeleCtrlTalisman::on_pickup_hook()
{
        auto prop = new PropTeleControl();

        prop->set_indefinite();

        add_carrier_prop(prop, Verbosity::silent);
}

void TeleCtrlTalisman::on_removed_from_inv_hook()
{
        clear_carrier_props();
}

// -----------------------------------------------------------------------------
// Horn of Malice
// -----------------------------------------------------------------------------
void HornOfMaliceHeard::run(actor::Actor& actor) const
{
        if (!actor.is_player())
        {
                actor.m_properties.apply(new PropConflict());
        }
}

HornOfMalice::HornOfMalice(ItemData* const item_data) :
        Item(item_data),
        m_charges(rnd::range(4, 6))
{

}

std::string HornOfMalice::name_inf_str() const
{
        return "{" + std::to_string(m_charges) + "}";
}

void HornOfMalice::save() const
{
        saving::put_int(m_charges);
}

void HornOfMalice::load()
{
        m_charges = saving::get_int();
}

ConsumeItem HornOfMalice::activate(actor::Actor* const actor)
{
        (void)actor;

        if (m_charges <= 0)
        {
                msg_log::add("It makes no sound.");

                return ConsumeItem::no;
        }

        auto effect = std::shared_ptr<SndHeardEffect>(
                new HornOfMaliceHeard);

        Snd snd("The Horn of Malice resounds!",
                SfxId::END, // TODO: Make a sound effect
                IgnoreMsgIfOriginSeen::no,
                map::g_player->m_pos,
                map::g_player,
                SndVol::high,
                AlertsMon::yes,
                MorePromptOnMsg::no,
                effect);

        snd_emit::run(snd);

        --m_charges;

        game_time::tick();

        return ConsumeItem::no;
}

// -----------------------------------------------------------------------------
// Horn of Banishment
// -----------------------------------------------------------------------------
void HornOfBanishmentHeard::run(actor::Actor& actor) const
{
        if (actor.m_properties.has(PropId::summoned))
        {
                if (map::g_player->can_see_actor(actor))
                {
                        const std::string name_the =
                                text_format::first_to_upper(
                                        actor.name_the());

                        msg_log::add(
                                name_the +
                                " " +
                                common_text::g_mon_disappear);
                }

                actor.m_state = ActorState::destroyed;
        }
}

HornOfBanishment::HornOfBanishment(ItemData* const item_data) :
        Item(item_data),
        m_charges(rnd::range(4, 6))
{

}

std::string HornOfBanishment::name_inf_str() const
{
        return "{" + std::to_string(m_charges) + "}";
}

void HornOfBanishment::save() const
{
        saving::put_int(m_charges);
}

void HornOfBanishment::load()
{
        m_charges = saving::get_int();
}

ConsumeItem HornOfBanishment::activate(actor::Actor* const actor)
{
        (void)actor;

        if (m_charges <= 0)
        {
                msg_log::add("It makes no sound.");

                return ConsumeItem::no;
        }

        auto effect = std::shared_ptr<SndHeardEffect>(
                new HornOfBanishmentHeard);

        Snd snd("The Horn of Banishment resounds!",
                SfxId::END, // TODO: Make a sound effect
                IgnoreMsgIfOriginSeen::no,
                map::g_player->m_pos,
                map::g_player,
                SndVol::high,
                AlertsMon::yes,
                MorePromptOnMsg::no,
                effect);

        snd_emit::run(snd);

        --m_charges;

        game_time::tick();

        return ConsumeItem::no;
}

// -----------------------------------------------------------------------------
// Arcane Clockwork
// -----------------------------------------------------------------------------
Clockwork::Clockwork(ItemData* const item_data) :
        Item(item_data),
        m_charges(rnd::range(4, 6))
{

}

std::string Clockwork::name_inf_str() const
{
        return "{" + std::to_string(m_charges) + "}";
}

void Clockwork::save() const
{
        saving::put_int(m_charges);
}

void Clockwork::load()
{
        m_charges = saving::get_int();
}

ConsumeItem Clockwork::activate(actor::Actor* const actor)
{
        (void)actor;

        if (m_charges <= 0)
        {
                msg_log::add("Nothing happens.");

                return ConsumeItem::no;
        }

        if (map::g_player->m_properties.has(PropId::clockwork_hasted))
        {
                msg_log::add("It will not move.");

                return ConsumeItem::no;
        }

        msg_log::add("I wind up the clockwork.");

        map::g_player->incr_shock(
                ShockLvl::terrifying,
                ShockSrc::use_strange_item);

        if (!map::g_player->is_alive())
        {
                return ConsumeItem::no;
        }

        map::g_player->m_properties.apply(new PropClockworkHasted());

        --m_charges;

        game_time::tick();

        return ConsumeItem::no;
}

// -----------------------------------------------------------------------------
// Spirit Dagger
// -----------------------------------------------------------------------------
SpiritDagger::SpiritDagger(ItemData* const item_data) :
        Wpn(item_data)
{

}

void SpiritDagger::on_melee_hit(actor::Actor& actor_hit, const int dmg)
{
        (void)dmg;

        if (m_actor_carrying->m_sp >= 17)
        {
                const P defender_pos = actor_hit.m_pos;

                const P attacker_pos = m_actor_carrying->m_pos;

                const auto tgt_pos =
                        defender_pos + (defender_pos - attacker_pos);

                const auto* const tgt_f = map::g_cells.at(tgt_pos).terrain;

                if (tgt_f->id() != terrain::Id::chasm &&
                    tgt_f->id() != terrain::Id::liquid_deep)
                {
                        P expl_pos;

                        int expl_d = 0;

                        if (tgt_f->is_projectile_passable())
                        {
                                expl_pos = tgt_pos;
                                expl_d = -1;
                        }
                        else
                        {
                                expl_pos = defender_pos;
                                expl_d = -2;
                        }

                        // TODO: Emit sound from explosion center

                        explosion::run(
                                expl_pos,
                                ExplType::apply_prop,
                                EmitExplSnd::no,
                                expl_d,
                                ExplExclCenter::no,
                                {new PropBurning()});
                }
        }

        actor::hit_sp(*m_actor_carrying, 1, Verbosity::verbose);
}

void SpiritDagger::specific_dmg_mod(
        DmgRange& range,
        const actor::Actor* const actor) const
{
        if (!actor)
        {
                return;
        }

        const auto sp_db = (double)actor->m_sp;

        const double exp = 0.5;

        const auto dmg_plus_db = std::pow(sp_db, exp);

        range.set_plus((int)dmg_plus_db);
}

// -----------------------------------------------------------------------------
// Sorcery Orb
// -----------------------------------------------------------------------------
OrbOfLife::OrbOfLife(ItemData* const item_data) :
        Item(item_data)
{

}

void OrbOfLife::on_pickup_hook()
{
        map::g_player->change_max_hp(4, Verbosity::verbose);

        auto prop_r_poison = new PropRPoison();

        prop_r_poison->set_indefinite();

        add_carrier_prop(prop_r_poison, Verbosity::verbose);

        auto prop_r_disease = new PropRDisease();

        prop_r_disease->set_indefinite();

        add_carrier_prop(prop_r_disease, Verbosity::verbose);
}

void OrbOfLife::on_removed_from_inv_hook()
{
        map::g_player->change_max_hp(-4, Verbosity::verbose);

        clear_carrier_props();
}

} // item
