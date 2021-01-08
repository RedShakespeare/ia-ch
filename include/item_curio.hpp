// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ITEM_CURIO_HPP
#define ITEM_CURIO_HPP

#include "item.hpp"

namespace item
{
class ZombieDust : public Wpn
{
public:
        ZombieDust(ItemData* const item_data) :
                Wpn(item_data) {}

        void on_ranged_hit(actor::Actor& actor_hit) override;
};

class WitchesEye : public Item
{
public:
        WitchesEye(ItemData* const item_data) :
                Item(item_data) {}

        ConsumeItem activate(actor::Actor* actor);

        // TODO: Consider interface color for "Other Curiosities". Should they
        // all share the same color?
        // Color interface_color() const final
        // {
        //         return colors::light_red();
        // }
};

// class FlaskOfDamning : public Item
// {
// public:
//         FlaskOfDamning(ItemData* const item_data) :
//                 Item(item_data) {}

//         ConsumeItem activate(actor::Actor* actor);
// };

// class ObsidianCharm : public Item
// {
// public:
//         ObsidianCharm(ItemData* const item_data) :
//                 Item(item_data) {}

//         ConsumeItem activate(actor::Actor* actor);
// };

class FluctuatingMaterial : public Item
{
public:
        FluctuatingMaterial(ItemData* const item_data) :
                Item(item_data) {}

        ConsumeItem activate(actor::Actor* actor);
};

// class BatWingSalve : public Item
// {
// public:
//         BatWingSalve(ItemData* const item_data) :
//                 Item(item_data) {}

//         ConsumeItem activate(actor::Actor* actor);
// };

class AstralOpium : public Item
{
public:
        AstralOpium(ItemData* const item_data) :
                Item(item_data) {}

        ConsumeItem activate(actor::Actor* actor);
};

}  // namespace item

#endif  // ITEM_CURIO_HPP
