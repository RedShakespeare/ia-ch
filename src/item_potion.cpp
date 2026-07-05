// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "item_potion.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>

#include "actor.hpp"
#include "actor_see.hpp"
#include "array2.hpp"
#include "audio.hpp"
#include "audio_data.hpp"
#include "common_text.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "insanity.hpp"
#include "inventory_handling.hpp"
#include "item_data.hpp"
#include "item_appearance.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "property_handler.hpp"
#include "query.hpp"
#include "random.hpp"
#include "saving.hpp"
#include "state.hpp"
#include "teleport.hpp"
#include "terrain.hpp"
#include "terrain_data.hpp"
#include "text_format.hpp"
#include "i18n.hpp"

struct P;

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
struct PotionAppearance
{
    std::string name_plain;
    std::string name_a;
    Color color;
};

static std::vector<PotionAppearance> s_potion_appearances;

static std::vector<actor::Actor*> actors_reached_by_onyx_drop()
{
    const int max_dist = 6;

    std::vector<actor::Actor*> actors;

    for (actor::Actor* const actor : game_time::g_actors) {
        if (actor::is_player(actor)) {
            continue;
        }

        if (king_dist(map::g_player->m_pos, actor->m_pos) > max_dist) {
            continue;
        }

        actors.push_back(actor);
    }

    return actors;
}

static void apply_properties_with_equal_duration(
    const std::vector<prop::Id>& ids,
    actor::Actor& actor)
{
    std::vector<prop::Prop*> props_applied;

    for (prop::Id id : ids) {
        props_applied.push_back(prop::make(id));
    }

    int duration = -1;

    // The duration of all properties is decided by the randomized duration of the first.
    for (prop::Prop* const prop : props_applied) {
        if (duration == -1) {
            duration = prop->nr_turns_left();
        }
        else {
            prop->set_duration(duration);
        }
    }

    for (prop::Prop* const prop : props_applied) {
        actor.m_properties.apply(prop);
    }
}

// -----------------------------------------------------------------------------
// potion
// -----------------------------------------------------------------------------
namespace potion
{
// Build the potion appearance pool. Construction order must be deterministic and
// match between init() and refresh_localized_text() so fake_appearance_idx values align.
static void build_appearance_pool(std::vector<PotionAppearance>& pool)
{
    pool.assign(
        {{i18n::get("item_potion.appearance_golden", "Golden"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_golden", "Golden"),
          colors::yellow()},
         {i18n::get("item_potion.appearance_yellow", "Yellow"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_yellow", "Yellow"),
          colors::yellow()},
         {i18n::get("item_potion.appearance_dark", "Dark"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_dark", "Dark"),
          colors::gray()},
         {i18n::get("item_potion.appearance_black", "Black"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_black", "Black"),
          colors::gray()},
         {i18n::get("item_potion.appearance_oily", "Oily"),
          i18n::get("item_potion.article_an_space", "an ") +
              i18n::get("item_potion.appearance_oily", "Oily"),
          colors::gray()},
         {i18n::get("item_potion.appearance_smoky", "Smoky"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_smoky", "Smoky"),
          colors::white()},
         {i18n::get("item_potion.appearance_slimy", "Slimy"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_slimy", "Slimy"),
          colors::green()},
         {i18n::get("item_potion.appearance_green", "Green"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_green", "Green"),
          colors::light_green()},
         {i18n::get("item_potion.appearance_fiery", "Fiery"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_fiery", "Fiery"),
          colors::light_red()},
         {i18n::get("item_potion.appearance_murky", "Murky"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_murky", "Murky"),
          colors::dark_brown()},
         {i18n::get("item_potion.appearance_muddy", "Muddy"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_muddy", "Muddy"),
          colors::brown()},
         {i18n::get("item_potion.appearance_violet", "Violet"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_violet", "Violet"),
          colors::violet()},
         {i18n::get("item_potion.appearance_orange", "Orange"),
          i18n::get("item_potion.article_an_space", "an ") +
              i18n::get("item_potion.appearance_orange", "Orange"),
          colors::orange()},
         {i18n::get("item_potion.appearance_watery", "Watery"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_watery", "Watery"),
          colors::light_blue()},
         {i18n::get("item_potion.appearance_metallic", "Metallic"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_metallic", "Metallic"),
          colors::gray()},
         {i18n::get("item_potion.appearance_clear", "Clear"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_clear", "Clear"),
          colors::light_white()},
         {i18n::get("item_potion.appearance_misty", "Misty"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_misty", "Misty"),
          colors::light_white()},
         {i18n::get("item_potion.appearance_bloody", "Bloody"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_bloody", "Bloody"),
          colors::red()},
         {i18n::get("item_potion.appearance_magenta", "Magenta"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_magenta", "Magenta"),
          colors::magenta()},
         {i18n::get("item_potion.appearance_clotted", "Clotted"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_clotted", "Clotted"),
          colors::green()},
         {i18n::get("item_potion.appearance_moldy", "Moldy"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_moldy", "Moldy"),
          colors::brown()},
         {i18n::get("item_potion.appearance_frothy", "Frothy"),
          i18n::get("item_potion.article_a_space", "a ") +
              i18n::get("item_potion.appearance_frothy", "Frothy"),
          colors::white()}});
}

void init()
{
    TRACE_FUNC_BEGIN;

    // Init possible potion colors and fake names
    build_appearance_pool(s_potion_appearances);

    for (auto& d : item::g_data) {
        if (d.type != ItemType::potion) {
            continue;
        }

        // Color and false name
        const size_t idx = rnd::range(0, (int)s_potion_appearances.size() - 1);

        d.session.fake_appearance_idx = (int)idx;

        auto& look = s_potion_appearances[idx];

        d.text.base_name_un_id.names[(size_t)ItemNameType::plain] =
            look.name_plain +
            i18n::get("item_potion.unidentified_suffix", " Potion");

        d.text.base_name_un_id.names[(size_t)ItemNameType::plural] =
            look.name_plain +
            i18n::get("item_potion.unidentified_plural_suffix", " Potions");

        d.text.base_name_un_id.names[(size_t)ItemNameType::a] =
            look.name_a +
            i18n::get("item_potion.unidentified_suffix", " Potion");

        d.color = look.color;

        s_potion_appearances.erase(std::begin(s_potion_appearances) + (int)idx);

        // True name
        const auto* const potion =
            static_cast<const Potion*>(
                item::make(d.id, 1));

        const std::string real_type_name = potion->real_name();

        delete potion;

        const std::string real_name =
            i18n::get("item_potion.real_name_prefix", "Potion of ") +
            real_type_name;
        const std::string real_name_plural =
            i18n::get("item_potion.real_name_plural_prefix", "Potions of ") +
            real_type_name;
        const std::string real_name_a =
            i18n::get("item_potion.real_name_a_prefix", "a Potion of ") +
            real_type_name;

        d.text.base_name.names[(size_t)ItemNameType::plain] = real_name;
        d.text.base_name.names[(size_t)ItemNameType::plural] = real_name_plural;
        d.text.base_name.names[(size_t)ItemNameType::a] = real_name_a;
    }

    TRACE_FUNC_END;
}

void refresh_localized_text()
{
    TRACE_FUNC_BEGIN;

    // Rebuild the (now re-translated) appearance pool in the same deterministic
    // order as init(), then re-apply each potion's recorded appearance index.
    // No RNG — preserves the per-session potion<->appearance mapping.
    item::rebuild_fake_appearances<PotionAppearance>(
        ItemType::potion,
        [](std::vector<PotionAppearance>& pool) { build_appearance_pool(pool); },
        [](item::ItemData& d, const PotionAppearance& look) {
            d.text.base_name_un_id.names[(size_t)ItemNameType::plain] =
                look.name_plain +
                i18n::get("item_potion.unidentified_suffix", " Potion");
            d.text.base_name_un_id.names[(size_t)ItemNameType::plural] =
                look.name_plain +
                i18n::get("item_potion.unidentified_plural_suffix", " Potions");
            d.text.base_name_un_id.names[(size_t)ItemNameType::a] =
                look.name_a +
                i18n::get("item_potion.unidentified_suffix", " Potion");

            d.color = look.color;
        },
        [](item::ItemData& d) {
            const auto* const potion =
                static_cast<const Potion*>(item::make(d.id, 1));

            const std::string real_type_name = potion->real_name();

            delete potion;

            d.text.base_name.names[(size_t)ItemNameType::plain] =
                i18n::get("item_potion.real_name_prefix", "Potion of ") +
                real_type_name;
            d.text.base_name.names[(size_t)ItemNameType::plural] =
                i18n::get("item_potion.real_name_plural_prefix", "Potions of ") +
                real_type_name;
            d.text.base_name.names[(size_t)ItemNameType::a] =
                i18n::get("item_potion.real_name_a_prefix", "a Potion of ") +
                real_type_name;
        });

    TRACE_FUNC_END;
}

void save()
{
    for (int i = 0; i < (int)item::Id::END; ++i) {
        auto& d = item::g_data[i];

        if (d.type != ItemType::potion) {
            continue;
        }

        saving::put_str(d.text.base_name_un_id.names[(size_t)ItemNameType::plain]);
        saving::put_str(d.text.base_name_un_id.names[(size_t)ItemNameType::plural]);
        saving::put_str(d.text.base_name_un_id.names[(size_t)ItemNameType::a]);
        saving::put_str(colors::color_to_name(d.color));
    }
}

void load()
{
    for (int i = 0; i < (int)item::Id::END; ++i) {
        auto& d = item::g_data[i];

        if (d.type != ItemType::potion) {
            continue;
        }

        d.text.base_name_un_id.names[(size_t)ItemNameType::plain] = saving::get_str();
        d.text.base_name_un_id.names[(size_t)ItemNameType::plural] = saving::get_str();
        d.text.base_name_un_id.names[(size_t)ItemNameType::a] = saving::get_str();
        d.color = colors::name_to_color(saving::get_str());
    }
}

Potion::Potion(item::ItemData* const item_data) :
    Item(item_data)
{
}

ConsumeItem Potion::activate(actor::Actor* const actor)
{
    ASSERT(actor);

    if (!actor->m_properties.allow_eat(Verbose::yes)) {
        return ConsumeItem::no;
    }

    if (actor::is_player(actor)) {
        // Really quaff a known malign potion?
        if ((alignment() == PotionAlignment::bad) &&
            m_data->session.is_alignment_known &&
            config::warn_on_drink_malign_potion()) {
            const std::string name = this->name(ItemNameType::a);

            const std::string msg =
                "Drink " +
                name +
                "? " +
                common_text::g_yes_or_no_hint;

            msg_log::add(
                msg,
                colors::light_white(),
                MsgInterruptPlayer::no,
                MorePromptOnMsg::no,
                CopyToMsgHistory::no);

            auto result = query::yes_or_no();

            msg_log::clear();

            if (result == BinaryAnswer::no) {
                return ConsumeItem::no;
            }
        }

        m_data->session.is_tried = true;

        audio::play(audio::SfxId::potion_quaff);

        if (m_data->session.is_identified) {
            const std::string potion_name =
                name(ItemNameType::a, ItemNameInfo::none);

            msg_log::add(
                i18n::get("item_potion.drink_prefix", "I drink ") +
                potion_name +
                i18n::get("item_potion.ellipsis", "..."));
        }
        else {
            // Not identified
            const std::string potion_name =
                name(ItemNameType::plain, ItemNameInfo::none);

            msg_log::add(
                i18n::get("item_potion.drink_unknown_prefix", "I drink an unknown ") +
                potion_name +
                i18n::get("item_potion.ellipsis", "..."));
        }

        map::g_player->incr_shock(12.0, ShockSrc::use_strange_item);

        if (!actor::is_alive(*map::g_player)) {
            return ConsumeItem::yes;
        }
    }

    quaff_impl(*actor);

    // Also apply effect to nearby creatures if this is a malign potion and
    // the player has the Onyx Drop artifact.
    if ((alignment() == PotionAlignment::bad) &&
        actor::is_player(actor) &&
        actor->m_inv.has_item_in_backpack(item::Id::onyx_drop)) {
        for (actor::Actor* const other_actor : actors_reached_by_onyx_drop()) {
            quaff_impl(*other_actor);
        }
    }

    if (actor::is_alive(*map::g_player)) {
        game_time::tick();
    }

    return ConsumeItem::yes;
}

void Potion::identify(const Verbose verbose)
{
    if (m_data->session.is_identified) {
        return;
    }

    m_data->session.is_identified = true;

    m_data->session.is_alignment_known = true;

    if (verbose == Verbose::yes) {
        const std::string name_after =
            name(ItemNameType::a, ItemNameInfo::none);

        msg_log::add(
            i18n::get("item_potion.identified_prefix", "I have identified ") +
            name_after +
            ".");

        game::add_history_event(
            i18n::get("item_potion.history_identified_prefix", "Identified ") +
            name_after);
    }
}

std::vector<std::string> Potion::descr_hook() const
{
    if (m_data->session.is_identified) {
        return {descr_identified()};
    }
    else {
        auto lines = m_data->text.base_descr;

        if (m_data->session.is_alignment_known) {
            lines.push_back(
                i18n::get("item_potion.this_potion_is", "This potion is ") +
                text_format::first_to_lower(alignment_str()) +
                ".");
        }
        else {
            lines.emplace_back(
                i18n::get(
                    "item_potion.keep_it_for_a_while",
                    "Perhaps keeping it for a while will reveal something about it."));
        }

        return lines;
    }
}

std::string Potion::alignment_str() const
{
    return ((alignment() == PotionAlignment::good)
        ? i18n::get("item_potion.benign", "Benign")
        : i18n::get("item_potion.malign", "Malign"));
}

void Potion::reveal_alignment() const
{
    if (m_data->session.is_alignment_known || m_data->session.is_identified) {
        return;
    }

    TRACE << "Potion alignment discovered" << "\n";

    const std::string name_plural =
        m_data->text.base_name_un_id.names[(size_t)ItemNameType::plural];

    const std::string align_str =
        text_format::first_to_lower(alignment_str());

    msg_log::add(
        std::string(
            i18n::get("item_potion.feel_like_prefix", "I feel like ") +
            name_plural +
            i18n::get("item_potion.are", " are ") +
            align_str +
            i18n::get("item_potion.period", ".")));

    m_data->session.is_alignment_known = true;
}

void Potion::on_collide(const P& pos, actor::Actor* const actor)
{
    const bool is_seen = map::g_seen.at(pos);
    const auto* const terrain = map::g_terrain.at(pos);

    if (actor) {
        ASSERT(actor::is_alive(*actor));

        if (actor::is_player_aware_of_me(*actor)) {
            const std::string actor_name =
                actor::can_player_see_actor(*actor)
                ? actor::name_the(*actor)
                : "it";

            msg_log::add(
                i18n::get("item_potion.shatters_on_prefix", "The potion shatters on ") +
                actor_name +
                i18n::get("item_potion.period", "."));

            actor->make_player_aware_of_me();
        }

        collide_hook(pos, actor);
    }
    else if (is_seen) {
        const std::vector<terrain::Id> deep_terrains = {
            terrain::Id::chasm};

        if (!map_parsers::IsAnyOfTerrains(deep_terrains).run(pos)) {
            msg_log::add(
                i18n::get("item_potion.shatters_on_prefix", "The potion shatters on ") +
                terrain->name(Article::the) +
                i18n::get("item_potion.period", "."));
        }
    }
}

std::string Potion::name_info_str(const ItemNameIdentified id_type) const
{
    std::string str;

    // Only display benign/malign status if alignment is known, and the
    // potion is unidentified.
    if (data().session.is_alignment_known &&
        (!data().session.is_identified && (id_type != ItemNameIdentified::force_identified))) {
        str = "(" + alignment_str() + ")";
    }

    return str;
}

Color Potion::interface_color() const
{
    return colors::light_blue();
}

std::string Vitality::real_name() const
{
    return i18n::get("item_potion.vitality_name", "Vitality");
}

std::string Vitality::descr_identified() const
{
    return (
        i18n::get(
            "item_potion.vitality_descr",
            "This elixir fully restores all hit points, heals all "
            "wounds, and cures blindness, deafness, poisoning, "
            "infections, disease and weakening. "
            "Also, for some duration after consuming the potion, "
            "+1 extra hit point is healed per turn, and there is "
            "10% chance per turn to heal one wound."));
}

PotionAlignment Vitality::alignment() const
{
    return PotionAlignment::good;
}

void Vitality::quaff_impl(actor::Actor& actor)
{
    std::vector<prop::Id> props_can_heal = {
        prop::Id::blind,
        prop::Id::deaf,
        prop::Id::poisoned,
        prop::Id::infected,
        prop::Id::diseased,
        prop::Id::weakened,
        prop::Id::wound};

    for (prop::Id prop_id : props_can_heal) {
        actor.m_properties.end_prop(prop_id);
    }

    actor::restore_hp(actor, 999, actor::AllowRestoreAboveMax::no);

    actor.m_properties.apply(prop::make(prop::Id::regenerating));

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Vitality::collide_hook(const P& pos, actor::Actor* const actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Spirit::real_name() const
{
    return i18n::get("item_potion.spirit_name", "Spirit");
}

std::string Spirit::descr_identified() const
{
    return i18n::get("item_potion.spirit_descr", "Fully restores the spirit.");
}

PotionAlignment Spirit::alignment() const
{
    return PotionAlignment::good;
}

void Spirit::quaff_impl(actor::Actor& actor)
{
    // SP is always restored at least up to maximum, but can go beyond
    const int sp_max = actor::max_sp(actor);

    const int sp_restored = std::max(10, sp_max - actor.m_sp);

    actor::restore_sp(actor, sp_restored, actor::AllowRestoreAboveMax::yes);

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Spirit::collide_hook(const P& pos, actor::Actor* const actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Blindness::real_name() const
{
    return i18n::get("item_potion.blindness_name", "Blindness");
}

std::string Blindness::descr_identified() const
{
    return i18n::get(
        "item_potion.blindness_descr",
        "Causes temporary loss of vision.");
}

PotionAlignment Blindness::alignment() const
{
    return PotionAlignment::bad;
}

void Blindness::quaff_impl(actor::Actor& actor)
{
    actor.m_properties.apply(prop::make(prop::Id::blind));

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Blindness::collide_hook(const P& pos, actor::Actor* const actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Paral::real_name() const
{
    return i18n::get("item_potion.paralyzation_name", "Paralyzation");
}

std::string Paral::descr_identified() const
{
    return i18n::get("item_potion.paralyzation_descr", "Causes paralysis.");
}

PotionAlignment Paral::alignment() const
{
    return PotionAlignment::bad;
}

void Paral::quaff_impl(actor::Actor& actor)
{
    actor.m_properties.apply(prop::make(prop::Id::paralyzed));

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Paral::collide_hook(const P& pos, actor::Actor* const actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Disease::real_name() const
{
    return i18n::get("item_potion.disease_name", "Disease");
}

std::string Disease::descr_identified() const
{
    return i18n::get("item_potion.disease_descr", "Causes disease.");
}

PotionAlignment Disease::alignment() const
{
    return PotionAlignment::bad;
}

void Disease::quaff_impl(actor::Actor& actor)
{
    actor.m_properties.apply(prop::make(prop::Id::diseased));

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Disease::collide_hook(const P& pos, actor::Actor* const actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Conf::real_name() const
{
    return i18n::get("item_potion.confusion_name", "Confusion");
}

std::string Conf::descr_identified() const
{
    return i18n::get("item_potion.confusion_descr", "Causes confusion.");
}

PotionAlignment Conf::alignment() const
{
    return PotionAlignment::bad;
}

void Conf::quaff_impl(actor::Actor& actor)
{
    actor.m_properties.apply(prop::make(prop::Id::confused));

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Conf::collide_hook(const P& pos, actor::Actor* const actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Fortitude::real_name() const
{
    return i18n::get("item_potion.fortitude_name", "Fortitude");
}

std::string Fortitude::descr_identified() const
{
    return i18n::get(
        "item_potion.fortitude_descr",
        "Gives the consumer complete peace and clarity of mind.");
}

PotionAlignment Fortitude::alignment() const
{
    return PotionAlignment::good;
}

void Fortitude::quaff_impl(actor::Actor& actor)
{
    apply_properties_with_equal_duration(
        {prop::Id::r_fear,
         prop::Id::r_conf,
         prop::Id::r_sleep},
        actor);

    actor.m_properties.end_prop(prop::Id::frenzied);
    actor.m_properties.end_prop(prop::Id::hallucinating);

    if (actor::is_player(&actor)) {
        const std::vector<const InsSympt*> sympts = insanity::active_sympts();

        if (!sympts.empty()) {
            const InsSymptId id = rnd::element(sympts)->id();

            insanity::end_sympt(id);
        }

        map::g_player->restore_shock(999, false);

        msg_log::add(i18n::get(
            "item_potion.feel_more_at_ease",
            "I feel more at ease."));
    }

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Fortitude::collide_hook(const P& pos, actor::Actor* const actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Poison::real_name() const
{
    return i18n::get("item_potion.poison_name", "Poison");
}

std::string Poison::descr_identified() const
{
    return i18n::get("item_potion.poison_descr", "A sinister brew.");
}

PotionAlignment Poison::alignment() const
{
    return PotionAlignment::bad;
}

void Poison::quaff_impl(actor::Actor& actor)
{
    actor.m_properties.apply(prop::make(prop::Id::poisoned));

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Poison::collide_hook(const P& pos, actor::Actor* const actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Insight::real_name() const
{
    return i18n::get("item_potion.insight_name", "Insight");
}

std::string Insight::descr_identified() const
{
    return i18n::get(
        "item_potion.insight_descr",
        "This strange concoction causes a sudden flash of intuition.");
}

PotionAlignment Insight::alignment() const
{
    return PotionAlignment::good;
}

void Insight::quaff_impl(actor::Actor& actor)
{
    (void)actor;

    identify(Verbose::yes);

    // Run identify selection menu

    // NOTE: We push this state BEFORE giving any XP (directly or via
    // identifying stuff), because if the player gains a new level in the
    // process, the trait selection should occur first
    states::push(std::make_unique<SelectIdentify>());

    // Insight gives some extra XP, to avoid making the potion worthless if
    // the player identifies all items)
    msg_log::add(i18n::get(
        "item_potion.feel_insightful",
        "I feel insightful."));

    game::incr_player_xp(g_xp_on_drink_insight_potion);

    msg_log::more_prompt();
}

void Insight::collide_hook(const P& pos, actor::Actor* const actor)
{
    (void)pos;
    (void)actor;
}

std::string Curing::real_name() const
{
    return i18n::get("item_potion.curing_name", "Curing");
}

std::string Curing::descr_identified() const
{
    return (
        i18n::get(
            "item_potion.curing_descr",
            "Restores 3 hit points, and cures blindness, deafness, "
            "poisoning, infections, disease and weakening."));
}

PotionAlignment Curing::alignment() const
{
    return PotionAlignment::good;
}

void Curing::quaff_impl(actor::Actor& actor)
{
    std::vector<prop::Id> props_can_heal = {
        prop::Id::blind,
        prop::Id::deaf,
        prop::Id::poisoned,
        prop::Id::infected,
        prop::Id::diseased,
        prop::Id::weakened,
    };

    bool is_noticable = false;

    for (prop::Id prop_id : props_can_heal) {
        if (actor.m_properties.end_prop(prop_id)) {
            is_noticable = true;
        }
    }

    const bool did_restore_hp =
        actor::restore_hp(
            actor,
            3,
            actor::AllowRestoreAboveMax::no);

    if (did_restore_hp) {
        is_noticable = true;
    }

    if (!is_noticable && actor::is_player(&actor)) {
        msg_log::add(i18n::get("item_potion.feel_fine", "I feel fine."));

        is_noticable = true;
    }

    if (is_noticable &&
        actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Curing::collide_hook(const P& pos, actor::Actor* actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Resistance::real_name() const
{
    return i18n::get("item_potion.resistance_name", "Resistance");
}

std::string Resistance::descr_identified() const
{
    return (
        i18n::get(
            "item_potion.resistance_descr",
            "Completely protects the consumer from electricity, fire and poison - "
            "and also prevents paralysis."));
}

PotionAlignment Resistance::alignment() const
{
    return PotionAlignment::good;
}

void Resistance::quaff_impl(actor::Actor& actor)
{
    apply_properties_with_equal_duration(
        {prop::Id::r_elec,
         prop::Id::r_fire,
         prop::Id::r_poison,
         prop::Id::r_para},
        actor);

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Resistance::collide_hook(const P& pos, actor::Actor* actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Descent::real_name() const
{
    return i18n::get("item_potion.descent_name", "Descent");
}

std::string Descent::descr_identified() const
{
    return (
        i18n::get(
            "item_potion.descent_descr",
            "A bizarre liquid that causes the consumer to "
            "dematerialize and sink through the ground."));
}

PotionAlignment Descent::alignment() const
{
    return PotionAlignment::good;
}

void Descent::quaff_impl(actor::Actor& actor)
{
    (void)actor;

    if (map::g_dlvl < (g_dlvl_last + 1)) {
        if (!map::g_player->m_properties.has(prop::Id::descend)) {
            map::g_player->m_properties.apply(prop::make(prop::Id::descend));
        }
    }
    else {
        // Cannot bypass the boss level.
        msg_log::add(i18n::get(
            "item_potion.sinking_sensation_disappears",
            "I feel a faint sinking sensation, but it soon disappears..."));
    }

    identify(Verbose::yes);
}

void Descent::collide_hook(const P& pos, actor::Actor* const actor)
{
    (void)pos;
    (void)actor;
}

std::string Skill::real_name() const
{
    return i18n::get("item_potion.skill_name", "Skill");
}

std::string Skill::descr_identified() const
{
    return (
        i18n::get(
            "item_potion.skill_descr",
            "The consumer becomes more skillful "
            "(+10% to hit chance, evasion, stealth, and searching)."));
}

PotionAlignment Skill::alignment() const
{
    return PotionAlignment::good;
}

void Skill::quaff_impl(actor::Actor& actor)
{
    actor.m_properties.apply(prop::make(prop::Id::extra_skill));

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Skill::collide_hook(const P& pos, actor::Actor* actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Carapace::real_name() const
{
    return i18n::get("item_potion.carapace_name", "Carapace");
}

std::string Carapace::descr_identified() const
{
    return (
        i18n::get(
            "item_potion.carapace_descr",
            "Causes a tough carapace to grow over the consumer's skin "
            "providing protection against physical attacks "
            "(+3 armor points), "
            "as well as some resistance against burning "
            "(+25% chance to resist burning)."));
}

PotionAlignment Carapace::alignment() const
{
    return PotionAlignment::good;
}

void Carapace::quaff_impl(actor::Actor& actor)
{
    actor.m_properties.apply(prop::make(prop::Id::magic_carapace));

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Carapace::collide_hook(const P& pos, actor::Actor* actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Blinking::real_name() const
{
    return i18n::get("item_potion.blinking_name", "Blinking");
}

std::string Blinking::descr_identified() const
{
    return (
        i18n::get(
            "item_potion.blinking_descr",
            "Causes the consumer to rapidly fade away from existence, "
            "and reappear again at a nearby position "
            "of their choosing."));
}

PotionAlignment Blinking::alignment() const
{
    return PotionAlignment::good;
}

void Blinking::quaff_impl(actor::Actor& actor)
{
    if (actor::is_player(&actor)) {
        msg_log::add(i18n::get(
            "item_potion.fade_out_and_reappear",
            "I fade out of existence and reappear."));
    }

    const int max_dist = 4;

    teleport(actor, ShouldCtrlTele::always, max_dist);

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Blinking::collide_hook(const P& pos, actor::Actor* actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

std::string Burrowing::real_name() const
{
    return i18n::get("item_potion.burrowing_name", "Burrowing");
}

std::string Burrowing::descr_identified() const
{
    return (
        i18n::get(
            "item_potion.burrowing_descr",
            "Infused with the alchemically treated blood of Chthonians, "
            "it grants the consumer the ability to burrow through earth and rock "
            "(can burrow through walls and rubble)."));
}

PotionAlignment Burrowing::alignment() const
{
    return PotionAlignment::good;
}

void Burrowing::quaff_impl(actor::Actor& actor)
{
    actor.m_properties.apply(prop::make(prop::Id::burrowing));

    if (actor::can_player_see_actor(actor)) {
        identify(Verbose::yes);
    }
}

void Burrowing::collide_hook(const P& pos, actor::Actor* actor)
{
    (void)pos;

    if (actor) {
        quaff_impl(*actor);
    }
}

}  // namespace potion
