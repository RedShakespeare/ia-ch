// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <string>
#include <vector>

#include "actor_data.hpp"
#include "spells.hpp"


namespace item
{
enum class Id;
}

enum class SpellId;


namespace pact
{

enum class BenefitId
{
        upgrade_spell,
        gain_hp,
        gain_sp,
        gain_xp,
        remove_insanity,
        gain_item,
        healed,
        blessed,

        // TODO: Consider these:
        // recharge_item (add charges back to artifact, repair device, ...)
        // minor_treasure (3 random minor treasure)
        // remove_item_curse

        END,
        undefined,
};

enum class TollId
{
        hp_reduced,
        sp_reduced,
        xp_reduced,
        deaf,
        cursed,
        unlearn_spell,
        burning,
        spawn_monsters,

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
        virtual ~Benefit() {}

        virtual BenefitId id() const = 0;

        virtual bool is_allowed_to_offer_now() const = 0;

        virtual std::string offer_msg() const = 0;

        virtual void run_effect() = 0;
};

class Toll
{
public:
        Toll();

        virtual ~Toll() {}

        void on_player_reached_new_dlvl();

        TollDone on_player_turn();

        virtual bool is_allowed_to_offer_now() const
        {
                return true;
        }

        virtual bool is_allowed_to_apply_now()
        {
                return true;
        }

        virtual TollId id() const = 0;

        virtual std::vector<BenefitId> benefits_not_allowed_with() const = 0;

        virtual std::string offer_msg() const = 0;

        virtual void run_effect() = 0;

private:
        int m_dlvl_countdown;
        int m_turn_countdown;
};

class UpgradeSpell : public Benefit
{
public:
        UpgradeSpell();

        BenefitId id() const override
        {
                return BenefitId::upgrade_spell;
        }

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
        BenefitId id() const override
        {
                return BenefitId::gain_hp;
        }

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class GainSp : public Benefit
{
public:
        BenefitId id() const override
        {
                return BenefitId::gain_sp;
        }

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class GainXp : public Benefit
{
public:
        BenefitId id() const override
        {
                return BenefitId::gain_xp;
        }

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class RemoveInsanity : public Benefit
{
public:
        BenefitId id() const override
        {
                return BenefitId::remove_insanity;
        }

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class GainItem : public Benefit
{
public:
        GainItem();

        BenefitId id() const override
        {
                return BenefitId::gain_item;
        }

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;

private:
        std::vector<item::Id> find_allowed_item_ids() const;

        item::Id m_item_id;
};

// class RechargeItem : public Benefit
// {
// public:
//         BenefitId id() const override
//         {
//                 return BenefitId::asdf;
//         }

//         std::string offer_msg() const override;

//         bool is_allowed_to_offer_now() const override;

//         void run_effect() override;
// };

class Healed : public Benefit
{
public:
        BenefitId id() const override
        {
                return BenefitId::healed;
        }

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class Blessed : public Benefit
{
public:
        BenefitId id() const override
        {
                return BenefitId::blessed;
        }

        std::string offer_msg() const override;

        bool is_allowed_to_offer_now() const override;

        void run_effect() override;
};

class HpReduced : public Toll
{
public:
        TollId id() const override
        {
                return TollId::hp_reduced;
        }

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

class SpReduced : public Toll
{
public:
        TollId id() const override
        {
                return TollId::sp_reduced;
        }

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

class XpReduced : public Toll
{
public:
        TollId id() const override
        {
                return TollId::xp_reduced;
        }

        bool is_allowed_to_apply_now() override;

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

class Deaf : public Toll
{
public:
        TollId id() const override
        {
                return TollId::deaf;
        }

        bool is_allowed_to_offer_now() const override;

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

class Cursed : public Toll
{
public:
        TollId id() const override
        {
                return TollId::cursed;
        }

        bool is_allowed_to_offer_now() const override;

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

class SpawnMonsters : public Toll
{
public:
        bool is_allowed_to_apply_now() override;

        TollId id() const override
        {
                return TollId::spawn_monsters;
        }

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;

private:
        actor::Id m_id_to_spawn {actor::Id::END};
};

class UnlearnSpell : public Toll
{
public:
        bool is_allowed_to_offer_now() const override;

        bool is_allowed_to_apply_now() override;

        TollId id() const override
        {
                return TollId::unlearn_spell;
        }

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;

private:
        std::vector<SpellId> make_spell_bucket() const;

        SpellId m_spell_to_unlearn {SpellId::END};
};

class Burning : public Toll
{
public:
        bool is_allowed_to_apply_now() override;

        TollId id() const override
        {
                return TollId::burning;
        }

        std::vector<BenefitId> benefits_not_allowed_with() const override;

        std::string offer_msg() const override;

        void run_effect() override;
};

} // pact
