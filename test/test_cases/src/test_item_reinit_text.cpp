// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "catch.hpp"
#include "config.hpp"
#include "i18n.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "item_potion.hpp"
#include "item_rod.hpp"
#include "item_scroll.hpp"
#include "test_utils.hpp"

namespace
{

struct NameTriple
{
    std::string plain;
    std::string plural;
    std::string a;
};

struct Fixture
{
    Fixture()
    {
        test_utils::init_all();
    }

    ~Fixture()
    {
        config::set_language("en");
        i18n::reload();
        test_utils::cleanup_all();
    }
};

struct SessionSnapshot
{
    bool is_identified;
    bool is_alignment_known;
    bool is_spell_domain_known;
    bool is_tried;
    bool is_found;
    bool allow_spawn;
    int chance_to_incl_in_spawn_list;
    int fake_appearance_idx;
};

struct AppearanceSnapshot
{
    item::Id id;
    NameTriple un_id;
    NameTriple id_name;
    Color color;
    int fake_idx;
};

NameTriple snapshot_un_id(const item::ItemData& d)
{
    return {
        d.text.base_name_un_id.names[(size_t)ItemNameType::plain],
        d.text.base_name_un_id.names[(size_t)ItemNameType::plural],
        d.text.base_name_un_id.names[(size_t)ItemNameType::a]};
}

NameTriple snapshot_id(const item::ItemData& d)
{
    return {
        d.text.base_name.names[(size_t)ItemNameType::plain],
        d.text.base_name.names[(size_t)ItemNameType::plural],
        d.text.base_name.names[(size_t)ItemNameType::a]};
}

SessionSnapshot snapshot_session(const item::ItemData& d)
{
    return {
        d.session.is_identified,
        d.session.is_alignment_known,
        d.session.is_spell_domain_known,
        d.session.is_tried,
        d.session.is_found,
        d.session.allow_spawn,
        d.session.chance_to_incl_in_spawn_list,
        d.session.fake_appearance_idx};
}

void require_name_triple(const NameTriple& actual, const NameTriple& expected)
{
    REQUIRE(actual.plain == expected.plain);
    REQUIRE(actual.plural == expected.plural);
    REQUIRE(actual.a == expected.a);
}

void require_session(const item::Id id, const SessionSnapshot& expected)
{
    const item::ItemData& actual = item::g_data[(size_t)id];

    INFO("id=" << (int)id);
    REQUIRE(actual.session.is_identified == expected.is_identified);
    REQUIRE(actual.session.is_alignment_known == expected.is_alignment_known);
    REQUIRE(actual.session.is_spell_domain_known == expected.is_spell_domain_known);
    REQUIRE(actual.session.is_tried == expected.is_tried);
    REQUIRE(actual.session.is_found == expected.is_found);
    REQUIRE(actual.session.allow_spawn == expected.allow_spawn);
    REQUIRE(actual.session.chance_to_incl_in_spawn_list ==
            expected.chance_to_incl_in_spawn_list);
    REQUIRE(actual.session.fake_appearance_idx == expected.fake_appearance_idx);
}

void refresh_all_item_localized_text(const std::string& language)
{
    config::set_language(language);
    i18n::reload();
    item::refresh_localized_text();
    scroll::refresh_localized_text();
    potion::refresh_localized_text();
    rod::refresh_localized_text();
}

void mutate_session_fields(
    const item::Id id,
    const int chance_to_include,
    const bool mutate_fake_idx)
{
    item::ItemData& d = item::g_data[(size_t)id];

    d.session.is_identified = !d.session.is_identified;
    d.session.is_alignment_known = !d.session.is_alignment_known;
    d.session.is_spell_domain_known = !d.session.is_spell_domain_known;
    d.session.is_tried = true;
    d.session.is_found = true;
    d.session.allow_spawn = false;
    d.session.chance_to_incl_in_spawn_list = chance_to_include;

    if (mutate_fake_idx) {
        d.session.fake_appearance_idx = chance_to_include + 100;
    }
}

bool ends_with(const std::string& text, const std::string& suffix)
{
    return (text.size() >= suffix.size()) &&
           (text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0);
}

}  // namespace

// refresh_localized_text() must reproduce the names init() wrote, without disturbing
// the per-session fake_appearance_idx. Snapshot the post-init names for
// every scroll/potion/rod, call refresh_localized_text(), and require equality.
TEST_CASE("scroll/potion/rod localized text refresh reproduces init names")
{
    Fixture fixture;

    std::vector<AppearanceSnapshot> entries;

    for (const item::ItemData& d : item::g_data) {
        if ((d.type != ItemType::scroll) &&
            (d.type != ItemType::potion) &&
            (d.type != ItemType::rod)) {
            continue;
        }

        entries.push_back(
            {d.id, snapshot_un_id(d), snapshot_id(d), d.color, d.session.fake_appearance_idx});
    }

    REQUIRE(!entries.empty());

    scroll::refresh_localized_text();
    potion::refresh_localized_text();
    rod::refresh_localized_text();

    for (const AppearanceSnapshot& e : entries) {
        const item::ItemData& d = item::g_data[(size_t)e.id];

        // fake_appearance_idx must be preserved.
        REQUIRE(d.session.fake_appearance_idx == e.fake_idx);
        REQUIRE(d.color == e.color);

        const auto un_id = snapshot_un_id(d);
        const auto id_name = snapshot_id(d);

        INFO("id=" << (int)e.id << " plain_un_id=[" << un_id.plain << "]");
        require_name_triple(un_id, e.un_id);

        INFO("id=" << (int)e.id << " plain_id=[" << id_name.plain << "]");
        require_name_triple(id_name, e.id_name);
    }
}

TEST_CASE("item localized text refresh preserves item session state")
{
    Fixture fixture;

    mutate_session_fields(item::Id::pistol, 11, true);
    mutate_session_fields(item::Id::device_blaster, 12, true);
    mutate_session_fields(item::Id::scroll_identify, 13, false);
    mutate_session_fields(item::Id::potion_vitality, 14, false);
    mutate_session_fields(item::Id::rod_opening, 15, false);

    std::unique_ptr<item::Item> necronomicon(
        item::make(item::Id::necronomicon, 1));
    REQUIRE(necronomicon != nullptr);

    std::vector<SessionSnapshot> expected((size_t)item::Id::END);

    for (size_t i = 0; i < (size_t)item::Id::END; ++i) {
        expected[i] = snapshot_session(item::g_data[i]);
    }

    refresh_all_item_localized_text("zh_CN");

    for (size_t i = 0; i < (size_t)item::Id::END; ++i) {
        require_session(item::Id(i), expected[i]);
    }
}

TEST_CASE("item localized text refresh relocalizes cached item text")
{
    Fixture fixture;

    refresh_all_item_localized_text("zh_CN");

    const auto& pistol = item::g_data[(size_t)item::Id::pistol];
    REQUIRE(
        pistol.text.base_name.names[(size_t)ItemNameType::plain] ==
        i18n::get("item_data.pistol.name", "M1911 Colt"));
    REQUIRE(
        pistol.text.base_name.names[(size_t)ItemNameType::plural] ==
        i18n::get("item_data.pistol.name_plural", "M1911 Colts"));
    REQUIRE(
        pistol.text.base_name.names[(size_t)ItemNameType::a] ==
        i18n::get("item_data.pistol.name_a", "an M1911 Colt"));
    REQUIRE(
        pistol.text.base_descr.at(0) ==
        i18n::get(
            "item_data.pistol.base_descr",
            "A semi-automatic, magazine-fed pistol chambered for the .45 "
            "ACP cartridge."));
    REQUIRE(
        pistol.text.melee_attack_msgs.player ==
        i18n::get("item_data.attack.strike.player", "strike"));
    REQUIRE(
        pistol.text.ranged_attack_msgs.player ==
        i18n::get("item_data.attack.fire.player", "fire"));
    REQUIRE(
        pistol.text.ranged_snd_msg ==
        i18n::get("item_data.pistol.ranged_snd_msg", "I hear a pistol being fired."));

    const auto& device = item::g_data[(size_t)item::Id::device_blaster];
    REQUIRE(
        device.text.base_name_un_id.names[(size_t)ItemNameType::plain] ==
        i18n::get("item_data.item_type.device.unidentified_name", "Strange Device"));
    REQUIRE(
        device.text.base_name_un_id.names[(size_t)ItemNameType::a] ==
        i18n::get("item_data.item_type.device.unidentified_name_a", "a Strange Device"));

    const auto& scroll = item::g_data[(size_t)item::Id::scroll_identify];
    REQUIRE(
        scroll.text.base_descr.at(0) ==
        i18n::get(
            "item_data.item_type.scroll.base_descr_1",
            "A short transcription of an eldritch incantation. "
            "There is a strange aura about it, as if some power "
            "was imbued in the paper itself."));
    REQUIRE(
        scroll.text.base_name_un_id.names[(size_t)ItemNameType::plain].find(
            i18n::get("item_scroll.manuscript_titled_prefix", "Manuscript titled ")) ==
        0);
    REQUIRE(ends_with(
        scroll.text.base_name_un_id.names[(size_t)ItemNameType::a],
        i18n::get("item_scroll.a_manuscript_titled_suffix", "")));

    const auto& potion = item::g_data[(size_t)item::Id::potion_vitality];
    REQUIRE(ends_with(
        potion.text.base_name_un_id.names[(size_t)ItemNameType::plain],
        i18n::get("item_potion.unidentified_suffix", " Potion")));
    REQUIRE(
        potion.text.base_name.names[(size_t)ItemNameType::plain].find(
            i18n::get("item_potion.real_name_prefix", "Potion of ")) ==
        0);

    const auto& rod = item::g_data[(size_t)item::Id::rod_opening];
    REQUIRE(ends_with(
        rod.text.base_name_un_id.names[(size_t)ItemNameType::plain],
        i18n::get("item_rod.rod_suffix", " Rod")));
    REQUIRE(
        rod.text.base_name.names[(size_t)ItemNameType::plain].find(
            i18n::get("item_rod.rod_of_prefix", "Rod of ")) ==
        0);
}

TEST_CASE("item localized text refresh preserves fake appearance mapping across languages")
{
    Fixture fixture;

    std::vector<AppearanceSnapshot> entries;

    for (const item::ItemData& d : item::g_data) {
        if ((d.type != ItemType::scroll) &&
            (d.type != ItemType::potion) &&
            (d.type != ItemType::rod)) {
            continue;
        }

        entries.push_back(
            {d.id, snapshot_un_id(d), snapshot_id(d), d.color, d.session.fake_appearance_idx});
    }

    REQUIRE(!entries.empty());

    refresh_all_item_localized_text("zh_CN");

    for (const AppearanceSnapshot& e : entries) {
        const item::ItemData& d = item::g_data[(size_t)e.id];

        INFO("id=" << (int)e.id);
        REQUIRE(d.session.fake_appearance_idx == e.fake_idx);
        REQUIRE(d.color == e.color);
        REQUIRE(snapshot_un_id(d).plain != e.un_id.plain);
        REQUIRE(snapshot_id(d).plain != e.id_name.plain);
    }

    refresh_all_item_localized_text("en");

    for (const AppearanceSnapshot& e : entries) {
        const item::ItemData& d = item::g_data[(size_t)e.id];

        INFO("id=" << (int)e.id);
        REQUIRE(d.session.fake_appearance_idx == e.fake_idx);
        REQUIRE(d.color == e.color);
        require_name_triple(snapshot_un_id(d), e.un_id);
        require_name_triple(snapshot_id(d), e.id_name);
    }
}
