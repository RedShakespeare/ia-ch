// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef FEATURE_PYLON_HPP
#define FEATURE_PYLON_HPP

#include <memory>

#include "feature_rigid.hpp"

enum class PylonId
{
        burning,
        slow,
        terrify,
        invis,
        knockback,
        teleport,
        END,
        any
};

class PylonImpl;

// -----------------------------------------------------------------------------
// Pylon
// -----------------------------------------------------------------------------
class Pylon: public Rigid
{
public:
        Pylon(const P& p, PylonId id);

        Pylon() = delete;

        ~Pylon() {}

        FeatureId id() const override
        {
                return FeatureId::pylon;
        }

        std::string name(const Article article) const override;

        void on_hit(const int dmg,
                    const DmgType dmg_type,
                    const DmgMethod dmg_method,
                    Actor* const actor) override;

        void on_lever_pulled(Lever* const lever) override;

        void add_light_hook(Array2<bool>& light) const override;

        int nr_turns_active() const
        {
                return m_nr_turns_active;
        }

private:
        PylonImpl* make_pylon_impl_from_id(const PylonId id);

        virtual void on_new_turn_hook() override;

        Color color_default() const override;

        std::unique_ptr<PylonImpl> m_pylon_impl;

        bool m_is_activated;

        int m_nr_turns_active;
};

// -----------------------------------------------------------------------------
// Pylon implementation
// -----------------------------------------------------------------------------
class PylonImpl
{
public:
        PylonImpl(P p, Pylon* pylon) :
                m_pos(p),
                m_pylon(pylon) {}

        virtual void on_new_turn_activated() = 0;

protected:
        // void emit_trigger_snd() const;

        std::vector<Actor*> living_actors_reached() const;

        Actor* rnd_reached_living_actor() const;

        P m_pos;

        Pylon* const m_pylon;
};

class PylonBurning: public PylonImpl
{
public:
        PylonBurning(P p, Pylon* pylon) :
                PylonImpl(p, pylon) {}

        void on_new_turn_activated() override;
};

class PylonTerrify: public PylonImpl
{
public:
        PylonTerrify(P p, Pylon* pylon) :
                PylonImpl(p, pylon) {}

        void on_new_turn_activated() override;
};

class PylonInvis: public PylonImpl
{
public:
        PylonInvis(P p, Pylon* pylon) :
                PylonImpl(p, pylon) {}

        void on_new_turn_activated() override;
};

class PylonSlow: public PylonImpl
{
public:
        PylonSlow(P p, Pylon* pylon) :
                PylonImpl(p, pylon) {}

        void on_new_turn_activated() override;
};

class PylonKnockback: public PylonImpl
{
public:
        PylonKnockback(P p, Pylon* pylon) :
                PylonImpl(p, pylon) {}

        void on_new_turn_activated() override;
};

class PylonTeleport: public PylonImpl
{
public:
        PylonTeleport(P p, Pylon* pylon) :
                PylonImpl(p, pylon) {}

        void on_new_turn_activated() override;
};

#endif // FEATURE_PYLON_HPP
