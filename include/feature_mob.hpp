// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef FEATURE_MOB_HPP
#define FEATURE_MOB_HPP

#include "feature.hpp"

class Mob: public Feature
{
public:
        Mob(const P& feature_pos) :
                Feature(feature_pos) {}

        virtual ~Mob() {}

        virtual FeatureId id() const override = 0;

        virtual std::string name(const Article article) const override = 0;

        Color color() const override = 0;

        Color color_bg() const override final
        {
                return colors::black();
        }
};

class Smoke: public Mob
{
public:
        Smoke(const P& feature_pos, const int nr_turns) :
                Mob(feature_pos),
                m_nr_turns_left(nr_turns) {}

        Smoke(const P& feature_pos) :
                Mob(feature_pos),
                m_nr_turns_left(-1) {}

        ~Smoke() {}

        FeatureId id() const override
        {
                return FeatureId::smoke;
        }

        std::string name(const Article article)  const override;

        Color color() const override;

        void on_new_turn() override;

protected:
        int m_nr_turns_left;
};

class ForceField: public Mob
{
public:
        ForceField(const P& feature_pos, const int nr_turns) :
                Mob(feature_pos),
                m_nr_turns_left(nr_turns) {}

        ForceField(const P& feature_pos) :
                Mob(feature_pos),
                m_nr_turns_left(-1) {}

        ~ForceField() {}

        FeatureId id() const override
        {
                return FeatureId::force_field;
        }

        void on_new_turn() override;

        std::string name(const Article article)  const override;

        Color color() const override;

protected:
        int m_nr_turns_left;
};

class LitDynamite: public Mob
{
public:
        LitDynamite(const P& feature_pos, const int nr_turns) :
                Mob(feature_pos),
                m_nr_turns_left(nr_turns) {}

        LitDynamite(const P& feature_pos) :
                Mob(feature_pos),
                m_nr_turns_left(-1) {}

        ~LitDynamite() {}

        FeatureId id() const override
        {
                return FeatureId::lit_dynamite;
        }

        std::string name(const Article article) const override;

        Color color() const override;

        // TODO: Lit dynamite should add light on their own cell (just one cell)
        // void add_light(Array2<bool>& light) const;

        void on_new_turn() override;

private:
        int m_nr_turns_left;
};

class LitFlare: public Mob
{
public:
        LitFlare(const P& feature_pos, const int nr_turns) :
                Mob(feature_pos),
                m_nr_turns_left(nr_turns) {}

        LitFlare(const P& feature_pos) :
                Mob(feature_pos),
                m_nr_turns_left(-1) {}

        ~LitFlare() {}

        FeatureId id() const override
        {
                return FeatureId::lit_flare;
        }

        std::string name(const Article article) const override;

        Color color() const override;

        void on_new_turn() override;

        void add_light(Array2<bool>& light) const override;

private:
        int m_nr_turns_left;
};

#endif // FEATURE_MOB_HPP
