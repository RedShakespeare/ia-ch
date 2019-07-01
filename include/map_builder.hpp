// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MAP_BUILDER_HPP
#define MAP_BUILDER_HPP

#include <memory>
#include <vector>

#include "array2.hpp"
#include "map_templates.hpp"
#include "pos.hpp"

class MapController;
class MapBuilder;

enum class MapType
{
        deep_one_lair,
        magic_pool,
        egypt,
        high_priest,
        intro_forest,
        rat_cave,
        std,
        trapez
};


// -----------------------------------------------------------------------------
// map_builder
// -----------------------------------------------------------------------------
namespace map_builder
{

std::unique_ptr<MapBuilder> make(const MapType map_type);

} // map_builder

// -----------------------------------------------------------------------------
// MapBuilder
// -----------------------------------------------------------------------------
class MapBuilder
{
public:
        virtual ~MapBuilder() {}

        void build();

private:
        virtual bool build_specific() = 0;

        virtual std::unique_ptr<MapController> map_controller() const;
};

// -----------------------------------------------------------------------------
// MapBuilderTemplateLevel
// -----------------------------------------------------------------------------
class MapBuilderTemplateLevel: public MapBuilder
{
public:
        virtual ~MapBuilderTemplateLevel() {}

protected:
        const Array2<char>& get_template() const
        {
                return m_template;
        }

private:
        bool build_specific() override final;

        virtual LevelTemplId template_id() const = 0;

        virtual bool allow_transform_template() const
        {
                return true;
        }

        virtual void handle_template_pos(const P& p, const char c) = 0;

        virtual void on_template_built() {}

        Array2<char> m_template {P(0, 0)};
};

// -----------------------------------------------------------------------------
// MapBuilderStd
// -----------------------------------------------------------------------------
class MapBuilderStd: public MapBuilder
{
public:
        ~MapBuilderStd() {}

private:
        bool build_specific() override;

        std::unique_ptr<MapController> map_controller() const override;
};

// -----------------------------------------------------------------------------
// MapBuilderDeepOneLair
// -----------------------------------------------------------------------------
class MapBuilderDeepOneLair: public MapBuilderTemplateLevel
{
public:
        MapBuilderDeepOneLair();

        ~MapBuilderDeepOneLair() {}

private:
        LevelTemplId template_id() const override
        {
                return LevelTemplId::deep_one_lair;
        }

        bool allow_transform_template() const override
        {
                return true;
        }

        void handle_template_pos(const P& p, const char c) override;

        void on_template_built() override;

        const char m_passage_symbol;
};

// -----------------------------------------------------------------------------
// MapBuilderMagicPool
// -----------------------------------------------------------------------------
class MapBuilderMagicPool: public MapBuilderTemplateLevel
{
public:
        MapBuilderMagicPool();

        ~MapBuilderMagicPool() {}

private:
        LevelTemplId template_id() const override
        {
                return LevelTemplId::magic_pool;
        }

        bool allow_transform_template() const override
        {
                return true;
        }

        void handle_template_pos(const P& p, const char c) override;

        void on_template_built() override;
};

// -----------------------------------------------------------------------------
// MapBuilderIntroForest
// -----------------------------------------------------------------------------
class MapBuilderIntroForest: public MapBuilderTemplateLevel
{
public:
        MapBuilderIntroForest() :
                MapBuilderTemplateLevel() {}

        ~MapBuilderIntroForest() {}

private:
        LevelTemplId template_id() const override
        {
                return LevelTemplId::intro_forest;
        }

        bool allow_transform_template() const override
        {
                return false;
        }

        void handle_template_pos(const P& p, const char c) override;

        void on_template_built() override;

        std::vector<P> m_possible_grave_positions {};
};

// -----------------------------------------------------------------------------
// MapBuilderEgypt
// -----------------------------------------------------------------------------
class MapBuilderEgypt: public MapBuilderTemplateLevel
{
public:
        MapBuilderEgypt();

        ~MapBuilderEgypt() {}

private:
        LevelTemplId template_id() const override
        {
                return LevelTemplId::egypt;
        }

        void handle_template_pos(const P& p, const char c) override;

        void on_template_built() override;

        const char m_stair_symbol;
};

// -----------------------------------------------------------------------------
// MapBuilderRatCave
// -----------------------------------------------------------------------------
class MapBuilderRatCave: public MapBuilderTemplateLevel
{
public:
        MapBuilderRatCave() :
                MapBuilderTemplateLevel() {}

        ~MapBuilderRatCave() {}

private:
        LevelTemplId template_id() const override
        {
                return LevelTemplId::rat_cave;
        }

        void handle_template_pos(const P& p, const char c) override;

        void on_template_built() override;
};

// -----------------------------------------------------------------------------
// MapBuilderBoss
// -----------------------------------------------------------------------------
class MapBuilderBoss: public MapBuilderTemplateLevel
{
public:
        MapBuilderBoss() :
                MapBuilderTemplateLevel() {}

        ~MapBuilderBoss() {}

private:
        LevelTemplId template_id() const override
        {
                return LevelTemplId::high_priest;
        }

        bool allow_transform_template() const override
        {
                return false;
        }

        void handle_template_pos(const P& p, const char c) override;

        void on_template_built() override;

        std::unique_ptr<MapController> map_controller() const override;
};

// -----------------------------------------------------------------------------
// MapBuilderTrapez
// -----------------------------------------------------------------------------
class MapBuilderTrapez: public MapBuilderTemplateLevel
{
public:
        MapBuilderTrapez() :
                MapBuilderTemplateLevel() {}

        virtual ~MapBuilderTrapez() {}

private:
        LevelTemplId template_id() const override
        {
                return LevelTemplId::trapez;
        }

        void handle_template_pos(const P& p, const char c) override;
};

#endif // MAP_BUILDER_HPP
