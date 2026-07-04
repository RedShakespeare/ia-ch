// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <cstddef>
#include <string>
#include <vector>

#include "catch.hpp"
#include "item_data.hpp"
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

NameTriple snapshot_un_id(const item::ItemData& d)
{
    return {
        d.base_name_un_id.names[(size_t)ItemNameType::plain],
        d.base_name_un_id.names[(size_t)ItemNameType::plural],
        d.base_name_un_id.names[(size_t)ItemNameType::a]};
}

NameTriple snapshot_id(const item::ItemData& d)
{
    return {
        d.base_name.names[(size_t)ItemNameType::plain],
        d.base_name.names[(size_t)ItemNameType::plural],
        d.base_name.names[(size_t)ItemNameType::a]};
}

}  // namespace

// reinit_text() must reproduce the names init() wrote, without disturbing
// the per-session fake_appearance_idx. Snapshot the post-init names for
// every scroll/potion/rod, call reinit_text(), and require equality.
TEST_CASE("scroll/potion/rod reinit_text reproduces init names")
{
    test_utils::init_all();

    struct Entry
    {
        item::Id id;
        NameTriple un_id;
        NameTriple id_name;
        int fake_idx;
    };

    std::vector<Entry> entries;

    for (const item::ItemData& d : item::g_data) {
        if ((d.type != ItemType::scroll) &&
            (d.type != ItemType::potion) &&
            (d.type != ItemType::rod)) {
            continue;
        }

        entries.push_back({d.id, snapshot_un_id(d), snapshot_id(d), d.fake_appearance_idx});
    }

    REQUIRE(!entries.empty());

    scroll::reinit_text();
    potion::reinit_text();
    rod::reinit_text();

    for (const Entry& e : entries) {
        const item::ItemData& d = item::g_data[(size_t)e.id];

        // fake_appearance_idx must be preserved.
        REQUIRE(d.fake_appearance_idx == e.fake_idx);

        const auto un_id = snapshot_un_id(d);
        const auto id_name = snapshot_id(d);

        INFO("id=" << (int)e.id << " plain_un_id=[" << un_id.plain << "]");
        REQUIRE(un_id.plain == e.un_id.plain);
        REQUIRE(un_id.plural == e.un_id.plural);
        REQUIRE(un_id.a == e.un_id.a);

        INFO("id=" << (int)e.id << " plain_id=[" << id_name.plain << "]");
        REQUIRE(id_name.plain == e.id_name.plain);
        REQUIRE(id_name.plural == e.id_name.plural);
        REQUIRE(id_name.a == e.id_name.a);
    }
}
