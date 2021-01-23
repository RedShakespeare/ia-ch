// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "item_rod.hpp"

#include "actor_hit.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "actor_see.hpp"
#include "fov.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "io.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "knockback.hpp"
#include "map.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "saving.hpp"
#include "spells.hpp"
#include "terrain.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
std::vector<rod::RodLook> s_rod_looks;

// -----------------------------------------------------------------------------
// rod
// -----------------------------------------------------------------------------
namespace rod
{
void init()
{
        TRACE_FUNC_BEGIN;

        // Init possible rod colors and fake names
        s_rod_looks.clear();

        s_rod_looks.push_back(
                {"Iron", "an Iron", colors::gray()});

        s_rod_looks.push_back(
                {"Zinc", "a Zinc", colors::light_white()});

        s_rod_looks.push_back(
                {"Chromium", "a Chromium", colors::light_white()});

        s_rod_looks.push_back(
                {"Tin", "a Tin", colors::light_white()});

        s_rod_looks.push_back(
                {"Silver", "a Silver", colors::light_white()});

        s_rod_looks.push_back(
                {"Golden", "a Golden", colors::yellow()});

        s_rod_looks.push_back(
                {"Nickel", "a Nickel", colors::light_white()});

        s_rod_looks.push_back(
                {"Copper", "a Copper", colors::brown()});

        s_rod_looks.push_back(
                {"Lead", "a Lead", colors::gray()});

        s_rod_looks.push_back(
                {"Tungsten", "a Tungsten", colors::white()});

        s_rod_looks.push_back(
                {"Platinum", "a Platinum", colors::light_white()});

        s_rod_looks.push_back(
                {"Lithium", "a Lithium", colors::white()});

        s_rod_looks.push_back(
                {"Zirconium", "a Zirconium", colors::white()});

        s_rod_looks.push_back(
                {"Gallium", "a Gallium", colors::light_white()});

        s_rod_looks.push_back(
                {"Cobalt", "a Cobalt", colors::light_blue()});

        s_rod_looks.push_back(
                {"Titanium", "a Titanium", colors::light_white()});

        s_rod_looks.push_back(
                {"Magnesium", "a Magnesium", colors::white()});

        for (auto& d : item::g_data)
        {
                if (d.type == ItemType::rod)
                {
                        // Color and false name
                        const size_t idx =
                                rnd::range(0, (int)s_rod_looks.size() - 1);

                        RodLook& look = s_rod_looks[idx];

                        d.base_name_un_id.names[(size_t)ItemRefType::plain] =
                                look.name_plain + " Rod";

                        d.base_name_un_id.names[(size_t)ItemRefType::plural] =
                                look.name_plain + " Rods";

                        d.base_name_un_id.names[(size_t)ItemRefType::a] =
                                look.name_a + " Rod";

                        d.color = look.color;

                        s_rod_looks.erase(s_rod_looks.begin() + idx);

                        // True name
                        const auto* const rod =
                                static_cast<const Rod*>(
                                        item::make(d.id, 1));

                        const std::string real_type_name = rod->real_name();

                        delete rod;

                        const std::string real_name =
                                "Rod of " + real_type_name;

                        const std::string real_name_plural =
                                "Rods of " + real_type_name;

                        const std::string real_name_a =
                                "a Rod of " + real_type_name;

                        d.base_name.names[(size_t)ItemRefType::plain] =
                                real_name;

                        d.base_name.names[(size_t)ItemRefType::plural] =
                                real_name_plural;

                        d.base_name.names[(size_t)ItemRefType::a] =
                                real_name_a;
                }
        }

        TRACE_FUNC_END;
}

void save()
{
        for (int i = 0; i < (int)item::Id::END; ++i)
        {
                auto& d = item::g_data[i];

                if (d.type == ItemType::rod)
                {
                        saving::put_str(
                                d.base_name_un_id
                                        .names[(size_t)ItemRefType::plain]);

                        saving::put_str(
                                d.base_name_un_id
                                        .names[(size_t)ItemRefType::plural]);

                        saving::put_str(
                                d.base_name_un_id
                                        .names[(size_t)ItemRefType::a]);

                        saving::put_str(colors::color_to_name(d.color));
                }
        }
}

void load()
{
        for (int i = 0; i < (int)item::Id::END; ++i)
        {
                auto& d = item::g_data[i];

                if (d.type == ItemType::rod)
                {
                        d.base_name_un_id.names[(size_t)ItemRefType::plain] =
                                saving::get_str();

                        d.base_name_un_id.names[(size_t)ItemRefType::plural] =
                                saving::get_str();

                        d.base_name_un_id.names[(size_t)ItemRefType::a] =
                                saving::get_str();

                        d.color =
                                colors::name_to_color(saving::get_str())
                                        .value();
                }
        }
}

void Rod::save_hook() const
{
        saving::put_int(m_nr_charge_turns_left);
}

void Rod::load_hook()
{
        m_nr_charge_turns_left = saving::get_int();
}

void Rod::set_max_charge_turns_left()
{
        m_nr_charge_turns_left = nr_turns_to_recharge();

        if (player_bon::has_trait(Trait::elec_incl))
        {
                m_nr_charge_turns_left /= 2;
        }
}

ConsumeItem Rod::activate(actor::Actor* const actor)
{
        (void)actor;

        // Prevent using it if still charging, and identified (player character
        // knows that it's useless)
        if ((m_nr_charge_turns_left > 0) && m_data->is_identified)
        {
                const std::string rod_name =
                        name(ItemRefType::plain, ItemRefInf::none);

                msg_log::add("The " + rod_name + " is still charging.");

                return ConsumeItem::no;
        }

        m_data->is_tried = true;

        // TODO: Sfx

        const std::string rod_name_a =
                name(ItemRefType::a, ItemRefInf::none);

        msg_log::add("I activate " + rod_name_a + "...");

        if (m_nr_charge_turns_left == 0)
        {
                run_effect();

                set_max_charge_turns_left();
        }

        if (m_data->is_identified)
        {
                map::g_player->incr_shock(8.0, ShockSrc::use_strange_item);
        }
        else
        {
                // Not identified
                msg_log::add("Nothing happens.");
        }

        if (map::g_player->is_alive())
        {
                game_time::tick();
        }

        return ConsumeItem::no;
}

void Rod::on_std_turn_in_inv_hook(const InvType inv_type)
{
        (void)inv_type;

        // Already fully charged?
        if (m_nr_charge_turns_left == 0)
        {
                return;
        }

        // All charges not finished, continue countdown

        ASSERT(m_nr_charge_turns_left > 0);

        --m_nr_charge_turns_left;

        if (m_nr_charge_turns_left == 0)
        {
                const std::string my_name =
                        name(
                                ItemRefType::plain,
                                ItemRefInf::none);

                msg_log::add("The " + my_name + " has finished charging.");
        }
}

std::vector<std::string> Rod::descr_hook() const
{
        if (m_data->is_identified)
        {
                return {descr_identified()};
        }
        else
        {
                // Not identified
                return m_data->base_descr;
        }
}

void Rod::identify(const Verbose verbose)
{
        if (!m_data->is_identified)
        {
                m_data->is_identified = true;

                if (verbose == Verbose::yes)
                {
                        const std::string name_after =
                                name(ItemRefType::a,
                                     ItemRefInf::none);

                        msg_log::add("I have identified " + name_after + ".");

                        game::add_history_event("Identified " + name_after);
                }
        }
}

std::string Rod::name_inf_str() const
{
        if (m_data->is_identified)
        {
                if (m_nr_charge_turns_left > 0)
                {
                        const auto turns_left_str =
                                std::to_string(m_nr_charge_turns_left);

                        return "{" + turns_left_str + "}";
                }
                else
                {
                        return "";
                }
        }
        else
        {
                // Not identified
                return m_data->is_tried ? "{Tried}" : "";
        }
}

void Curing::run_effect()
{
        auto& player = *map::g_player;

        std::vector<PropId> props_can_heal = {
                PropId::blind,
                PropId::deaf,
                PropId::poisoned,
                PropId::infected,
                PropId::diseased,
                PropId::weakened,
                PropId::hp_sap};

        bool is_something_healed = false;

        for (PropId prop_id : props_can_heal)
        {
                if (player.m_properties.end_prop(prop_id))
                {
                        is_something_healed = true;
                }
        }

        if (player.restore_hp(3, false /* Not allowed above max */))
        {
                is_something_healed = true;
        }

        if (!is_something_healed)
        {
                msg_log::add("I feel fine.");
        }

        identify(Verbose::yes);
}

void Opening::run_effect()
{
        bool is_any_opened = false;

        const int chance = 100;

        for (const auto& p : map::rect().positions())
        {
                const auto& cell = map::g_cells.at(p);

                if (!cell.is_seen_by_player)
                {
                        continue;
                }

                const auto did_open =
                        spells::run_opening_spell_effect_at(
                                p,
                                chance,
                                SpellSkill::master);

                if (did_open == terrain::DidOpen::yes)
                {
                        is_any_opened = true;
                }
        }

        if (is_any_opened)
        {
                identify(Verbose::yes);
        }
}

void Bless::run_effect()
{
        const int nr_turns = rnd::range(8, 12);

        auto* prop = new PropBlessed();

        prop->set_duration(nr_turns);

        map::g_player->m_properties.apply(prop);

        identify(Verbose::yes);
}

void CloudMinds::run_effect()
{
        msg_log::add("I vanish from the minds of my enemies.");

        for (auto* actor : game_time::g_actors)
        {
                if (actor->is_player())
                {
                        continue;
                }

                actor->m_mon_aware_state.aware_counter = 0;
                actor->m_mon_aware_state.wary_counter = 0;
        }

        identify(Verbose::yes);
}

void Shockwave::run_effect()
{
        msg_log::add("It triggers a shock wave around me.");

        const P& player_pos = map::g_player->m_pos;

        for (const P& d : dir_utils::g_dir_list)
        {
                const P p(player_pos + d);

                auto* const terrain = map::g_cells.at(p).terrain;

                terrain->hit(DmgType::explosion, nullptr);
        }

        for (actor::Actor* actor : game_time::g_actors)
        {
                if (actor->is_player() ||
                    !actor->is_alive())
                {
                        continue;
                }

                const P& other_pos = actor->m_pos;

                const bool is_adj = is_pos_adj(player_pos, other_pos, false);

                if (!is_adj)
                {
                        continue;
                }

                if (actor::can_player_see_actor(*actor))
                {
                        std::string msg =
                                text_format::first_to_upper(actor->name_the()) +
                                " is hit!";

                        msg = text_format::first_to_upper(msg);

                        msg_log::add(msg);
                }

                actor::hit(*actor, rnd::range(1, 6), DmgType::explosion);

                // Surived the damage? Knock the monster back
                if (actor->is_alive())
                {
                        knockback::run(
                                *actor,
                                player_pos,
                                false,
                                Verbose::yes,
                                1);  // 1 extra turn paralyzed
                }
        }

        Snd snd("",
                audio::SfxId::END,
                IgnoreMsgIfOriginSeen::yes,
                player_pos,
                map::g_player,
                SndVol::high,
                AlertsMon::yes);

        snd.run();

        identify(Verbose::yes);
}

}  // namespace rod
