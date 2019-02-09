// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "pact.hpp"

#include <memory>
#include <vector>

#include "actor_player.hpp"
#include "audio.hpp"
#include "game.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "player_spells.hpp"
#include "popup.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "random.hpp"
#include "saving.hpp"
#include "text_format.hpp"

namespace pact
{

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<std::unique_ptr<Toll>> s_waiting_tolls;


static std::unique_ptr<Benefit> make_benefit(BenefitId id)
{
        switch (id)
        {
        case BenefitId::upgrade_spell:
                return std::make_unique<UpgradeSpell>(id);

        case BenefitId::gain_hp:
                return std::make_unique<GainHp>(id);

        case BenefitId::gain_sp:
                return std::make_unique<GainSp>(id);

        case BenefitId::gain_xp:
                return std::make_unique<GainXp>(id);

        case BenefitId::remove_insanity:
                return std::make_unique<RemoveInsanity>(id);

        case BenefitId::gain_item:
                return std::make_unique<GainItem>(id);

        // case BenefitId::recharge_item:
        //         return std::make_unique<RechargeItem>(id);

        case BenefitId::healed:
                return std::make_unique<Healed>(id);

        case BenefitId::blessed:
                return std::make_unique<Blessed>(id);

        case BenefitId::hasted:
                return std::make_unique<Hasted>(id);

        case BenefitId::START_OF_BENEFITS:
        case BenefitId::undefined:
        case BenefitId::END:
                break;
        }

        ASSERT(false);

        return nullptr;
}

static std::unique_ptr<Toll> make_toll(TollId id)
{
        switch (id)
        {
        case TollId::hp_reduced:
                return std::make_unique<HpReduced>(id);

        case TollId::sp_reduced:
                return std::make_unique<SpReduced>(id);

        case TollId::xp_reduced:
                return std::make_unique<XpReduced>(id);

        // case TollId::unlearn_spell:
        //         return std::make_unique<UnlearnSpell>(id);

        case TollId::slowed:
                return std::make_unique<Slowed>(id);

        case TollId::blind:
                return std::make_unique<Blind>(id);

        case TollId::deaf:
                return std::make_unique<Deaf>(id);

        case TollId::cursed:
                return std::make_unique<Cursed>(id);

        case TollId::END:
                break;
        }

        ASSERT(false);

        return nullptr;
}

static std::vector<std::unique_ptr<Benefit>> make_all_benefits_can_be_offered()
{
        std::vector< std::unique_ptr<Benefit> > benefits;

        for (int i = (int)BenefitId::START_OF_BENEFITS + 1;
             i < (int)BenefitId::END;
             ++i)
        {
                auto benefit = make_benefit((BenefitId)i);

                // Robustness for release mode, should not happen
                if (!benefit)
                {
                        continue;
                }

                if (!benefit->is_allowed_to_offer_now())
                {
                        continue;
                }

                benefits.push_back(std::move(benefit));
        }

        return benefits;
}

static std::unique_ptr<Benefit> make_random_benefit_can_be_offered()
{
        auto benefit_bucket = make_all_benefits_can_be_offered();

        if (benefit_bucket.empty())
        {
                return nullptr;
        }
        else
        {
                const auto idx = rnd::idx(benefit_bucket);

                auto benefit = std::move(benefit_bucket[idx]);

                return benefit;
        }
}

static bool is_toll_allowing_benefit(
        const Toll& toll,
        const BenefitId benefit_id)
{
        const auto benefits_not_allowed_with =
                toll.benefits_not_allowed_with();

        const bool is_allowing_benefit =
                std::find(
                        std::begin(benefits_not_allowed_with),
                        std::end(benefits_not_allowed_with),
                        benefit_id)
                == std::end(benefits_not_allowed_with);

        return is_allowing_benefit;
}

static std::vector<std::unique_ptr<Toll>> make_all_tolls_can_be_offered(
        const BenefitId benefit_id_accepted)
{
        ASSERT((benefit_id_accepted != BenefitId::undefined));
        ASSERT((benefit_id_accepted != BenefitId::START_OF_BENEFITS));
        ASSERT((benefit_id_accepted != BenefitId::END));

        std::vector<std::unique_ptr<Toll>> tolls;

        for (int i = 0; i < (int)TollId::END; ++i)
        {
                auto toll = make_toll((TollId)i);

                // Robustness for release mode, should not happen
                if (!toll)
                {
                        continue;
                }

                if (!is_toll_allowing_benefit(*toll.get(), benefit_id_accepted))
                {
                        continue;
                }

                if (!toll->is_allowed_to_offer_now())
                {
                        continue;
                }

                tolls.push_back(std::move(toll));
        }

        return tolls;
}

// -----------------------------------------------------------------------------
// Public
// -----------------------------------------------------------------------------
void init()
{
        cleanup();
}

void cleanup()
{
        s_waiting_tolls.resize(0);
}

void save()
{
        saving::put_int(s_waiting_tolls.size());

        for (const auto& toll : s_waiting_tolls)
        {
                saving::put_int((int)toll->id());
        }
}

void load()
{
        const size_t nr_tolls = saving::get_int();

        for (size_t i = 0; i < nr_tolls; ++i)
        {
                const int id = saving::get_int();

                auto toll = make_toll((TollId)id);

                s_waiting_tolls.push_back(std::move(toll));
        }
}

void offer_pact_to_player()
{
        auto benefit = make_random_benefit_can_be_offered();

        if (!benefit)
        {
                ASSERT(false);

                return;
        }

        std::string msg =
                "Do you not desire to ease this mortal coil? "
                "Hear my offer: ";

        msg += benefit->offer_msg();

        msg += " Yet know that a toll must be paid in return.";

        const int choice =
                popup::menu(
                        msg,
                        {"Accept the offer", "Refuse"},
                        "A dark pact", // Title
                        5, // Width change
                        SfxId::END); // TODO: Play sound

        if (choice == 1)
        {
                msg_log::add("Whispering voice: \"How disappointing.\"");

                return;
        }

        benefit->run_effect();

        game::add_history_event("Signed a dark pact.");

        auto toll_bucket = make_all_tolls_can_be_offered(benefit->id());

        if (toll_bucket.empty())
        {
                // NOTE: We assume that there are always several tolls which can
                // be offered, so there is no need for a special message or
                // anything here - we just make sure that the game won't crash
                // on release builds if this assumption is wrong

                ASSERT(false);

                return;
        }

        rnd::shuffle(toll_bucket);

        const int max_nr_tolls_to_offer = 3;

        const int nr_tolls_to_offer =
                std::min(
                        (int)toll_bucket.size(),
                        max_nr_tolls_to_offer);

        for (int toll_idx = 0; toll_idx < nr_tolls_to_offer; ++toll_idx)
        {
                msg_log::clear();

                io::clear_screen();

                states::draw();

                const auto& toll = toll_bucket[toll_idx];

                bool is_accepted = true;

                if (toll_idx < (nr_tolls_to_offer - 1))
                {
                        std::string msg = "";

                        if (toll_idx == 0)
                        {
                                // This is the first toll offer
                                msg = "Now you must consider the price. ";
                        }
                        else
                        {
                                // This is not the first offer, adjust the
                                // message as a reply to the previous choice
                                msg =
                                        "As you wish, I shall give you "
                                        "another choice. ";
                        }

                        msg += "When the time comes, ";

                        msg += toll->offer_msg();

                        const int choice = popup::menu(
                                msg,
                                {"Accept the toll", "Refuse"},
                                "A dark pact", // Title,
                                5, // Width change
                                SfxId::END); // TODO: Play sound

                        if (choice == 1)
                        {
                                is_accepted = false;
                        }
                }
                else
                {
                        std::string msg =
                                "Then the price shall be this: when the time "
                                "comes, ";

                        msg += toll->offer_msg();

                        popup::msg(
                                msg,
                                "A dark pact", // Title
                                SfxId::END, // TODO: Add sound effect
                                5); // Width change
                }

                if (is_accepted)
                {
                        auto it = std::begin(toll_bucket) + toll_idx;

                        s_waiting_tolls.push_back(std::move(*it));

                        msg_log::add(
                                "Whispering voice: \"And so it is done!\"");

                        audio::play(SfxId::thunder);

                        break;
                }
        }

        map::g_player->incr_shock(ShockLvl::terrifying, ShockSrc::misc);

        return;
}

void on_player_reached_new_dlvl()
{
        for (auto& toll : s_waiting_tolls)
        {
                toll->on_player_reached_new_dlvl();
        }
}

void on_player_turn()
{
        for (auto it = std::begin(s_waiting_tolls);
             it != std::end(s_waiting_tolls);
             /* No incremenet */)
        {
                const auto& toll = *it;

                const auto is_done = toll->on_player_turn();

                if (is_done == TollDone::yes)
                {
                        s_waiting_tolls.erase(it);
                }
                else
                {
                        ++it;
                }
        }
}

// -----------------------------------------------------------------------------
// Toll
// -----------------------------------------------------------------------------
Toll::Toll(TollId id) :
        m_id(id),
        m_dlvl_countdown(rnd::range(1, 3)),
        m_turn_countdown(rnd::range(100, 300))
{
}

void Toll::on_player_reached_new_dlvl()
{
        if (m_dlvl_countdown > 0)
        {
                --m_dlvl_countdown;
        }
}

TollDone Toll::on_player_turn()
{
        if (m_dlvl_countdown > 0)
        {
                return TollDone::no;
        }

        if (m_turn_countdown > 0)
        {
                --m_turn_countdown;
        }

        if ((m_turn_countdown <= 0) && is_allowed_to_apply_now())
        {
                std::string msg = "The time has come to pay your toll. ";

                msg += offer_msg();

                popup::msg(
                        msg,
                        "A dark pact", // Title
                        SfxId::thunder,
                        5); // Width change

                run_effect();

                return TollDone::yes;
        }
        else
        {
                return TollDone::no;
        }
}

// -----------------------------------------------------------------------------
// Upgrade spell
// -----------------------------------------------------------------------------
UpgradeSpell::UpgradeSpell(BenefitId id) :
        Benefit(id),
        m_spell_id(SpellId::END)
{
        const auto bucket = find_spells_can_upgrade();

        if (!bucket.empty())
        {
                m_spell_id = rnd::element(bucket);
        }
}

std::string UpgradeSpell::offer_msg() const
{
        const std::unique_ptr<Spell> spell(
                spell_factory::make_spell_from_id(m_spell_id));

        const auto name = text_format::first_to_upper(spell->name());

        return
                "I can enhance your MAGIC, allowing you to cast "
                "\"" +
                name +
                "\" at a higher level.";
}

bool UpgradeSpell::is_allowed_to_offer_now() const
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

        for (int i = 0; i < (int)SpellId::END; ++i)
        {
                const auto id = (SpellId)i;

                if (player_spells::is_spell_learned(id) &&
                    (player_spells::spell_skill(id) != SpellSkill::master))
                {
                        spells.push_back(id);
                }
        }

        return spells;
}

// -----------------------------------------------------------------------------
// Gain HP
// -----------------------------------------------------------------------------
GainHp::GainHp(BenefitId id) :
        Benefit(id)
{

}

std::string GainHp::offer_msg() const
{
        return "I can elevate your HEALTH (+2 maximum hit points).";
}

bool GainHp::is_allowed_to_offer_now() const
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
GainSp::GainSp(BenefitId id) :
        Benefit(id)
{

}

std::string GainSp::offer_msg() const
{
        return "I can elevate your SPIRIT (+2 maximum spirit points).";
}

bool GainSp::is_allowed_to_offer_now() const
{
        return true;
}

void GainSp::run_effect()
{
        map::g_player->change_max_sp(2);
}

// -----------------------------------------------------------------------------
// Gain XP
// -----------------------------------------------------------------------------
GainXp::GainXp(BenefitId id) :
        Benefit(id)
{

}

std::string GainXp::offer_msg() const
{
        return "I can advance your EXPERIENCE (+50% experience).";
}

bool GainXp::is_allowed_to_offer_now() const
{
        // We never offer this benefit if the player would gain a clvl from it -
        // this looks confusing, since you only get to pick a new trait after
        // picking the pact toll
        return game::xp_pct() < 50;
}

void GainXp::run_effect()
{
        game::incr_player_xp(50, Verbosity::silent);
}

// -----------------------------------------------------------------------------
// Remove insanity
// -----------------------------------------------------------------------------
RemoveInsanity::RemoveInsanity(BenefitId id) :
        Benefit(id)
{

}

std::string RemoveInsanity::offer_msg() const
{
        return "I can give you SANITY (-25% insanity).";
}

bool RemoveInsanity::is_allowed_to_offer_now() const
{
        return map::g_player->m_ins >= 25;
}

void RemoveInsanity::run_effect()
{
        map::g_player->m_ins -= 25;
}

// -----------------------------------------------------------------------------
// Gain item
// -----------------------------------------------------------------------------
GainItem::GainItem(BenefitId id) :
        Benefit(id),
        m_item_id(ItemId::END)
{
        const auto item_ids = find_allowed_item_ids();

        if (!item_ids.empty())
        {
                m_item_id = rnd::element(item_ids);
        }
}

std::string GainItem::offer_msg() const
{
        return "I can grant you an artifact of considerable power.";
}

bool GainItem::is_allowed_to_offer_now() const
{
        return m_item_id != ItemId::END;
}

void GainItem::run_effect()
{
        auto* const item = item_factory::make(m_item_id);

        const std::string name_a = item->name(ItemRefType::a);

        msg_log::add("I have received " + name_a + ".");

        map::g_player->m_inv.put_in_backpack(item);
}

std::vector<ItemId> GainItem::find_allowed_item_ids() const
{
        std::vector<ItemId> ids;

        for (size_t i = 0; i < (size_t)ItemId::END; ++i)
        {
                const auto& d = item_data::g_data[i];

                if (d.allow_spawn && d.value >= ItemValue::supreme_treasure)
                {
                        ids.push_back((ItemId)i);
                }
        }

        return ids;
}

// -----------------------------------------------------------------------------
// Recharge item
// -----------------------------------------------------------------------------
// RechargeItem::RechargeItem(BenefitId id) :
//         Benefit(id)
// {

// }

// std::string RechargeItem::offer_msg() const
// {
//         return "";
// }

// bool RechargeItem::is_allowed_to_offer_now() const
// {
//         return true;
// }

// void RechargeItem::run_effect()
// {

// }

// -----------------------------------------------------------------------------
// Healed
// -----------------------------------------------------------------------------
Healed::Healed(BenefitId id) :
        Benefit(id)
{

}

std::string Healed::offer_msg() const
{
        return
                "I can make you fully healed and cleansed of all physical "
                "maladies.";
}

bool Healed::is_allowed_to_offer_now() const
{
        const auto& player = *map::g_player;

        if (player.m_properties.has(PropId::poisoned) && (player.m_hp <= 6))
        {
                return true;
        }

        const auto* const prop = player.m_properties.prop(PropId::wound);

        if (prop)
        {
                const auto* const wound = static_cast<const PropWound*>(prop);

                if (wound->nr_wounds() >= 3)
                {
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
                PropId::wound
        };

        for (PropId prop_id : props_can_heal)
        {
                map::g_player->m_properties.end_prop(prop_id);
        }

        map::g_player->restore_hp(
                999,    // HP restored
                false); // Not allowed above max
}

// -----------------------------------------------------------------------------
// Blessed
// -----------------------------------------------------------------------------
Blessed::Blessed(BenefitId id) :
        Benefit(id)
{

}

std::string Blessed::offer_msg() const
{
        std::string blessed_descr =
                property_data::g_data[(size_t)PropId::blessed]
                .descr;

        blessed_descr = text_format::first_to_lower(blessed_descr);

        return
                "I can give you LUCK (permanently blessed, " +
                blessed_descr +
                ", lasts until reverted by a curse).";
}

bool Blessed::is_allowed_to_offer_now() const
{
        return !map::g_player->m_properties.has(PropId::blessed);
}

void Blessed::run_effect()
{
        auto* const blessed = property_factory::make(PropId::blessed);

        blessed->set_indefinite();

        map::g_player->m_properties.apply(blessed);
}

// -----------------------------------------------------------------------------
// Hasted
// -----------------------------------------------------------------------------
Hasted::Hasted(BenefitId id) :
        Benefit(id)
{

}

std::string Hasted::offer_msg() const
{
        return
                "I can give you TIME (permanently hasted, all actions are "
                "performed at double speed, lasts until reverted by slowing).";
}

bool Hasted::is_allowed_to_offer_now() const
{
        return !map::g_player->m_properties.has(PropId::hasted);
}

void Hasted::run_effect()
{
        auto* const hasted = property_factory::make(PropId::hasted);

        hasted->set_indefinite();

        map::g_player->m_properties.apply(hasted);
}

// -----------------------------------------------------------------------------
// HP reduced
// -----------------------------------------------------------------------------
HpReduced::HpReduced(TollId id) :
        Toll(id)
{
}

std::vector<BenefitId> HpReduced::benefits_not_allowed_with() const
{
        return {BenefitId::gain_hp};
}

std::string HpReduced::offer_msg() const
{
        return "I shall take your HEALTH (-2 maximum hit points).";
}

void HpReduced::run_effect()
{
        map::g_player->change_max_hp(-2);
}

// -----------------------------------------------------------------------------
// SP reduced
// -----------------------------------------------------------------------------
SpReduced::SpReduced(TollId id) :
        Toll(id)
{

}

std::vector<BenefitId> SpReduced::benefits_not_allowed_with() const
{
        return {BenefitId::gain_sp};
}

std::string SpReduced::offer_msg() const
{
        return "I shall take your SPIRIT (-2 maximum spirit points);";
}

void SpReduced::run_effect()
{
        map::g_player->change_max_sp(-2);
}

// -----------------------------------------------------------------------------
// XP reduced
// -----------------------------------------------------------------------------
XpReduced::XpReduced(TollId id) :
        Toll(id)
{

}

bool XpReduced::is_allowed_to_apply_now() const
{
        return game::xp_pct() >= 50;
}

std::vector<BenefitId> XpReduced::benefits_not_allowed_with() const
{
        return {BenefitId::gain_xp};
}

std::string XpReduced::offer_msg() const
{
        return "I shall take your EXPERIENCE (-50% experience).";
}

void XpReduced::run_effect()
{
        game::decr_player_xp(50);
}

// -----------------------------------------------------------------------------
// Unlearn spell
// -----------------------------------------------------------------------------
// UnlearnSpell::UnlearnSpell(TollId id) :
//         Toll(id)
// {

// }

// std::vector<BenefitId> UnlearnSpell::benefits_not_allowed_with() const
// {
//         return {BenefitId::upgrade_spell};
// }

// bool UnlearnSpell::is_allowed_to_offer_now() const
// {
//         // TODO: Only allow it if at least one spell is learned
//         return true;
// }

// std::string UnlearnSpell::offer_msg() const
// {
//         return
//                 "I shall take your MAGIC (unlearn one spell, can be relearned "
//                 "at the same spell level).";
// }

// void UnlearnSpell::run_effect()
// {

// }

// -----------------------------------------------------------------------------
// Slowed
// -----------------------------------------------------------------------------
Slowed::Slowed(TollId id) :
        Toll(id)
{

}

bool Slowed::is_allowed_to_offer_now() const
{
        const auto prop = map::g_player->m_properties.prop(PropId::slowed);

        return !prop || (prop->duration_mode() != PropDurationMode::indefinite);
}

std::vector<BenefitId> Slowed::benefits_not_allowed_with() const
{
        return {BenefitId::hasted};
}

std::string Slowed::offer_msg() const
{
        return
                "I shall take your TIME (permanently slowed, all actions are "
                "performed at half speed, lasts until reverted by hasting).";
}

void Slowed::run_effect()
{
        auto* const slowed = property_factory::make(PropId::slowed);

        slowed->set_indefinite();

        map::g_player->m_properties.apply(slowed);

        if (map::g_player->m_properties.has(PropId::r_slow))
        {
                msg_log::add("Whispering voice: \"How remarkable!\"");
        }
}

// -----------------------------------------------------------------------------
// Blind
// -----------------------------------------------------------------------------
Blind::Blind(TollId id) :
        Toll(id)
{

}

bool Blind::is_allowed_to_offer_now() const
{
        const auto prop = map::g_player->m_properties.prop(PropId::blind);

        return !prop || (prop->duration_mode() != PropDurationMode::indefinite);
}

std::vector<BenefitId> Blind::benefits_not_allowed_with() const
{
        return {};
}

std::string Blind::offer_msg() const
{
        return "I shall take your SIGHT (permanently blinded, until healed).";
}

void Blind::run_effect()
{
        auto* const blind = property_factory::make(PropId::blind);

        blind->set_indefinite();

        map::g_player->m_properties.apply(blind);
}

// -----------------------------------------------------------------------------
// Deaf
// -----------------------------------------------------------------------------
Deaf::Deaf(TollId id) :
        Toll(id)
{

}

bool Deaf::is_allowed_to_offer_now() const
{
        const auto prop = map::g_player->m_properties.prop(PropId::deaf);

        return !prop || (prop->duration_mode() != PropDurationMode::indefinite);
}

std::vector<BenefitId> Deaf::benefits_not_allowed_with() const
{
        return {};
}

std::string Deaf::offer_msg() const
{
        return "I shall take your HEARING (permanently deaf, until healed).";
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
Cursed::Cursed(TollId id) :
        Toll(id)
{

}

bool Cursed::is_allowed_to_offer_now() const
{
        const auto prop = map::g_player->m_properties.prop(PropId::cursed);

        return !prop || (prop->duration_mode() != PropDurationMode::indefinite);
}

std::vector<BenefitId> Cursed::benefits_not_allowed_with() const
{
        return {BenefitId::blessed};
}

std::string Cursed::offer_msg() const
{
        std::string cursed_descr =
                property_data::g_data[(size_t)PropId::cursed]
                .descr;

        cursed_descr = text_format::first_to_lower(cursed_descr);

        return
                "I shall take your LUCK (permanently cursed, " +
                cursed_descr +
                ", lasts until reverted by a blessing).";
}

void Cursed::run_effect()
{
        auto* const cursed = property_factory::make(PropId::cursed);

        cursed->set_indefinite();

        map::g_player->m_properties.apply(cursed);
}

} // pact
