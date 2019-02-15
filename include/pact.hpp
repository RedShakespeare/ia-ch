// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <string>
#include <vector>

#include "item_data.hpp"

namespace pact
{

enum class BenefitId
{
        undefined,

        START_OF_BENEFITS,

        upgrade_spell,
        gain_hp,
        gain_sp,
        gain_xp,
        remove_insanity,
        gain_item,
        // TODO: Consider this - it could be used to recharge depleted artifacts
        // which has a limited number of uses (or perhaps this is not a good
        // pact, and should be something available elsewhere)
        // recharge_item,
        healed,
        blessed,

        END
};

enum class TollId
{
        hp_reduced,
        sp_reduced,
        xp_reduced,
        // TODO: Consider this:
        // unlearn_spell,
        slowed,
        blind,
        deaf,
        cursed,

        END
};

enum class TollDone
{
        no,
        yes
};


void init();

void cleanup();

void save();

void load();

void offer_pact_to_player();

void on_player_reached_new_dlvl();

void on_player_turn();


class Benefit
{
public:
        Benefit(BenefitId id) :
                m_id(id) {}

        virtual ~Benefit() {}

        BenefitId id() const
        {
                return m_id;
        }

        virtual bool is_allowed_to_offer_now() const = 0;

        virtual std::string offer_msg() const = 0;

        virtual void run_effect() = 0;

private:
        const BenefitId m_id;
};

class Toll
{
public:
        Toll(TollId id);

        virtual ~Toll() {}

        TollId id() const
        {
                return m_id;
        }

        void on_player_reached_new_dlvl();

        TollDone on_player_turn();

        virtual bool is_allowed_to_offer_now() const
        {
                return true;
        }

        virtual bool is_allowed_to_apply_now() const
        {
                return true;
        }

        virtual std::vector<BenefitId> benefits_not_allowed_with() const = 0;

        virtual std::string offer_msg() const = 0;

        virtual void run_effect() = 0;

private:
        const TollId m_id;

        int m_dlvl_countdown;
        int m_turn_countdown;
};

class UpgradeSpell : public Benefit
{
public:
        UpgradeSpell(BenefitId id);

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;

private:
        std::vector<SpellId> find_spells_can_upgrade() const;

        SpellId m_spell_id;
};

class GainHp : public Benefit
{
public:
        GainHp(BenefitId id);

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class GainSp : public Benefit
{
public:
        GainSp(BenefitId id);

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class GainXp : public Benefit
{
public:
        GainXp(BenefitId id);

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class RemoveInsanity : public Benefit
{
public:
        RemoveInsanity(BenefitId id);

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class GainItem : public Benefit
{
public:
        GainItem(BenefitId id);

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;

private:
        std::vector<ItemId> find_allowed_item_ids() const;

        ItemId m_item_id;
};

// class RechargeItem : public Benefit
// {
// public:
//         RechargeItem(BenefitId id);

//         std::string offer_msg() const override;

//         bool is_allowed_to_offer_now() const override;

//         void run_effect() override;
// };

class Healed : public Benefit
{
public:
        Healed(BenefitId id);

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class Blessed : public Benefit
{
public:
        Blessed(BenefitId id);

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class HpReduced : public Toll
{
public:
        HpReduced(TollId id);

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

class SpReduced : public Toll
{
public:
        SpReduced(TollId id);

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

class XpReduced : public Toll
{
public:
        XpReduced(TollId id);

        bool is_allowed_to_apply_now() const override;

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

// class UnlearnSpell : public Toll
// {
// public:
//         UnlearnSpell(TollId id);

//         std::vector<BenefitId> benefits_not_allowed_with() const override;

//         bool is_allowed_to_offer_now() const override;

//         std::string offer_msg() const override;

//         void run_effect() override;
// };

class Slowed : public Toll
{
public:
        Slowed(TollId id);

        bool is_allowed_to_offer_now() const override;

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

class Blind : public Toll
{
public:
        Blind(TollId id);

        bool is_allowed_to_offer_now() const override;

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

class Deaf : public Toll
{
public:
        Deaf(TollId id);

        bool is_allowed_to_offer_now() const override;

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

class Cursed : public Toll
{
public:
        Cursed(TollId id);

        bool is_allowed_to_offer_now() const override;

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

} // pact
