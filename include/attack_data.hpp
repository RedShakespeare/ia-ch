// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ATTACK_DATA_HPP
#define ATTACK_DATA_HPP

#include "ability_values.hpp"
#include "actor_data.hpp"
#include "dmg_range.hpp"

namespace actor {
class Actor;
} // namespace actor

namespace item {
class Item;
} // namespace item

struct P;

struct AttData {
public:
        virtual ~AttData() = default;

        int hit_chance_tot {0};
        DmgRange dmg_range {};
        actor::Actor* attacker {nullptr};
        actor::Actor* defender {nullptr};
        const item::Item* att_item {nullptr};
        bool is_intrinsic_att {false};

protected:
        AttData(
                actor::Actor* attacker,
                actor::Actor* defender,
                const item::Item& att_item);
};

struct MeleeAttData : public AttData {
public:
        MeleeAttData(
                actor::Actor* attacker,
                actor::Actor& defender,
                const item::Wpn& wpn);

        ~MeleeAttData() = default;

        bool is_backstab {false};
        bool is_weak_attack {false};
};

struct RangedAttData : public AttData {
public:
        RangedAttData(
                actor::Actor* attacker,
                const P& attacker_origin,
                const P& aim_pos,
                const P& current_pos,
                const item::Wpn& wpn);

        ~RangedAttData() = default;

        P aim_pos {0, 0};
        actor::Size aim_lvl {(actor::Size)0};
        actor::Size defender_size {(actor::Size)0};
};

struct ThrowAttData : public AttData {
public:
        ThrowAttData(
                actor::Actor* attacker,
                const P& attacker_origin,
                const P& aim_pos,
                const P& current_pos,
                const item::Item& item);

        actor::Size aim_lvl {(actor::Size)0};
        actor::Size defender_size {(actor::Size)0};
};

#endif // ATTACK_DATA_HPP
