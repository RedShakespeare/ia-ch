// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "terrain_gong.hpp"

#include <memory>
#include <vector>

#include "actor_factory.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "audio.hpp"
#include "common_text.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "item_scroll.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "player_spells.hpp"
#include "popup.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "query.hpp"
#include "random.hpp"
#include "saving.hpp"
#include "spells.hpp"
#include "terrain.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::unique_ptr<terrain::gong::Bonus> make_bonus(
        terrain::gong::BonusId id)
{
        switch (id) {
        case terrain::gong::BonusId::upgrade_spell:
                return std::make_unique<terrain::gong::UpgradeSpell>();

        case terrain::gong::BonusId::gain_hp:
                return std::make_unique<terrain::gong::GainHp>();

        case terrain::gong::BonusId::gain_sp:
                return std::make_unique<terrain::gong::GainSp>();

        case terrain::gong::BonusId::gain_xp:
                return std::make_unique<terrain::gong::GainXp>();

        case terrain::gong::BonusId::remove_insanity:
                return std::make_unique<terrain::gong::RemoveInsanity>();

        case terrain::gong::BonusId::gain_item:
                return std::make_unique<terrain::gong::GainItem>();

        case terrain::gong::BonusId::healed:
                return std::make_unique<terrain::gong::Healed>();

        case terrain::gong::BonusId::blessed:
                return std::make_unique<terrain::gong::Blessed>();

        case terrain::gong::BonusId::undefined:
        case terrain::gong::BonusId::END:
                break;
        }

        ASSERT(false);

        return nullptr;
}

static std::unique_ptr<terrain::gong::Toll> make_toll(terrain::gong::TollId id)
{
        switch (id) {
        case terrain::gong::TollId::hp_reduced:
                return std::make_unique<terrain::gong::HpReduced>();

        case terrain::gong::TollId::sp_reduced:
                return std::make_unique<terrain::gong::SpReduced>();

        case terrain::gong::TollId::xp_reduced:
                return std::make_unique<terrain::gong::XpReduced>();

        case terrain::gong::TollId::deaf:
                return std::make_unique<terrain::gong::Deaf>();

        case terrain::gong::TollId::cursed:
                return std::make_unique<terrain::gong::Cursed>();

        case terrain::gong::TollId::unlearn_spell:
                return std::make_unique<terrain::gong::UnlearnSpell>();

        case terrain::gong::TollId::spawn_monsters:
                return std::make_unique<terrain::gong::SpawnMonsters>();

        case terrain::gong::TollId::END:
                break;
        }

        ASSERT(false);

        return nullptr;
}

static std::vector<std::unique_ptr<terrain::gong::Bonus>>
make_all_allowed_bonuses()
{
        std::vector<std::unique_ptr<terrain::gong::Bonus>> bonuses;

        for (int i = 0; i < (int)terrain::gong::BonusId::END; ++i) {
                auto bonus = make_bonus((terrain::gong::BonusId)i);

                if (!bonus) {
                        ASSERT(false);

                        continue;
                }

                if (!bonus->is_allowed()) {
                        continue;
                }

                bonuses.push_back(std::move(bonus));
        }

        return bonuses;
}

static bool is_toll_blacklist_allowing_bonus(
        const terrain::gong::Toll& toll,
        const terrain::gong::BonusId bonus_id)
{
        const auto bonuses_not_allowed_with =
                toll.bonuses_not_allowed_with();

        const auto search =
                std::find(
                        std::begin(bonuses_not_allowed_with),
                        std::end(bonuses_not_allowed_with),
                        bonus_id);

        const bool is_in_blacklist =
                (search != std::end(bonuses_not_allowed_with));

        return !is_in_blacklist;
}

static bool is_toll_whitelist_allowing_bonus(
        const terrain::gong::Toll& toll,
        const terrain::gong::BonusId bonus_id)
{
        const auto bonuses_only_allowed_with =
                toll.bonuses_only_allowed_with();

        if (bonuses_only_allowed_with.empty()) {
                // The toll does not have a bonus whitelist
                return true;
        }

        // The toll has a bonus whitelist

        const auto search =
                std::find(
                        std::begin(bonuses_only_allowed_with),
                        std::end(bonuses_only_allowed_with),
                        bonus_id);

        const bool is_in_whitelist =
                (search != std::end(bonuses_only_allowed_with));

        return is_in_whitelist;
}

static bool is_toll_allowing_bonus(
        const terrain::gong::Toll& toll,
        const terrain::gong::BonusId bonus_id)
{
        if (!is_toll_blacklist_allowing_bonus(toll, bonus_id)) {
                return false;
        }

        if (!is_toll_whitelist_allowing_bonus(toll, bonus_id)) {
                return false;
        }

        return true;
}

static std::vector<std::unique_ptr<terrain::gong::Toll>> make_all_allowed_tolls(
        const terrain::gong::BonusId bonus_id)
{
        ASSERT((bonus_id != terrain::gong::BonusId::undefined));
        ASSERT((bonus_id != terrain::gong::BonusId::END));

        std::vector<std::unique_ptr<terrain::gong::Toll>> tolls;

        for (int i = 0; i < (int)terrain::gong::TollId::END; ++i) {
                auto toll = make_toll((terrain::gong::TollId)i);

                if (!toll) {
                        ASSERT(false);

                        continue;
                }

                if (!is_toll_allowing_bonus(*toll, bonus_id)) {
                        continue;
                }

                if (!toll->is_allowed()) {
                        continue;
                }

                tolls.push_back(std::move(toll));
        }

        return tolls;
}

static std::unique_ptr<terrain::gong::Bonus> make_random_allowed_bonus()
{
        auto bonus_bucket = make_all_allowed_bonuses();

        if (bonus_bucket.empty()) {
                return nullptr;
        }

        const auto idx = rnd::idx(bonus_bucket);

        auto bonus = std::move(bonus_bucket[idx]);

        return bonus;
}

static std::unique_ptr<terrain::gong::Toll> make_random_allowed_toll(
        const terrain::gong::BonusId bonus_id)
{
        auto toll_bucket = make_all_allowed_tolls(bonus_id);

        if (toll_bucket.empty()) {
                return nullptr;
        }

        const auto idx = rnd::idx(toll_bucket);

        auto toll = std::move(toll_bucket[idx]);

        return toll;
}

static void run_gong_effect()
{
        const auto bonus = make_random_allowed_bonus();

        if (!bonus) {
                return;
        }

        bonus->run_effect();

        const auto toll = make_random_allowed_toll(bonus->id());

        if (!toll) {
                return;
        }

        msg_log::more_prompt();

        toll->run_effect();
}

// -----------------------------------------------------------------------------
// terrain
// -----------------------------------------------------------------------------
namespace terrain {

// -----------------------------------------------------------------------------
// Gong
// -----------------------------------------------------------------------------
Gong::Gong(const P& p) :
        Terrain(p) {}

void Gong::bump(actor::Actor& actor_bumping)
{
        if (!actor_bumping.is_player()) {
                return;
        }

        if (!map::g_cells.at(m_pos).is_seen_by_player) {
                msg_log::clear();

                msg_log::add("There is a temple gong here.");

                if (!player_bon::is_bg(Bg::exorcist)) {
                        msg_log::add(
                                "Strike it? " + common_text::g_yes_or_no_hint,
                                colors::light_white(),
                                MsgInterruptPlayer::no,
                                MorePromptOnMsg::no,
                                CopyToMsgHistory::no);

                        const auto answer = query::yes_or_no();

                        if (answer == BinaryAnswer::no) {
                                msg_log::clear();

                                return;
                        }
                }
        }

        if (player_bon::is_bg(Bg::exorcist)) {
                msg_log::add("This unholy instrument must be destroyed!");

                return;
        }

        msg_log::add("I strike the temple gong!");

        Snd snd(
                "The crash resonates through the air!",
                audio::SfxId::gong,
                IgnoreMsgIfOriginSeen::no,
                m_pos,
                map::g_player,
                SndVol::high,
                AlertsMon::yes,
                MorePromptOnMsg::no);

        snd.run();

        if (m_is_used) {
                msg_log::add("Nothing happens.");
        } else {
                msg_log::more_prompt();

                run_gong_effect();

                m_is_used = true;
        }

        game_time::tick();
}

void Gong::on_hit(
        const DmgType dmg_type,
        actor::Actor* const actor,
        const int dmg)
{
        (void)dmg;
        (void)actor;

        switch (dmg_type) {
        case DmgType::explosion:
        case DmgType::pure:
                if (map::is_pos_seen_by_player(m_pos)) {
                        msg_log::add("The gong is destroyed.");
                }

                map::put(new RubbleLow(m_pos));
                map::update_vision();

                if (player_bon::is_bg(Bg::exorcist)) {
                        const auto msg =
                                rnd::element(
                                        common_text::g_exorcist_purge_phrases);

                        msg_log::add(msg);

                        game::incr_player_xp(10);

                        map::g_player->restore_sp(999, false);
                        map::g_player->restore_sp(10, true);
                }
                break;

        default:
                break;
        }
}

std::string Gong::name(const Article article) const
{
        std::string a = (article == Article::a) ? "a " : "the ";

        return a + "temple gong";
}

Color Gong::color_default() const
{
        return m_is_used
                ? colors::gray()
                : colors::brown();
}

// -----------------------------------------------------------------------------
// gong
// -----------------------------------------------------------------------------
namespace gong {

// -----------------------------------------------------------------------------
// Upgrade spell
// -----------------------------------------------------------------------------
UpgradeSpell::UpgradeSpell() :

        m_spell_id(SpellId::END)
{
        const auto bucket = find_spells_can_upgrade();

        if (!bucket.empty()) {
                m_spell_id = rnd::element(bucket);
        }
}

bool UpgradeSpell::is_allowed() const
{
        return m_spell_id != SpellId::END;
}

void UpgradeSpell::run_effect()
{
        player_spells::incr_spell_skill(m_spell_id);
}

std::vector<SpellId> UpgradeSpell::find_spells_can_upgrade() const
{
        std::vector<SpellId> spells;

        spells.reserve((size_t)SpellId::END);

        for (int i = 0; i < (int)SpellId::END; ++i) {
                const auto id = (SpellId)i;

                std::unique_ptr<const Spell> spell(spells::make(id));

                if (player_spells::is_spell_learned(id) &&
                    (player_spells::spell_skill(id) != SpellSkill::master) &&
                    spell->can_be_improved_with_skill()) {
                        spells.push_back(id);
                }
        }

        return spells;
}

// -----------------------------------------------------------------------------
// Gain HP
// -----------------------------------------------------------------------------
bool GainHp::is_allowed() const
{
        return true;
}

void GainHp::run_effect()
{
        map::g_player->change_max_hp(2);
}

// -----------------------------------------------------------------------------
// Gain SP
// -----------------------------------------------------------------------------
bool GainSp::is_allowed() const
{
        return true;
}

void GainSp::run_effect()
{
        map::g_player->change_max_sp(1);
}

// -----------------------------------------------------------------------------
// Gain XP
// -----------------------------------------------------------------------------
bool GainXp::is_allowed() const
{
        return game::xp_pct() < 50;
}

void GainXp::run_effect()
{
        msg_log::add("I feel more experienced.");

        game::incr_player_xp(50, Verbose::no);
}

// -----------------------------------------------------------------------------
// Remove insanity
// -----------------------------------------------------------------------------
bool RemoveInsanity::is_allowed() const
{
        return map::g_player->m_ins >= 25;
}

void RemoveInsanity::run_effect()
{
        msg_log::add("I feel more sane.");

        map::g_player->m_ins -= 25;
}

// -----------------------------------------------------------------------------
// Gain item
// -----------------------------------------------------------------------------
GainItem::GainItem() :

        m_item_id(item::Id::END)
{
        const auto item_ids = find_allowed_item_ids();

        if (!item_ids.empty()) {
                m_item_id = rnd::element(item_ids);
        }
}

bool GainItem::is_allowed() const
{
        return m_item_id != item::Id::END;
}

void GainItem::run_effect()
{
        auto* const item = item::make(m_item_id);

        item::set_item_randomized_properties(*item);

        const std::string name_a = item->name(ItemRefType::a);

        msg_log::add("I have received " + name_a + ".");

        map::g_player->m_inv.put_in_backpack(item);
}

std::vector<item::Id> GainItem::find_allowed_item_ids() const
{
        std::vector<item::Id> ids;

        for (size_t i = 0; i < (size_t)item::Id::END; ++i) {
                const auto& d = item::g_data[i];

                if (d.allow_spawn && d.value >= item::Value::supreme_treasure) {
                        ids.push_back((item::Id)i);
                }
        }

        return ids;
}

// -----------------------------------------------------------------------------
// Healed
// -----------------------------------------------------------------------------
bool Healed::is_allowed() const
{
        const auto& player = *map::g_player;

        if (player.m_properties.has(PropId::poisoned) && (player.m_hp <= 6)) {
                return true;
        }

        const auto* const prop = player.m_properties.prop(PropId::wound);

        if (prop) {
                const auto* const wound = static_cast<const PropWound*>(prop);

                if (wound->nr_wounds() >= 3) {
                        return true;
                }
        }

        return false;
}

void Healed::run_effect()
{
        std::vector<PropId> props_can_heal = {
                PropId::blind,
                PropId::deaf,
                PropId::poisoned,
                PropId::infected,
                PropId::diseased,
                PropId::weakened,
                PropId::hp_sap,
                PropId::wound};

        for (PropId prop_id : props_can_heal) {
                map::g_player->m_properties.end_prop(prop_id);
        }

        map::g_player->restore_hp(
                999, // HP restored
                false); // Not allowed above max
}

// -----------------------------------------------------------------------------
// Blessed
// -----------------------------------------------------------------------------
bool Blessed::is_allowed() const
{
        const bool is_blessed =
                map::g_player->m_properties.has(PropId::blessed);

        const bool has_cursed_item =
                (get_random_cursed_item() != nullptr);

        return !is_blessed || has_cursed_item;
}

void Blessed::run_effect()
{
        auto* const blessed = property_factory::make(PropId::blessed);

        blessed->set_indefinite();

        map::g_player->m_properties.apply(blessed);

        auto* const cursed_item = get_random_cursed_item();

        if (cursed_item) {
                const auto name =
                        cursed_item->name(
                                ItemRefType::plain,
                                ItemRefInf::none);

                msg_log::add("The " + name + " seems cleansed!");

                cursed_item->current_curse().on_curse_end();

                cursed_item->remove_curse();
        }
}

item::Item* Blessed::get_random_cursed_item() const
{
        std::vector<item::Item*> cursed_items;

        for (const auto& slot : map::g_player->m_inv.m_slots) {
                if (slot.item && slot.item->is_cursed()) {
                        cursed_items.push_back(slot.item);
                }
        }

        for (auto* const item : map::g_player->m_inv.m_backpack) {
                if (item->is_cursed()) {
                        cursed_items.push_back(item);
                }
        }

        if (cursed_items.empty()) {
                return nullptr;
        } else {
                return rnd::element(cursed_items);
        }
}

// -----------------------------------------------------------------------------
// HP reduced
// -----------------------------------------------------------------------------
std::vector<BonusId> HpReduced::bonuses_only_allowed_with() const
{
        return {BonusId::gain_sp};
}

void HpReduced::run_effect()
{
        map::g_player->change_max_hp(-2);
}

// -----------------------------------------------------------------------------
// SP reduced
// -----------------------------------------------------------------------------
std::vector<BonusId> SpReduced::bonuses_only_allowed_with() const
{
        return {BonusId::gain_hp};
}

void SpReduced::run_effect()
{
        map::g_player->change_max_sp(-1);
}

// -----------------------------------------------------------------------------
// XP reduced
// -----------------------------------------------------------------------------
bool XpReduced::is_allowed() const
{
        return game::xp_pct() >= 50;
}

std::vector<BonusId> XpReduced::bonuses_not_allowed_with() const
{
        return {BonusId::gain_xp};
}

void XpReduced::run_effect()
{
        msg_log::add("I feel less experienced.");

        game::decr_player_xp(50);
}

// -----------------------------------------------------------------------------
// Deaf
// -----------------------------------------------------------------------------
bool Deaf::is_allowed() const
{
        const auto prop = map::g_player->m_properties.prop(PropId::deaf);

        return !prop || (prop->duration_mode() != PropDurationMode::indefinite);
}

void Deaf::run_effect()
{
        auto* const deaf = property_factory::make(PropId::deaf);

        deaf->set_indefinite();

        map::g_player->m_properties.apply(deaf);
}

// -----------------------------------------------------------------------------
// Cursed
// -----------------------------------------------------------------------------
bool Cursed::is_allowed() const
{
        const auto prop = map::g_player->m_properties.prop(PropId::cursed);

        return !prop || (prop->duration_mode() != PropDurationMode::indefinite);
}

std::vector<BonusId> Cursed::bonuses_not_allowed_with() const
{
        return {BonusId::blessed};
}

void Cursed::run_effect()
{
        auto* const cursed = property_factory::make(PropId::cursed);

        cursed->set_indefinite();

        map::g_player->m_properties.apply(cursed);
}

// -----------------------------------------------------------------------------
// Spawn monsters
// -----------------------------------------------------------------------------
SpawnMonsters::SpawnMonsters()

{
        std::vector<actor::Id> summon_bucket;

        summon_bucket.reserve((size_t)actor::Id::END);

        for (int i = 0; i < (int)actor::Id::END; ++i) {
                const actor::ActorData& data = actor::g_data[i];

                if (data.can_be_summoned_by_mon) {
                        if (data.spawn_min_dlvl <= (map::g_dlvl + 3)) {
                                summon_bucket.push_back(actor::Id(i));
                        }
                }
        }

        if (!summon_bucket.empty()) {
                m_id_to_spawn = rnd::element(summon_bucket);
        }
}

bool SpawnMonsters::is_allowed() const
{
        return m_id_to_spawn != actor::Id::END;
}

void SpawnMonsters::run_effect()
{
        if (m_id_to_spawn == actor::Id::END) {
                ASSERT(false);

                return;
        }

        const size_t nr_mon = rnd::range(3, 4);

        const auto mon_summoned =
                actor::spawn(
                        map::g_player->m_pos,
                        {nr_mon, m_id_to_spawn},
                        map::rect())
                        .make_aware_of_player();

        std::for_each(
                std::begin(mon_summoned.monsters),
                std::end(mon_summoned.monsters),
                [](auto* const mon) {
                        auto prop_waiting = new PropWaiting();

                        prop_waiting->set_duration(2);

                        mon->m_properties.apply(prop_waiting);
                });
}

// -----------------------------------------------------------------------------
// Unlearn spell
// -----------------------------------------------------------------------------
UnlearnSpell::UnlearnSpell()
{
        const auto spell_bucket = make_spell_bucket();

        if (!spell_bucket.empty()) {
                m_spell_to_unlearn = rnd::element(spell_bucket);
        }
}

std::vector<BonusId> UnlearnSpell::bonuses_not_allowed_with() const
{
        return {BonusId::upgrade_spell};
}

bool UnlearnSpell::is_allowed() const
{
        return m_spell_to_unlearn != SpellId::END;
}

void UnlearnSpell::run_effect()
{
        player_spells::unlearn_spell(m_spell_to_unlearn, Verbose::yes);
}

std::vector<SpellId> UnlearnSpell::make_spell_bucket() const
{
        std::vector<SpellId> result;

        // Find all spells which have scrolls with low spawn chances
        std::vector<SpellId> low_spawn_spells;

        for (size_t i = 0; i < (size_t)item::Id::END; ++i) {
                const auto& d = item::g_data[i];

                const bool is_scroll = d.type == ItemType::scroll;

                const bool is_low_chance =
                        d.chance_to_incl_in_spawn_list ==
                        scroll::g_low_spawn_chance;

                if (is_scroll && is_low_chance) {
                        if (d.spell_cast_from_scroll == SpellId::END) {
                                ASSERT(false);

                                continue;
                        }

                        low_spawn_spells.push_back(d.spell_cast_from_scroll);
                }
        }

        ASSERT(!low_spawn_spells.empty());

        // Get all learned spells from the low spawn chance spells
        for (const auto id : low_spawn_spells) {
                if (player_spells::is_spell_learned(id)) {
                        result.push_back(id);
                }
        }

        return result;
}

} // namespace gong

} // namespace terrain
