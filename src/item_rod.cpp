// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "item_rod.hpp"

#include "actor_hit.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "feature_rigid.hpp"
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
#include "text_format.hpp"

void Rod::save() const
{
        saving::put_int(m_nr_charge_turns_left);
}

void Rod::load()
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

ConsumeItem Rod::activate(Actor* const actor)
{
        (void)actor;

        // Prevent using it if still charging, and identified (player character
        // knows that it's useless)
        if ((m_nr_charge_turns_left > 0) &&
            m_data->is_identified)
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
        else // Not identified
        {
                msg_log::add("Nothing happens.");
        }

        if (map::g_player->is_alive())
        {
                game_time::tick();
        }

        return ConsumeItem::no;
}

void Rod::on_std_turn_in_inv(const InvType inv_type)
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
                const std::string rod_name =
                        name(ItemRefType::plain,
                             ItemRefInf::none);

                msg_log::add("The " + rod_name + " has finished charging.");
        }
}

std::vector<std::string> Rod::descr() const
{
        if (m_data->is_identified)
        {
                return {descr_identified()};
        }
        else // Not identified
        {
                return m_data->base_descr;
        }
}

void Rod::identify(const Verbosity verbosity)
{
        if (!m_data->is_identified)
        {
                m_data->is_identified = true;

                if (verbosity == Verbosity::verbose)
                {
                        const std::string name_after =
                                name(ItemRefType::a,
                                     ItemRefInf::none);

                        msg_log::add("I have identified " + name_after + ".");

                        game::add_history_event(
                                "Identified " + name_after + ".");
                }
        }
}

std::string Rod::name_inf_str() const
{
        if (m_data->is_identified)
        {
                const std::string charge_str =
                        std::to_string(m_nr_charge_turns_left);

                return
                        (m_nr_charge_turns_left > 0)
                        ? "{" + charge_str + "}"
                        : "";
        }
        else // Not identified
        {
                return
                        m_data->is_tried
                        ? "{Tried}"
                        : "";
        }
}

void RodCuring::run_effect()
{
        Player& player = *map::g_player;

        std::vector<PropId> props_can_heal =
                {
                        PropId::blind,
                        PropId::poisoned,
                        PropId::infected,
                        PropId::diseased,
                        PropId::weakened,
                        PropId::hp_sap
                };

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

        identify(Verbosity::verbose);
}

void RodOpening::run_effect()
{
        bool is_any_opened = false;

        for (auto& cell : map::g_cells)
        {
                if (cell.is_seen_by_player)
                {
                        DidOpen did_open = cell.rigid->open(nullptr);

                        if (did_open == DidOpen::yes)
                        {
                                is_any_opened = true;
                        }
                }
        }

        if (is_any_opened)
        {
                identify(Verbosity::verbose);
        }
}

void RodBless::run_effect()
{
        const int nr_turns = rnd::range(8, 12);

        auto prop = new PropBlessed();

        prop->set_duration(nr_turns);

        map::g_player->m_properties.apply(prop);

        identify(Verbosity::verbose);
}

void RodCloudMinds::run_effect()
{
        msg_log::add("I vanish from the minds of my enemies.");

        for (Actor* actor : game_time::g_actors)
        {
                if (!actor->is_player())
                {
                        Mon* const mon = static_cast<Mon*>(actor);

                        mon->m_aware_of_player_counter = 0;

                        mon->m_wary_of_player_counter = 0;
                }
        }

        identify(Verbosity::verbose);
}

void RodShockwave::run_effect()
{
        msg_log::add("It triggers a shock wave around me.");

        const P& player_pos = map::g_player->m_pos;

        for (const P& d : dir_utils::g_dir_list)
        {
                const P p(player_pos + d);

                Rigid* const rigid = map::g_cells.at(p).rigid;

                rigid->hit(
                        1, // Doesn't matter
                        DmgType::physical,
                        DmgMethod::explosion);
        }

        for (Actor* actor : game_time::g_actors)
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

                if (map::g_player->can_see_actor(*actor))
                {
                        std::string msg =
                                text_format::first_to_upper(actor->name_the()) +
                                " is hit!";

                        msg = text_format::first_to_upper(msg);

                        msg_log::add(msg);
                }

                actor::hit(*actor, rnd::dice(1, 6), DmgType::physical);

                // Surived the damage? Knock the monster back
                if (actor->is_alive())
                {
                        knockback::run(*actor,
                                       player_pos,
                                       false,
                                       Verbosity::verbose,
                                       1); // 1 extra turn paralyzed
                }
        }

        Snd snd("",
                SfxId::END,
                IgnoreMsgIfOriginSeen::yes,
                player_pos,
                map::g_player,
                SndVol::high,
                AlertsMon::yes);

        snd.run();

        identify(Verbosity::verbose);
}

namespace rod_handling
{

namespace
{

std::vector<RodLook> rod_looks_;

} // namespace

void init()
{
        TRACE_FUNC_BEGIN;

        // Init possible rod colors and fake names
        rod_looks_.clear();
        rod_looks_.push_back({"Iron", "an Iron", colors::gray()});
        rod_looks_.push_back({"Zinc", "a Zinc", colors::light_white()});
        rod_looks_.push_back({"Chromium", "a Chromium", colors::light_white()});
        rod_looks_.push_back({"Tin", "a Tin", colors::light_white()});
        rod_looks_.push_back({"Silver", "a Silver", colors::light_white()});
        rod_looks_.push_back({"Golden", "a Golden", colors::yellow()});
        rod_looks_.push_back({"Nickel", "a Nickel", colors::light_white()});
        rod_looks_.push_back({"Copper", "a Copper", colors::brown()});
        rod_looks_.push_back({"Lead", "a Lead", colors::gray()});
        rod_looks_.push_back({"Tungsten", "a Tungsten", colors::white()});
        rod_looks_.push_back({"Platinum", "a Platinum", colors::light_white()});
        rod_looks_.push_back({"Lithium", "a Lithium", colors::white()});
        rod_looks_.push_back({"Zirconium", "a Zirconium", colors::white()});
        rod_looks_.push_back({"Gallium", "a Gallium", colors::light_white()});
        rod_looks_.push_back({"Cobalt", "a Cobalt", colors::light_blue()});
        rod_looks_.push_back({"Titanium", "a Titanium", colors::light_white()});
        rod_looks_.push_back({"Magnesium", "a Magnesium", colors::white()});

        for (auto& d : item_data::g_data)
        {
                if (d.type == ItemType::rod)
                {
                        // Color and false name
                        const size_t idx = rnd::range(0, rod_looks_.size() - 1);

                        RodLook& look = rod_looks_[idx];

                        d.base_name_un_id.names[(size_t)ItemRefType::plain] =
                                look.name_plain + " Rod";

                        d.base_name_un_id.names[(size_t)ItemRefType::plural] =
                                look.name_plain + " Rods";

                        d.base_name_un_id.names[(size_t)ItemRefType::a] =
                                look.name_a + " Rod";

                        d.color = look.color;

                        rod_looks_.erase(rod_looks_.begin() + idx);

                        // True name
                        const Rod* const rod =
                                static_cast<const Rod*>(
                                        item_factory::make(d.id, 1));

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
        for (int i = 0; i < (int)ItemId::END; ++i)
        {
                ItemData& d = item_data::g_data[i];

                if (d.type == ItemType::rod)
                {
                        saving::put_str(
                                d.base_name_un_id.names[
                                        (size_t)ItemRefType::plain]);

                        saving::put_str(
                                d.base_name_un_id.names[
                                        (size_t)ItemRefType::plural]);

                        saving::put_str(
                                d.base_name_un_id.names[
                                        (size_t)ItemRefType::a]);

                        saving::put_str(colors::color_to_name(d.color));
                }
        }
}

void load()
{
        for (int i = 0; i < (int)ItemId::END; ++i)
        {
                ItemData& d = item_data::g_data[i];

                if (d.type == ItemType::rod)
                {
                        d.base_name_un_id.names[(size_t)ItemRefType::plain] =
                                saving::get_str();

                        d.base_name_un_id.names[(size_t)ItemRefType::plural] =
                                saving::get_str();

                        d.base_name_un_id.names[(size_t)ItemRefType::a] =
                                saving::get_str();

                        d.color = colors::name_to_color(saving::get_str());
                }
        }
}

} // rod_handling
