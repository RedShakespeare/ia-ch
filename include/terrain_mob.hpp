// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef TERRAIN_MOB_HPP
#define TERRAIN_MOB_HPP

#include "terrain.hpp"


namespace terrain
{

class Smoke: public Terrain
{
public:
        Smoke(const P& pos, const int nr_turns) :
                Terrain(pos),
                m_nr_turns_left(nr_turns) {}

        explicit Smoke(const P& pos) :
                Terrain(pos) {}

        ~Smoke() override = default;

        terrain::Id id() const override
        {
                return terrain::Id::smoke;
        }

        std::string name(const Article article)  const override;

        Color color() const override;

        void on_placed() override;

        void on_new_turn() override;

protected:
        int m_nr_turns_left {-1};
};

class ForceField: public Terrain
{
public:
        ForceField(const P& pos, const int nr_turns) :
                Terrain(pos),
                m_nr_turns_left(nr_turns) {}

        explicit ForceField(const P& pos) :
                Terrain(pos) {}

        ~ForceField() override = default;

        terrain::Id id() const override
        {
                return terrain::Id::force_field;
        }

        void on_new_turn() override;

        std::string name(const Article article)  const override;

        Color color() const override;

protected:
        int m_nr_turns_left {-1};
};

class LitDynamite: public Terrain
{
public:
        LitDynamite(const P& pos, const int nr_turns) :
                Terrain(pos),
                m_nr_turns_left(nr_turns) {}

        explicit LitDynamite(const P& pos) :
                Terrain(pos) {}

        ~LitDynamite() override = default;

        terrain::Id id() const override
        {
                return terrain::Id::lit_dynamite;
        }

        std::string name(const Article article) const override;

        Color color() const override;

        // TODO: Lit dynamite should add light on their own cell (just one cell)
        // void add_light(Array2<bool>& light) const;

        void on_new_turn() override;

private:
        int m_nr_turns_left {-1};
};

class LitFlare: public Terrain
{
public:
        LitFlare(const P& pos, const int nr_turns) :
                Terrain(pos),
                m_nr_turns_left(nr_turns) {}

        explicit LitFlare(const P& pos) :
                Terrain(pos) {}

        ~LitFlare() override = default;

        terrain::Id id() const override
        {
                return terrain::Id::lit_flare;
        }

        std::string name(const Article article) const override;

        Color color() const override;

        void on_new_turn() override;

        void add_light(Array2<bool>& light) const override;

private:
        int m_nr_turns_left {-1};
};

} // namespace terrain

#endif // TERRAIN_MOB_HPP
