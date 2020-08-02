// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "init.hpp"

#include "item_artifact.hpp"

#include "actor_factory.hpp"
#include "actor_hit.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "actor_see.hpp"
#include "common_text.hpp"
#include "explosion.hpp"
#include "fov.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "saving.hpp"
#include "terrain.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// item
// -----------------------------------------------------------------------------
namespace item {

// -----------------------------------------------------------------------------
// Staff of the pharaohs
// -----------------------------------------------------------------------------
PharaohStaff::PharaohStaff(ItemData* const item_data) :
        Wpn(item_data)
{
}

void PharaohStaff::on_std_turn_in_inv_hook(const InvType inv_type)
{
        (void)inv_type;

        if (actor_carrying() != map::g_player) {
                return;
        }

        Array2<bool> blocked_los(map::dims());

        map_parsers::BlocksLos()
                .run(blocked_los,
                     fov::fov_rect(map::g_player->m_pos, map::dims()),
                     MapParseMode::overwrite);

        for (auto* const actor : game_time::g_actors) {
                if (actor->is_player() || !actor->is_alive()) {
                        continue;
                }

                if (!actor->is_aware_of_player()) {
                        continue;
                }

                const bool mon_see_player =
                        actor::can_mon_see_actor(
                                *actor,
                                *map::g_player,
                                blocked_los);

                if (!mon_see_player) {
                        continue;
                }

                on_mon_see_player_carrying(*actor);
        }
}

void PharaohStaff::on_mon_see_player_carrying(actor::Actor& mon) const
{
        // TODO: Consider an "is_mummy" actor data field
        if ((mon.id() != actor::Id::mummy) &&
            (mon.id() != actor::Id::croc_head_mummy)) {
                return;
        }

        if (mon.is_actor_my_leader(map::g_player)) {
                return;
        }

        const int convert_pct_chance = 10;

        if (rnd::percent(convert_pct_chance)) {
                mon.m_leader = map::g_player;

                if (actor::can_player_see_actor(mon)) {
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
        auto prop = property_factory::make(PropId::spell_reflect);

        prop->set_indefinite();

        add_carrier_prop(prop, Verbose::no);
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
        auto prop = property_factory::make(PropId::tele_ctrl);

        prop->set_indefinite();

        add_carrier_prop(prop, Verbose::no);
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
        if (!actor.is_player()) {
                actor.m_properties.apply(
                        property_factory::make(PropId::conflict));
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

void HornOfMalice::save_hook() const
{
        saving::put_int(m_charges);
}

void HornOfMalice::load_hook()
{
        m_charges = saving::get_int();
}

ConsumeItem HornOfMalice::activate(actor::Actor* const actor)
{
        (void)actor;

        if (m_charges <= 0) {
                msg_log::add("It makes no sound.");

                return ConsumeItem::no;
        }

        auto effect = std::shared_ptr<SndHeardEffect>(
                new HornOfMaliceHeard);

        Snd snd(
                "The Horn of Malice resounds!",
                audio::SfxId::END, // TODO: Make a sound effect
                IgnoreMsgIfOriginSeen::no,
                map::g_player->m_pos,
                map::g_player,
                SndVol::high,
                AlertsMon::yes,
                MorePromptOnMsg::no,
                effect);

        snd.run();

        --m_charges;

        game_time::tick();

        return ConsumeItem::no;
}

// -----------------------------------------------------------------------------
// Horn of Banishment
// -----------------------------------------------------------------------------
void HornOfBanishmentHeard::run(actor::Actor& actor) const
{
        if (actor.m_properties.has(PropId::summoned)) {
                if (actor::can_player_see_actor(actor)) {
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

void HornOfBanishment::save_hook() const
{
        saving::put_int(m_charges);
}

void HornOfBanishment::load_hook()
{
        m_charges = saving::get_int();
}

ConsumeItem HornOfBanishment::activate(actor::Actor* const actor)
{
        (void)actor;

        if (m_charges <= 0) {
                msg_log::add("It makes no sound.");

                return ConsumeItem::no;
        }

        auto effect = std::shared_ptr<SndHeardEffect>(
                new HornOfBanishmentHeard);

        Snd snd(
                "The Horn of Banishment resounds!",
                audio::SfxId::END, // TODO: Make a sound effect
                IgnoreMsgIfOriginSeen::no,
                map::g_player->m_pos,
                map::g_player,
                SndVol::high,
                AlertsMon::yes,
                MorePromptOnMsg::no,
                effect);

        snd.run();

        --m_charges;

        game_time::tick();

        return ConsumeItem::no;
}

// -----------------------------------------------------------------------------
// Holy Symbol
// -----------------------------------------------------------------------------
HolySymbol::HolySymbol(ItemData* item_data) :
        Item(item_data)
{
}

ConsumeItem HolySymbol::activate(actor::Actor* actor)
{
        (void)actor;

        if (!map::g_player->m_properties.allow_pray(Verbose::yes)) {
                return ConsumeItem::no;
        }

        if (m_has_failed_attempt) {
                msg_log::add(
                        "I have no faith that this would help me at "
                        "the moment.");

                return ConsumeItem::no;
        }

        // Is not in failed state

        const std::string my_name =
                name(
                        ItemRefType::plain,
                        ItemRefInf::none);

        std::string pray_msg = "";

        if (map::g_player->m_properties.has(PropId::terrified)) {
                pray_msg = "With trembling hands ";
        }

        pray_msg += "I make a prayer over the " + my_name + "...";

        msg_log::add(
                pray_msg,
                colors::text(),
                MsgInterruptPlayer::no,
                MorePromptOnMsg::yes);

        // If the item is still charging, roll for success
        if ((m_nr_charge_turns_left > 0) && !rnd::percent(25)) {
                // Failed!
                m_has_failed_attempt = true;

                // Set a recharge duration that is longer than normal
                m_nr_charge_turns_left = nr_turns_to_recharge().max;

                const int duration_pct = rnd::range(175, 200);

                m_nr_charge_turns_left =
                        (m_nr_charge_turns_left * duration_pct) / 100;

                msg_log::add("This feels useless!");

                map::g_player->incr_shock(
                        ShockLvl::frightening,
                        ShockSrc::misc);

                game_time::tick();

                return ConsumeItem::no;
        }

        // Item can be used

        m_nr_charge_turns_left = nr_turns_to_recharge().roll();

        run_effect();

        game_time::tick();

        return ConsumeItem::no;
}

void HolySymbol::on_std_turn_in_inv_hook(InvType inv_type)
{
        (void)inv_type;

        // Already fully charged?
        if (m_nr_charge_turns_left == 0) {
                return;
        }

        ASSERT(m_nr_charge_turns_left > 0);

        --m_nr_charge_turns_left;

        if (m_nr_charge_turns_left == 0) {
                m_has_failed_attempt = false;

                const std::string my_name =
                        name(
                                ItemRefType::plain,
                                ItemRefInf::none);

                msg_log::add(
                        "I feel like praying over the " +
                        my_name +
                        " would be beneficent again.");
        }
}

std::string HolySymbol::name_inf_str() const
{
        if (m_nr_charge_turns_left <= 0) {
                return "";
        }

        const auto turns_left_str = std::to_string(m_nr_charge_turns_left);

        std::string str = "{" + turns_left_str;

        if (m_has_failed_attempt) {
                str += ", failed";
        }

        str += "}";

        return str;
}

void HolySymbol::save_hook() const
{
        saving::put_int(m_nr_charge_turns_left);
        saving::put_bool(m_has_failed_attempt);
}

void HolySymbol::load_hook()
{
        m_nr_charge_turns_left = saving::get_int();
        m_has_failed_attempt = saving::get_bool();
}

void HolySymbol::run_effect()
{
        map::g_player->restore_sp(rnd::range(1, 4));

        const int prop_duration = rnd::range(6, 12);

        auto r_fear = property_factory::make(PropId::r_fear);
        auto r_shock = property_factory::make(PropId::r_shock);

        r_fear->set_duration(prop_duration);
        r_shock->set_duration(prop_duration);

        map::g_player->m_properties.apply(r_fear);
        map::g_player->m_properties.apply(r_shock);
}

Range HolySymbol::nr_turns_to_recharge() const
{
        return {125, 175};
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

void Clockwork::save_hook() const
{
        saving::put_int(m_charges);
}

void Clockwork::load_hook()
{
        m_charges = saving::get_int();
}

ConsumeItem Clockwork::activate(actor::Actor* const actor)
{
        (void)actor;

        if (m_charges <= 0) {
                msg_log::add("Nothing happens.");

                return ConsumeItem::no;
        }

        if (map::g_player->m_properties.has(PropId::clockwork_hasted)) {
                msg_log::add("It will not move.");

                return ConsumeItem::no;
        }

        msg_log::add("I wind up the clockwork.");

        map::g_player->incr_shock(
                ShockLvl::terrifying,
                ShockSrc::use_strange_item);

        if (!map::g_player->is_alive()) {
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

void SpiritDagger::specific_dmg_mod(
        DmgRange& range,
        const actor::Actor* const actor) const
{
        if (!actor) {
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
        map::g_player->change_max_hp(4, Verbose::yes);

        auto prop_r_poison = new PropRPoison();

        prop_r_poison->set_indefinite();

        add_carrier_prop(prop_r_poison, Verbose::yes);

        auto prop_r_disease = new PropRDisease();

        prop_r_disease->set_indefinite();

        add_carrier_prop(prop_r_disease, Verbose::yes);
}

void OrbOfLife::on_removed_from_inv_hook()
{
        map::g_player->change_max_hp(-4, Verbose::yes);

        clear_carrier_props();
}

} // namespace item
