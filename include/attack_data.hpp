// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ATTACK_DATA_HPP
#define ATTACK_DATA_HPP

#include "ability_values.hpp"
#include "actor_data.hpp"


namespace Item
{
class Item;
class Wpn;
}

namespace actor
{
class Actor;
}

struct P;


struct AttData
{
public:
        virtual ~AttData() {}

        actor::Actor* attacker;
        actor::Actor* defender;
        int skill_mod;
        int wpn_mod;
        int dodging_mod;
        int state_mod;
        int hit_chance_tot;
        ActionResult att_result;
        int dmg;
        bool is_intrinsic_att;

protected:
        AttData(
                actor::Actor* const attacker,
                actor::Actor* const defender,
                const item::Item& att_item);
};

struct MeleeAttData: public AttData
{
public:
        MeleeAttData(
                actor::Actor* const attacker,
                actor::Actor& defender,
                const item::Wpn& wpn);

        ~MeleeAttData() {}

        bool is_backstab;
        bool is_weak_attack;
};

struct RangedAttData: public AttData
{
public:
        RangedAttData(
                actor::Actor* const attacker,
                const P& attacker_origin,
                const P& aim_pos,
                const P& current_pos,
                const item::Wpn& wpn);

        ~RangedAttData() {}

        P aim_pos;
        actor::Size aim_lvl;
        actor::Size defender_size;
        int dist_mod;
};

struct ThrowAttData: public AttData
{
public:
        ThrowAttData(
                actor::Actor* const attacker,
                const P& aim_pos,
                const P& current_pos,
                const item::Item& item);

        actor::Size aim_lvl;
        actor::Size defender_size;
        int dist_mod;
};

#endif // ATTACK_DATA_HPP
