// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef TERRAIN_GONG_HPP
#define TERRAIN_GONG_HPP

#include <string>
#include <vector>

#include "actor_data.hpp"
#include "spells.hpp"
#include "terrain.hpp"


namespace item
{
enum class Id;
}

enum class SpellId;


namespace terrain
{

class Gong: public Terrain
{
public:
        Gong(const P& p);

        Gong() = delete;

        Id id() const override
        {
                return Id::gong;
        }

        std::string name(const Article article) const override;

        void bump(actor::Actor& actor_bumping) override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        bool m_is_used {false};
};

namespace gong
{

enum class BonusId
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
        spawn_monsters,

        // TODO: Consider these:
        // diseased
        // wounded
        // mind_sapped

        END
};

class Bonus
{
public:
        virtual ~Bonus() {}

        virtual BonusId id() const = 0;

        virtual bool is_allowed() const = 0;

        virtual void run_effect() = 0;
};

class Toll
{
public:
        virtual ~Toll() {}

        virtual TollId id() const = 0;

        virtual bool is_allowed() const
        {
                return true;
        }

        virtual std::vector<BonusId> bonuses_not_allowed_with() const = 0;

        virtual void run_effect() = 0;
};

class UpgradeSpell : public Bonus
{
public:
        UpgradeSpell();

        BonusId id() const override
        {
                return BonusId::upgrade_spell;
        }

        bool is_allowed() const override;

        void run_effect() override;

private:
        std::vector<SpellId> find_spells_can_upgrade() const;

        SpellId m_spell_id;
};

class GainHp : public Bonus
{
public:
        BonusId id() const override
        {
                return BonusId::gain_hp;
        }

        bool is_allowed() const override;

        void run_effect() override;
};

class GainSp : public Bonus
{
public:
        BonusId id() const override
        {
                return BonusId::gain_sp;
        }

        bool is_allowed() const override;

        void run_effect() override;
};

class GainXp : public Bonus
{
public:
        BonusId id() const override
        {
                return BonusId::gain_xp;
        }

        bool is_allowed() const override;

        void run_effect() override;
};

class RemoveInsanity : public Bonus
{
public:
        BonusId id() const override
        {
                return BonusId::remove_insanity;
        }

        bool is_allowed() const override;

        void run_effect() override;
};

class GainItem : public Bonus
{
public:
        GainItem();

        BonusId id() const override
        {
                return BonusId::gain_item;
        }

        bool is_allowed() const override;

        void run_effect() override;

private:
        std::vector<item::Id> find_allowed_item_ids() const;

        item::Id m_item_id;
};

// class RechargeItem : public Bonus
// {
// public:
//         BonusId id() const override
//         {
//                 return BonusId::asdf;
//         }

//         bool is_allowed() const override;

//         void run_effect() override;
// };

class Healed : public Bonus
{
public:
        BonusId id() const override
        {
                return BonusId::healed;
        }

        bool is_allowed() const override;

        void run_effect() override;
};

class Blessed : public Bonus
{
public:
        BonusId id() const override
        {
                return BonusId::blessed;
        }

        bool is_allowed() const override;

        void run_effect() override;
};

class HpReduced : public Toll
{
public:
        TollId id() const override
        {
                return TollId::hp_reduced;
        }

        std::vector<BonusId> bonuses_not_allowed_with() const override;

        void run_effect() override;
};

class SpReduced : public Toll
{
public:
        TollId id() const override
        {
                return TollId::sp_reduced;
        }

        std::vector<BonusId> bonuses_not_allowed_with() const override;

        void run_effect() override;
};

class XpReduced : public Toll
{
public:
        TollId id() const override
        {
                return TollId::xp_reduced;
        }

        bool is_allowed() const override;

        std::vector<BonusId> bonuses_not_allowed_with() const override;

        void run_effect() override;
};

class Deaf : public Toll
{
public:
        TollId id() const override
        {
                return TollId::deaf;
        }

        bool is_allowed() const override;

        std::vector<BonusId> bonuses_not_allowed_with() const override;

        void run_effect() override;
};

class Cursed : public Toll
{
public:
        TollId id() const override
        {
                return TollId::cursed;
        }

        bool is_allowed() const override;

        std::vector<BonusId> bonuses_not_allowed_with() const override;

        void run_effect() override;
};

class SpawnMonsters : public Toll
{
public:
        SpawnMonsters();

        TollId id() const override
        {
                return TollId::spawn_monsters;
        }

        bool is_allowed() const override;

        std::vector<BonusId> bonuses_not_allowed_with() const override;

        void run_effect() override;

private:
        actor::Id m_id_to_spawn {actor::Id::END};
};

class UnlearnSpell : public Toll
{
public:
        UnlearnSpell();

        bool is_allowed() const override;

        TollId id() const override
        {
                return TollId::unlearn_spell;
        }

        std::vector<BonusId> bonuses_not_allowed_with() const override;

        void run_effect() override;

private:
        std::vector<SpellId> make_spell_bucket() const;

        SpellId m_spell_to_unlearn {SpellId::END};
};

} // gong

} // terrain

#endif // TERRAIN_GONG_HPP
