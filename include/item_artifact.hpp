// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ITEM_ARTIFACT_HPP
#define ITEM_ARTIFACT_HPP

#include "item.hpp"
#include "item_curse_ids.hpp"
#include "sound.hpp"

namespace actor {
class Actor;
class Mon;
} // namespace actor

namespace item {

// -----------------------------------------------------------------------------
// Staff of the Pharaohs
// -----------------------------------------------------------------------------
class PharaohStaff : public Wpn {
public:
        PharaohStaff(ItemData* item_data);

        void on_std_turn_in_inv_hook(InvType inv_type) override;

private:
        void on_mon_see_player_carrying(actor::Mon& mon) const;
};

// -----------------------------------------------------------------------------
// Talisman of Reflection
// -----------------------------------------------------------------------------
class ReflTalisman : public Item {
public:
        ReflTalisman(ItemData* item_data);

private:
        void on_pickup_hook() override;

        void on_removed_from_inv_hook() override;
};

// -----------------------------------------------------------------------------
// Talisman of Resurrection
// -----------------------------------------------------------------------------
class ResurrectTalisman : public Item {
public:
        ResurrectTalisman(ItemData* item_data);

        bool is_curse_allowed(item_curse::Id id) const override
        {
                (void)id;

                // This item is destroyed on use, it's too messy to also
                // have to deactivate the curse
                return false;
        }
};

// -----------------------------------------------------------------------------
// Talisman of Teleportation Control
// -----------------------------------------------------------------------------
class TeleCtrlTalisman : public Item {
public:
        TeleCtrlTalisman(ItemData* item_data);

        bool is_curse_allowed(item_curse::Id id) const override
        {
                return id != item_curse::Id::teleport;
        }

private:
        void on_pickup_hook() override;

        void on_removed_from_inv_hook() override;
};

// -----------------------------------------------------------------------------
// Horn of Malice
// -----------------------------------------------------------------------------
class HornOfMaliceHeard : public SndHeardEffect {
public:
        HornOfMaliceHeard() = default;

        ~HornOfMaliceHeard() = default;

        void run(actor::Actor& actor) const override;
};

class HornOfMalice : public Item {
public:
        HornOfMalice(ItemData* item_data);

        std::string name_inf_str() const override;

        void save_hook() const override;

        void load_hook() override;

        ConsumeItem activate(actor::Actor* actor) override;

private:
        int m_charges;
};

// -----------------------------------------------------------------------------
// Horn of Banishment
// -----------------------------------------------------------------------------
class HornOfBanishmentHeard : public SndHeardEffect {
public:
        HornOfBanishmentHeard() = default;

        ~HornOfBanishmentHeard() = default;

        void run(actor::Actor& actor) const override;
};

class HornOfBanishment : public Item {
public:
        HornOfBanishment(ItemData* item_data);

        std::string name_inf_str() const override;

        void save_hook() const override;

        void load_hook() override;

        ConsumeItem activate(actor::Actor* actor) override;

private:
        int m_charges;
};

// -----------------------------------------------------------------------------
// Arcane Clockwork
// -----------------------------------------------------------------------------
class Clockwork : public Item {
public:
        Clockwork(ItemData* item_data);

        ConsumeItem activate(actor::Actor* actor) override;

        std::string name_inf_str() const override;

        void save_hook() const override;

        void load_hook() override;

private:
        int m_charges;
};

// -----------------------------------------------------------------------------
// Spirit Dagger
// -----------------------------------------------------------------------------
class SpiritDagger : public Wpn {
public:
        SpiritDagger(ItemData* item_data);

protected:
        void specific_dmg_mod(
                DmgRange& range,
                const actor::Actor* actor) const override;
};

// -----------------------------------------------------------------------------
// Orb of Life
// -----------------------------------------------------------------------------
class OrbOfLife : public Item {
public:
        OrbOfLife(ItemData* item_data);

private:
        void on_pickup_hook() override;

        void on_removed_from_inv_hook() override;
};

} // namespace item

#endif // ITEM_ARTIFACT_HPP
