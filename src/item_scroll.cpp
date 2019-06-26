// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "item_scroll.hpp"

#include <string>

#include "actor_player.hpp"
#include "game.hpp"
#include "init.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "property_handler.hpp"
#include "saving.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<std::string> s_false_names;


static SpellSkill get_player_skill_for_scroll(SpellId spell_id)
{
        const auto player_spell_skill = map::g_player->spell_skill(spell_id);

        return (SpellSkill)std::max(
                player_spell_skill,
                SpellSkill::expert);
}

// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------
namespace scroll
{

void init()
{
        TRACE_FUNC_BEGIN;

        // Randomize scroll fake names
        s_false_names.clear();
        s_false_names.push_back("Cruensseasrjit");
        s_false_names.push_back("Rudsceleratus");
        s_false_names.push_back("Rudminuox");
        s_false_names.push_back("Cruo stragara-na");
        s_false_names.push_back("Praya navita");
        s_false_names.push_back("Pretia Cruento");
        s_false_names.push_back("Pestis Cruento");
        s_false_names.push_back("Cruento Pestis");
        s_false_names.push_back("Domus-bhaava");
        s_false_names.push_back("Acerbus-shatruex");
        s_false_names.push_back("Pretaanluxis");
        s_false_names.push_back("Praansilenux");
        s_false_names.push_back("Quodpipax");
        s_false_names.push_back("Lokemundux");
        s_false_names.push_back("Profanuxes");
        s_false_names.push_back("Shaantitus");
        s_false_names.push_back("Geropayati");
        s_false_names.push_back("Vilomaxus");
        s_false_names.push_back("Bhuudesco");
        s_false_names.push_back("Durbentia");
        s_false_names.push_back("Bhuuesco");
        s_false_names.push_back("Maravita");
        s_false_names.push_back("Infirmux");

        std::vector<std::string> cmb;
        cmb.clear();
        cmb.push_back("Cruo");
        cmb.push_back("Cruonit");
        cmb.push_back("Cruentu");
        cmb.push_back("Marana");
        cmb.push_back("Domus");
        cmb.push_back("Malax");
        cmb.push_back("Caecux");
        cmb.push_back("Eximha");
        cmb.push_back("Vorox");
        cmb.push_back("Bibox");
        cmb.push_back("Pallex");
        cmb.push_back("Profanx");
        cmb.push_back("Invisuu");
        cmb.push_back("Invisux");
        cmb.push_back("Odiosuu");
        cmb.push_back("Odiosux");
        cmb.push_back("Vigra");
        cmb.push_back("Crudux");
        cmb.push_back("Desco");
        cmb.push_back("Esco");
        cmb.push_back("Gero");
        cmb.push_back("Klaatu");
        cmb.push_back("Barada");
        cmb.push_back("Nikto");

        const size_t nr_cmb_parts = cmb.size();

        for (size_t i = 0; i < nr_cmb_parts; ++i)
        {
                for (size_t ii = 0; ii < nr_cmb_parts; ii++)
                {
                        if (i != ii)
                        {
                                s_false_names.push_back(cmb[i] + " " + cmb[ii]);
                        }
                }
        }

        std::vector<item::ItemData*> scroll_data;

        for (auto& d : item::g_data)
        {
                if (d.type == ItemType::scroll)
                {
                        scroll_data.push_back(&d);
                }
        }

        for (auto& d : scroll_data)
        {
                // False name
                const size_t idx = rnd::idx(s_false_names);

                const auto& title = s_false_names[idx];

                d->base_name_un_id.names[(size_t)ItemRefType::plain] =
                        "Manuscript titled " + title;

                d->base_name_un_id.names[(size_t)ItemRefType::plural] =
                        "Manuscripts titled " + title;

                d->base_name_un_id.names[(size_t)ItemRefType::a] =
                        "a Manuscript titled " + title;

                s_false_names.erase(s_false_names.begin() + idx);

                // True name
                const std::unique_ptr<const Scroll> scroll(
                        static_cast<const Scroll*>(
                                item::make(d->id, 1)));

                const auto real_type_name = scroll->real_name();

                const auto real_name =
                        "Manuscript of " + real_type_name;

                const auto real_name_plural =
                        "Manuscripts of " + real_type_name;

                const auto real_name_a =
                        "a Manuscript of " + real_type_name;

                d->base_name.names[(size_t)ItemRefType::plain] =
                        real_name;

                d->base_name.names[(size_t)ItemRefType::plural] =
                        real_name_plural;

                d->base_name.names[(size_t)ItemRefType::a] =
                        real_name_a;
        }

        // Randomize scroll spawning chances - some scrolls have a "high"
        // chance of spawning, and some have a "low" chance. The effect of this
        // should be that there is less chance to find all spells.
        rnd::shuffle(scroll_data);

        const size_t nr_scrolls = scroll_data.size();
        const size_t nr_high_chance = nr_scrolls / 2;

        for (size_t i = 0; i < nr_scrolls; ++i)
        {
                scroll_data[i]->chance_to_incl_in_spawn_list =
                        (i < nr_high_chance)
                        ? 35
                        : 15;
        }

        TRACE_FUNC_END;
}

void save()
{
        for (size_t i = 0; i < (size_t)item::Id::END; ++i)
        {
                if (item::g_data[i].type != ItemType::scroll)
                {
                        continue;
                }

                auto& names = item::g_data[i].base_name_un_id.names;

                saving::put_str(names[(size_t)ItemRefType::plain]);
                saving::put_str(names[(size_t)ItemRefType::plural]);
                saving::put_str(names[(size_t)ItemRefType::a]);
        }
}

void load()
{
        for (size_t i = 0; i < (size_t)item::Id::END; ++i)
        {
                if (item::g_data[i].type != ItemType::scroll)
                {
                        continue;
                }

                auto& names = item::g_data[i].base_name_un_id.names;

                names[(size_t)ItemRefType::plain] = saving::get_str();
                names[(size_t)ItemRefType::plural] = saving::get_str();
                names[(size_t)ItemRefType::a] = saving::get_str();
        }
}


Scroll::Scroll(item::ItemData* const item_data) :
        Item(item_data),
        m_domain_feeling_dlvl_countdown(rnd::range(1, 3)),
        m_domain_feeling_turn_countdown(rnd::range(100, 200))
{

}

void Scroll::save_hook() const
{
        saving::put_int(m_domain_feeling_dlvl_countdown);
        saving::put_int(m_domain_feeling_turn_countdown);
}

void Scroll::load_hook()
{
        m_domain_feeling_dlvl_countdown = saving::get_int();
        m_domain_feeling_turn_countdown = saving::get_int();
}

const std::string Scroll::real_name() const
{
        const std::unique_ptr<const Spell> spell(make_spell());

        const std::string scroll_name = spell->name();

        return scroll_name;
}

std::vector<std::string> Scroll::descr_hook() const
{
        const std::unique_ptr<const Spell> spell(make_spell());

        if (m_data->is_identified)
        {
                const auto skill = get_player_skill_for_scroll(spell->id());

                const auto descr = spell->descr(skill, SpellSrc::manuscript);

                return descr;
        }
        else // Not identified
        {
                auto lines = m_data->base_descr;

                if (m_data->is_spell_domain_known)
                {
                        const std::string domain_str = spell->domain_descr();

                        if (!domain_str.empty())
                        {
                                lines.push_back(domain_str);
                        }
                }

                return lines;
        }
}

void Scroll::on_player_reached_new_dlvl_hook()
{
        auto& d = data();

        if (d.is_spell_domain_known ||
            d.is_identified ||
            (m_domain_feeling_dlvl_countdown <= 0))
        {
                return;
        }

        --m_domain_feeling_dlvl_countdown;
}

void Scroll::on_actor_turn_in_inv_hook(const InvType inv_type)
{
        (void)inv_type;

        if (m_actor_carrying != map::g_player)
        {
                return;
        }

        auto& d = data();

        if (d.is_spell_domain_known ||
            d.is_identified ||
            (m_domain_feeling_dlvl_countdown > 0))
        {
                return;
        }

        ASSERT(m_domain_feeling_turn_countdown > 0);

        --m_domain_feeling_turn_countdown;

        if (m_domain_feeling_turn_countdown <= 0)
        {
                TRACE << "Scroll domain discovered" << std::endl;

                const std::string name_plural =
                        d.base_name_un_id.names[(size_t)ItemRefType::plural];

                const std::unique_ptr<const Spell> spell(make_spell());

                const auto domain = spell->domain();

                if (domain != OccultistDomain::END)
                {
                        const std::string domain_str =
                                text_format::first_to_lower(
                                        player_bon::spell_domain_title(
                                                spell->domain()));

                        if (!domain_str.empty())
                        {
                                msg_log::add(
                                        std::string(
                                                "I feel like " +
                                                name_plural  +
                                                " belong to the " +
                                                domain_str +
                                                " domain."),
                                        colors::text(),
                                        false,
                                        MorePromptOnMsg::yes);
                        }
                }

                d.is_spell_domain_known = true;
        }
}

ConsumeItem Scroll::activate(actor::Actor* const actor)
{
        TRACE_FUNC_BEGIN;

        // Check properties which NEVER allows reading or speaking
        if (!actor->m_properties.allow_read_absolute(Verbosity::verbose) ||
            !actor->m_properties.allow_speak(Verbosity::verbose))
        {
                return ConsumeItem::no;
        }

        const P& player_pos(map::g_player->m_pos);

        if (map::g_dark.at(player_pos) && !map::g_light.at(player_pos))
        {
                msg_log::add("It's too dark to read here.");

                TRACE_FUNC_END;

                return ConsumeItem::no;
        }

        // OK, we can try to cast

        const bool is_identified_before = m_data->is_identified;

        if (is_identified_before)
        {
                const std::string scroll_name =
                        name(ItemRefType::a, ItemRefInf::none);

                msg_log::add("I read " + scroll_name + "...");
        }
        else // Not already identified
        {
                msg_log::add(
                        "I recite the forbidden incantations on the "
                        "manuscript...");
        }

        const std::string crumble_str = "The Manuscript crumbles to dust.";

        // Check properties which MAY allow reading, with a random chance
        if (!actor->m_properties.allow_read_chance(Verbosity::verbose))
        {
                msg_log::add(crumble_str);

                TRACE_FUNC_END;

                return ConsumeItem::yes;
        }

        // OK, we are fully allowed to read the scroll - cast the spell

        const std::unique_ptr<const Spell> spell(make_spell());

        const SpellId id = spell->id();

        const auto skill = get_player_skill_for_scroll(id);

        spell->cast(
                map::g_player,
                skill,
                SpellSrc::manuscript);

        msg_log::add(crumble_str);

        identify(Verbosity::verbose);

        // Learn spell
        if (spell->player_can_learn())
        {
                player_spells::learn_spell(id, Verbosity::verbose);
        }

        TRACE_FUNC_END;

        return ConsumeItem::yes;
}

Spell* Scroll::make_spell() const
{
        return spell_factory::make_spell_from_id(
                m_data->spell_cast_from_scroll);
}

void Scroll::identify(const Verbosity verbosity)
{
        if (m_data->is_identified)
        {
                return;
        }

        m_data->is_identified = true;

        if (verbosity == Verbosity::verbose)
        {
                const std::string name_after =
                        name(ItemRefType::a, ItemRefInf::none);

                msg_log::add("I have identified " + name_after + ".");

                game::add_history_event("Identified " + name_after + ".");
        }
}

std::string Scroll::name_inf_str() const
{
        if (m_data->is_spell_domain_known && !m_data->is_identified)
        {
                const std::unique_ptr<const Spell> spell(make_spell());

                const auto domain = spell->domain();

                if (domain != OccultistDomain::END)
                {
                        const auto domain_title =
                                player_bon::spell_domain_title(
                                        spell->domain());

                        return "{" + domain_title + "}";
                }
        }

        return "";
}

} // scroll
