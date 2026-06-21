// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "item_scroll.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <ostream>
#include <string>

#include "actor.hpp"
#include "actor_see.hpp"
#include "array2.hpp"
#include "debug.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "global.hpp"
#include "inventory.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "i18n.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "random.hpp"
#include "saving.hpp"
#include "spells.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
inline constexpr int s_scroll_low_spawn_chance = 5;
inline constexpr int s_scroll_high_spawn_chance = 20;

static std::vector<std::string> s_fake_names;

static SpellSkill player_skill_for_scroll(const SpellId spell_id)
{
    SpellSkill skill = actor::spell_skill(*map::g_player, spell_id);

    const bool has_necronomicon =
        map::g_player->m_inv.has_item_in_backpack(
            item::Id::necronomicon);

    const auto skill_cap =
        has_necronomicon
        ? SpellSkill::transcendent
        : SpellSkill::master;

    if (skill < skill_cap) {
        skill = (SpellSkill)((int)skill + 1);
    }

    return skill;
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
    s_fake_names.clear();

    // Fixed fake names:
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.cruensseasrjit", "Cruensseasrjit"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.rudsceleratus", "Rudsceleratus"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.rudminuox", "Rudminuox"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.cruo_stragara_na", "Cruo stragara-na"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.praya_navita", "Praya navita"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.pretia_cruento", "Pretia Cruento"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.pestis_cruento", "Pestis Cruento"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.cruento_pestis", "Cruento Pestis"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.domus_bhaava", "Domus-bhaava"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.acerbus_shatruex", "Acerbus-shatruex"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.pretaanluxis", "Pretaanluxis"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.praansilenux", "Praansilenux"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.quodpipax", "Quodpipax"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.lokemundux", "Lokemundux"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.profanuxes", "Profanuxes"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.shaantitus", "Shaantitus"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.geropayati", "Geropayati"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.vilomaxus", "Vilomaxus"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.bhuudesco", "Bhuudesco"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.durbentia", "Durbentia"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.bhuuesco", "Bhuuesco"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.maravita", "Maravita"));
    s_fake_names.emplace_back(i18n::get("item_scroll.fake_name.infirmux", "Infirmux"));

    // Words that can be combined into names:
    std::vector<std::string> combinable_names;
    combinable_names.clear();
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.cruo", "Cruo"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.cruonit", "Cruonit"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.cruentu", "Cruentu"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.marana", "Marana"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.domus", "Domus"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.malax", "Malax"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.caecux", "Caecux"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.eximha", "Eximha"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.vorox", "Vorox"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.bibox", "Bibox"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.pallex", "Pallex"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.profanx", "Profanx"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.invisuu", "Invisuu"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.invisux", "Invisux"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.odiosuu", "Odiosuu"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.odiosux", "Odiosux"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.vigra", "Vigra"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.crudux", "Crudux"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.desco", "Desco"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.esco", "Esco"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.gero", "Gero"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.klaatu", "Klaatu"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.barada", "Barada"));
    combinable_names.emplace_back(i18n::get("item_scroll.fake_word.nikto", "Nikto"));

    const size_t nr_combinable_names = combinable_names.size();

    for (size_t i = 0; i < nr_combinable_names; ++i) {
        for (size_t ii = 0; ii < nr_combinable_names; ii++) {
            if (i != ii) {
                s_fake_names.push_back(combinable_names[i] + " " + combinable_names[ii]);
            }
        }
    }

    std::vector<item::ItemData*> scroll_data;

    for (item::ItemData& d : item::g_data) {
        if (d.type == ItemType::scroll) {
            scroll_data.push_back(&d);
        }
    }

    for (item::ItemData* d : scroll_data) {
        // False name
        const size_t idx = rnd::idx(s_fake_names);

        const std::string& title = s_fake_names[idx];

        d->base_name_un_id.names[(size_t)ItemNameType::plain] =
            i18n::get(
                "item_scroll.manuscript_titled_prefix",
                "Manuscript titled ") +
            title +
            i18n::get("item_scroll.manuscript_titled_suffix", "");
        d->base_name_un_id.names[(size_t)ItemNameType::plural] =
            i18n::get(
                "item_scroll.manuscripts_titled_prefix",
                "Manuscripts titled ") +
            title +
            i18n::get("item_scroll.manuscripts_titled_suffix", "");
        d->base_name_un_id.names[(size_t)ItemNameType::a] =
            i18n::get(
                "item_scroll.a_manuscript_titled_prefix",
                "a Manuscript titled ") +
            title +
            i18n::get("item_scroll.a_manuscript_titled_suffix", "");

        s_fake_names.erase(s_fake_names.begin() + (int)idx);

        // True name
        const std::unique_ptr<const Scroll> scroll(
            static_cast<const Scroll*>(item::make(d->id, 1)));

        const std::string real_type_name = scroll->real_name();

        const std::string real_name =
            i18n::get("item_scroll.manuscript_of_prefix", "Manuscript of ") +
            real_type_name +
            i18n::get("item_scroll.manuscript_of_suffix", "");
        const std::string real_name_plural =
            i18n::get("item_scroll.manuscripts_of_prefix", "Manuscripts of ") +
            real_type_name +
            i18n::get("item_scroll.manuscripts_of_suffix", "");
        const std::string real_name_a =
            i18n::get("item_scroll.a_manuscript_of_prefix", "a Manuscript of ") +
            real_type_name +
            i18n::get("item_scroll.a_manuscript_of_suffix", "");

        d->base_name.names[(size_t)ItemNameType::plain] = real_name;
        d->base_name.names[(size_t)ItemNameType::plural] = real_name_plural;
        d->base_name.names[(size_t)ItemNameType::a] = real_name_a;
    }

    // Randomize scroll spawning chances - some scrolls have a "high" chance of spawning, and some
    // have a "low" chance. The effect of this should be that there is less chance to find all
    // spells.
    rnd::shuffle(scroll_data);

    const size_t nr_scrolls = scroll_data.size();
    const size_t nr_high_chance = nr_scrolls / 2;

    for (size_t i = 0; i < nr_scrolls; ++i) {
        scroll_data[i]->chance_to_incl_in_spawn_list =
            (i < nr_high_chance)
            ? s_scroll_low_spawn_chance
            : s_scroll_high_spawn_chance;
    }

    TRACE_FUNC_END;
}

void save()
{
    for (size_t i = 0; i < (size_t)item::Id::END; ++i) {
        if (item::g_data[i].type != ItemType::scroll) {
            continue;
        }

        auto& names = item::g_data[i].base_name_un_id.names;

        saving::put_str(names[(size_t)ItemNameType::plain]);
        saving::put_str(names[(size_t)ItemNameType::plural]);
        saving::put_str(names[(size_t)ItemNameType::a]);
    }
}

void load()
{
    for (size_t i = 0; i < (size_t)item::Id::END; ++i) {
        if (item::g_data[i].type != ItemType::scroll) {
            continue;
        }

        auto& names = item::g_data[i].base_name_un_id.names;

        names[(size_t)ItemNameType::plain] = saving::get_str();
        names[(size_t)ItemNameType::plural] = saving::get_str();
        names[(size_t)ItemNameType::a] = saving::get_str();
    }
}

Scroll::Scroll(item::ItemData* const item_data) :
    Item(item_data)
{
}

std::string Scroll::real_name() const
{
    const std::unique_ptr<const Spell> spell(make_spell());

    return spell->name();
}

std::vector<std::string> Scroll::descr_hook() const
{
    const std::unique_ptr<const Spell> spell(make_spell());

    if (m_data->is_identified) {
        const auto skill = player_skill_for_scroll(spell->id());

        return spell->descr(skill, SpellSrc::manuscript);
    }
    else {
        // Not identified
        auto lines = m_data->base_descr;

        if (m_data->is_spell_domain_known) {
            const std::string domain_str = spell->domain_descr();

            if (!domain_str.empty()) {
                lines.push_back(domain_str);
            }
        }
        else {
            lines.emplace_back(
                i18n::get(
                    "item_scroll.keep_to_reveal",
                    "Perhaps keeping it for a while will reveal "
                    "something about it."));
        }

        return lines;
    }
}

void Scroll::reveal_domain() const
{
    if (m_data->is_spell_domain_known || m_data->is_identified) {
        return;
    }

    TRACE << "Scroll domain discovered" << "\n";

    const std::string name_plural =
        m_data->base_name_un_id.names[(size_t)ItemNameType::plural];

    const std::unique_ptr<const Spell> spell(make_spell());

    const SpellDomain domain = spell->domain();

    if (domain != SpellDomain::END) {
        const std::string domain_str =
            text_format::first_to_lower(
                spells::spell_domain_title(
                    spell->domain()));

        if (!domain_str.empty()) {
            msg_log::add(
                std::string(
                    i18n::get("item_scroll.domain_reveal_prefix", "I feel like ") +
                    name_plural +
                    i18n::get("item_scroll.domain_reveal_mid", " belong to the ") +
                    domain_str +
                    i18n::get("item_scroll.domain_reveal_suffix", " domain.")));
        }
    }

    m_data->is_spell_domain_known = true;
}

ItemPrePickResult Scroll::pre_pickup_hook()
{
    if (!player_bon::is_bg(Bg::exorcist)) {
        return ItemPrePickResult::do_pickup;
    }

    // Is exorcist

    const std::string noun = (m_nr_items == 1)
        ? i18n::get("item_scroll.text_singular", "text")
        : i18n::get("item_scroll.text_plural", "texts");

    msg_log::add(
        i18n::get("item_scroll.destroy_profane_prefix", "I destroy the profane ") +
        noun +
        i18n::get("item_scroll.exclaim", "!"));

    game::incr_player_xp(g_xp_on_exorcist_destroy_scroll * m_nr_items);

    actor::restore_sp(
        *map::g_player,
        999,
        actor::AllowRestoreAboveMax::no,
        Verbose::no);

    actor::restore_exorcist_fervor(g_exorcist_fervor_destroy_scroll * m_nr_items);

    return ItemPrePickResult::destroy_item;
}

ConsumeItem Scroll::activate(actor::Actor* const actor)
{
    TRACE_FUNC_BEGIN;

    // Check properties which NEVER allows reading or speaking
    if (!actor->m_properties.allow_read_absolute(Verbose::yes) ||
        !actor->m_properties.allow_speak(Verbose::yes)) {
        return ConsumeItem::no;
    }

    const auto& player_pos = map::g_player->m_pos;

    if (map::g_dark.at(player_pos) &&
        !map::g_light.at(player_pos) &&
        !map::g_player->m_properties.has(prop::Id::darkvision)) {
        msg_log::add(i18n::get(
            "item_scroll.too_dark",
            "It's too dark to read here."));

        TRACE_FUNC_END;

        return ConsumeItem::no;
    }

    // OK, we can try to cast

    const bool is_identified_before = m_data->is_identified;

    if (is_identified_before) {
        const std::string scroll_name = name(ItemNameType::a, ItemNameInfo::none);

        msg_log::add(
            i18n::get("item_scroll.read_prefix", "I read ") +
            scroll_name +
            i18n::get("item_scroll.ellipsis", "..."));
    }
    else {
        // Not already identified
        msg_log::add(i18n::get(
            "item_scroll.recite_forbidden",
            "I recite the forbidden incantations on the manuscript..."));
    }

    const std::string crumble_str = i18n::get(
        "item_scroll.manuscript_crumbles",
        "The Manuscript crumbles to dust.");

    // Check properties which MAY allow reading, with a random chance
    if (!actor->m_properties.allow_read_chance(Verbose::yes)) {
        msg_log::add(crumble_str);

        game_time::tick();

        TRACE_FUNC_END;

        return ConsumeItem::yes;
    }

    // OK, we are fully allowed to read the scroll - cast the spell

    const std::unique_ptr<const Spell> spell(make_spell());

    const SpellId id = spell->id();

    const SpellSkill skill = player_skill_for_scroll(id);

    const std::vector<actor::Actor*> seen_foes = actor::seen_foes(*map::g_player);

    spell->cast(map::g_player, skill, SpellSrc::manuscript, seen_foes);

    msg_log::add(crumble_str);

    identify(Verbose::yes);

    player_spells::learn_spell(id, Verbose::yes);

    player_spells::recall_spell(id);

    TRACE_FUNC_END;

    return ConsumeItem::yes;
}

Spell* Scroll::make_spell() const
{
    return spells::make(m_data->spell_cast_from_scroll);
}

void Scroll::identify(const Verbose verbose)
{
    if (m_data->is_identified) {
        return;
    }

    m_data->is_identified = true;

    if (verbose == Verbose::yes) {
        const std::string name_after =
            name(
                ItemNameType::a,
                ItemNameInfo::none);

        msg_log::add(
            i18n::get("item_scroll.identified_prefix", "I have identified ") +
            name_after +
            ".");

        game::add_history_event(
            i18n::get("item_scroll.history_identified_prefix", "Identified ") +
            name_after);
    }
}

std::string Scroll::domain_str() const
{
    const std::unique_ptr<const Spell> spell(make_spell());

    const SpellDomain domain = spell->domain();

    if (domain == SpellDomain::END) {
        return "";
    }

    return spells::spell_domain_title(domain);
}

std::string Scroll::name_info_str(const ItemNameIdentified id_type) const
{
    if ((m_data->is_spell_domain_known && !m_data->is_identified) ||
        (id_type == ItemNameIdentified::force_identified)) {
        const std::string str = domain_str();

        if (!str.empty()) {
            return "(" + str + ")";
        }
    }

    return "";
}

}  // namespace scroll
