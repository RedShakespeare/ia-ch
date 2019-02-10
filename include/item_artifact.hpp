// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ITEM_ARTIFACT_HPP
#define ITEM_ARTIFACT_HPP

#include "item.hpp"
#include "sound.hpp"

// -----------------------------------------------------------------------------
// Staff of the Pharaohs
// -----------------------------------------------------------------------------
class PharaohStaff: public Wpn
{
public:
        PharaohStaff(ItemData* const item_data);

        void on_std_turn_in_inv(const InvType inv_type) override;

private:
        void on_mon_see_player_carrying(Mon& mon) const;
};

// -----------------------------------------------------------------------------
// Talisman of Reflection
// -----------------------------------------------------------------------------
class ReflTalisman: public Item
{
public:
        ReflTalisman(ItemData* const item_data);

private:
        void on_pickup_hook() override;

        void on_removed_from_inv_hook() override;
};

// -----------------------------------------------------------------------------
// Talisman of Resurrection
// -----------------------------------------------------------------------------
class ResurrectTalisman: public Item
{
public:
        ResurrectTalisman(ItemData* const item_data);
};

// -----------------------------------------------------------------------------
// Talisman of Teleportation Control
// -----------------------------------------------------------------------------
class TeleCtrlTalisman: public Item
{
public:
        TeleCtrlTalisman(ItemData* const item_data);

private:
        void on_pickup_hook() override;

        void on_removed_from_inv_hook() override;
};

// -----------------------------------------------------------------------------
// Horn of Malice
// -----------------------------------------------------------------------------
class HornOfMaliceHeard: public SndHeardEffect
{
public:
        HornOfMaliceHeard() {}

        ~HornOfMaliceHeard() {}

        void run(Actor& actor) const override;
};

class HornOfMalice: public Item
{
public:
        HornOfMalice(ItemData* const item_data);

        std::string name_inf_str() const override;

        void save() const override;

        void load() override;

        ConsumeItem activate(Actor* const actor) override;

private:
        int m_charges;
};

// -----------------------------------------------------------------------------
// Horn of Banishment
// -----------------------------------------------------------------------------
class HornOfBanishmentHeard: public SndHeardEffect
{
public:
        HornOfBanishmentHeard() {}

        ~HornOfBanishmentHeard() {}

        void run(Actor& actor) const override;
};

class HornOfBanishment: public Item
{
public:
        HornOfBanishment(ItemData* const item_data);

        std::string name_inf_str() const override;

        void save() const override;

        void load() override;

        ConsumeItem activate(Actor* const actor) override;

private:
        int m_charges;
};


// -----------------------------------------------------------------------------
// Arcane Clockwork
// -----------------------------------------------------------------------------
class Clockwork: public Item
{
public:
        Clockwork(ItemData* const item_data);

        ConsumeItem activate(Actor* const actor) override;

        std::string name_inf_str() const override;

        void save() const override;

        void load() override;

private:
        int m_charges;
};

// -----------------------------------------------------------------------------
// Spirit Dagger
// -----------------------------------------------------------------------------
class SpiritDagger: public Wpn
{
public:
        SpiritDagger(ItemData* const item_data);

        void on_melee_hit(Actor& actor_hit, const int dmg) override;
};

// -----------------------------------------------------------------------------
// Orb of Life
// -----------------------------------------------------------------------------
class OrbOfLife: public Item
{
public:
        OrbOfLife(ItemData* const item_data);

private:
        void on_pickup_hook() override;

        void on_removed_from_inv_hook() override;
};

#endif // ITEM_ARTIFACT_HPP
