// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MAP_PARSING_HPP
#define MAP_PARSING_HPP

#include <vector>

#include "config.hpp"
#include "feature_data.hpp"
#include "pos.hpp"

struct R;
struct Cell;
class Mob;
class Actor;

template<typename T>
class Array2;

// NOTE: If append mode is used, the caller is responsible for initializing the
// array (e.g. with a previous "overwrite" parse call)
enum class MapParseMode
{
        overwrite,
        append
};

enum class ParseCells {no ,yes};

enum class ParseMobs {no ,yes};

enum class ParseActors {no ,yes};

namespace map_parsers
{

// -----------------------------------------------------------------------------
// Map parsers (usage: create an object and call "run" or "cell")
// -----------------------------------------------------------------------------
class MapParser
{
public:
        virtual ~MapParser() {}

        void run(Array2<bool>& out,
                 const R& area_to_parse_cells,
                 const MapParseMode write_rule = MapParseMode::overwrite);

        bool cell(const P& pos) const;

        virtual bool parse(const Cell& c, const P& pos) const
        {
                (void)c;
                (void)pos;

                return false;
        }

        virtual bool parse(const Mob& f) const
        {
                (void)f;

                return false;
        }

        virtual bool parse(const Actor& a) const
        {
                (void)a;

                return false;
        }

protected:
        MapParser(ParseCells parse_cells,
                  ParseMobs parse_mobs,
                  ParseActors parse_actors) :
                m_parse_cells(parse_cells),
                m_parse_mobs(parse_mobs),
                m_parse_actors(parse_actors) {}

private:
        const ParseCells m_parse_cells;
        const ParseMobs m_parse_mobs;
        const ParseActors m_parse_actors;
};

class BlocksLos : public MapParser
{
public:
        BlocksLos() :
                MapParser(ParseCells::yes, ParseMobs::yes, ParseActors::no) {}

private:
        bool parse(const Cell& c, const P& pos) const override;
        bool parse(const Mob& f) const override;
};

class BlocksWalking : public MapParser
{
public:
        BlocksWalking(ParseActors parse_actors) :
                MapParser(ParseCells::yes, ParseMobs::yes, parse_actors) {}

private:
        bool parse(const Cell& c, const P& pos) const override;
        bool parse(const Mob& f) const override;
        bool parse(const Actor& a) const override;
};

class BlocksActor : public MapParser
{
public:
        BlocksActor(const Actor& actor, ParseActors parse_actors) :
                MapParser(ParseCells::yes, ParseMobs::yes, parse_actors),
                m_actor(actor) {}

private:
        bool parse(const Cell& c, const P& pos) const override;
        bool parse(const Mob& f) const override;
        bool parse(const Actor& a) const override;

        const Actor& m_actor;
};

class BlocksProjectiles : public MapParser
{
public:
        BlocksProjectiles() :
                MapParser(ParseCells::yes, ParseMobs::yes, ParseActors::no) {}

private:
        bool parse(const Cell& c, const P& pos) const override;
        bool parse(const Mob& f) const override;
};

class BlocksSound : public MapParser
{
public:
        BlocksSound() :
                MapParser(ParseCells::yes, ParseMobs::yes, ParseActors::no) {}

private:
        bool parse(const Cell& c, const P& pos) const override;
        bool parse(const Mob& f) const override;
};

class LivingActorsAdjToPos : public MapParser
{
public:
        LivingActorsAdjToPos(const P& pos) :
                MapParser(ParseCells::no, ParseMobs::no, ParseActors::yes),
                m_pos(pos) {}

private:
        bool parse(const Actor& a) const override;

        const P& m_pos;
};

class BlocksItems : public MapParser
{
public:
        BlocksItems() :
                MapParser(ParseCells::yes, ParseMobs::yes, ParseActors::no) {}

private:
        bool parse(const Cell& c, const P& pos) const override;
        bool parse(const Mob& f) const override;
};

class BlocksRigid : public MapParser
{
public:
        BlocksRigid() :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no) {}

private:
        bool parse(const Cell& c, const P& pos) const override;
};

class IsNotFeature : public MapParser
{
public:
        IsNotFeature(const FeatureId id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_feature(id) {}

private:
        bool parse(const Cell& c, const P& pos) const override;

        const FeatureId m_feature;
};

class IsAnyOfFeatures : public MapParser
{
public:
        IsAnyOfFeatures(const std::vector<FeatureId>& features) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_features(features) {}

        IsAnyOfFeatures(const FeatureId id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_features(std::vector<FeatureId> {id}) {}

private:
        bool parse(const Cell& c, const P& pos) const override;

        std::vector<FeatureId> m_features;
};

class AnyAdjIsAnyOfFeatures : public MapParser
{
public:
        AnyAdjIsAnyOfFeatures(const std::vector<FeatureId>& features) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_features(features) {}

        AnyAdjIsAnyOfFeatures(const FeatureId id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_features(std::vector<FeatureId> {id}) {}

private:
        bool parse(const Cell& c, const P& pos) const override;

        std::vector<FeatureId> m_features;
};

class AllAdjIsFeature : public MapParser
{
public:
        AllAdjIsFeature(const FeatureId id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_feature(id) {}

private:
        bool parse(const Cell& c, const P& pos) const override;

        const FeatureId m_feature;
};

class AllAdjIsAnyOfFeatures : public MapParser
{
public:
        AllAdjIsAnyOfFeatures(const std::vector<FeatureId>& features) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_features(features) {}

        AllAdjIsAnyOfFeatures(const FeatureId id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_features(std::vector<FeatureId> {id}) {}

private:
        bool parse(const Cell& c, const P& pos) const override;

        std::vector<FeatureId> m_features;
};

class AllAdjIsNotFeature : public MapParser
{
public:
        AllAdjIsNotFeature(const FeatureId id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_feature(id) {}

private:
        bool parse(const Cell& c, const P& pos) const override;

        const FeatureId m_feature;
};

class AllAdjIsNoneOfFeatures : public MapParser
{
public:
        AllAdjIsNoneOfFeatures(const std::vector<FeatureId>& features) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_features(features) {}

        AllAdjIsNoneOfFeatures(const FeatureId id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_features(std::vector<FeatureId> {id}) {}

private:
        bool parse(const Cell& c, const P& pos) const override;

        std::vector<FeatureId> m_features;
};


// -----------------------------------------------------------------------------
// Various utility algorithms
// -----------------------------------------------------------------------------
// Given a map array of booleans, this will fill a second map array of boolens
// where the cells are set to true if they are within the specified distance
// interval of the first array.
// This can be used for example to find all cells up to N steps from a wall.
Array2<bool> cells_within_dist_of_others(
        const Array2<bool>& in,
        const Range& dist_interval);

void append(Array2<bool>& base,
            const Array2<bool>& append);

// Optimized for expanding with a distance of one
Array2<bool> expand(
        const Array2<bool>& in,
        const R& area_allowed_to_modify);

// Slower version that can expand any distance
Array2<bool> expand(
        const Array2<bool>& in,
        const int dist);

bool is_map_connected(const Array2<bool>& blocked);

} // map_parsers


// Function object for sorting STL containers by distance to a position
struct IsCloserToPos
{
public:
        IsCloserToPos(const P& p) :
                m_pos(p) {}

        bool operator()(const P& p1, const P& p2);

        P m_pos;
};

// Function object for sorting STL containers by distance to a position
struct IsFurtherFromPos
{
public:
        IsFurtherFromPos(const P& p) :
                m_pos(p) {}

        bool operator()(const P& p1, const P& p2);

        P m_pos;
};

#endif // MAP_PARSING_HPP
