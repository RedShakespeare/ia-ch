// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef FEATURE_HPP
#define FEATURE_HPP

#include <vector>
#include <string>

#include "colors.hpp"
#include "feature_data.hpp"
#include "global.hpp"
#include "pos.hpp"

class Actor;

template<typename T>
class Array2;

class Feature
{
public:
        Feature(const P& feature_pos) :
                pos_(feature_pos) {}

        virtual ~Feature() {}

        virtual FeatureId id() const = 0;
        virtual std::string name(const Article article) const = 0;
        virtual Color color() const = 0;
        virtual Color color_bg() const = 0;

        const FeatureData& data() const;

        virtual void hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                Actor* const actor)
        {
                (void)dmg;
                (void)dmg_type;
                (void)dmg_method;
                (void)actor;
        }

        virtual void reveal(const Verbosity verbosity);

        virtual AllowAction pre_bump(Actor& actor_bumping)
        {
                (void)actor_bumping;

                return AllowAction::yes;
        }

        virtual void bump(Actor& actor_bumping);
        virtual void on_leave(Actor& actor_leaving);
        virtual void on_new_turn() {}
        virtual bool is_walkable() const;
        virtual bool can_move(const Actor& actor) const;
        virtual bool is_sound_passable() const;
        virtual bool is_los_passable() const;
        virtual bool is_projectile_passable() const;
        virtual bool is_smoke_passable() const;
        virtual char character() const;
        virtual TileId tile() const;
        virtual bool can_have_corpse() const;
        virtual bool can_have_rigid() const;
        virtual bool can_have_blood() const;
        virtual bool can_have_gore() const;
        virtual bool can_have_item() const;
        virtual Matl matl() const;

        int shock_when_adj() const;

        virtual void add_light(Array2<bool>& light) const;

        P pos() const
        {
                return pos_;
        }

protected:
        P pos_;
};

#endif // FEATURE_HPP
