// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MAP_PARSING_HPP
#define MAP_PARSING_HPP

#include <vector>

#include "config.hpp"
#include "terrain_data.hpp"
#include "pos.hpp"


namespace actor
{
class Actor;
}

namespace
{
class Terrain;
}

struct R;
struct Cell;

template<typename T>
class Array2;


// NOTE: If append mode is used, the caller is responsible for initializing the
// array (e.g. with a previous "overwrite" parse call)
enum class MapParseMode
{
        overwrite,
        append
};

enum class ParseCells
{
        no,
        yes
};

enum class ParseMobs
{
        no,
        yes
};

enum class ParseActors
{
        no,
        yes
};


namespace map_parsers
{

// -----------------------------------------------------------------------------
// Map parsers (usage: create an object and call "run" or "cell")
// -----------------------------------------------------------------------------
class MapParser
{
public:
        virtual ~MapParser() {}

        void run(
                Array2<bool>& out,
                const R& area_to_parse_cells,
                const MapParseMode write_rule = MapParseMode::overwrite);

        bool cell(const P& pos) const;

        virtual bool parse_cell(const Cell& c, const P& pos) const
        {
                (void)c;
                (void)pos;

                return false;
        }

        virtual bool parse_mob(const terrain::Terrain& f) const
        {
                (void)f;

                return false;
        }

        virtual bool parse_actor(const actor::Actor& a) const
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
        bool parse_cell(const Cell& c, const P& pos) const override;
        bool parse_mob(const terrain::Terrain& f) const override;
};

class BlocksWalking : public MapParser
{
public:
        BlocksWalking(ParseActors parse_actors) :
                MapParser(ParseCells::yes, ParseMobs::yes, parse_actors) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;
        bool parse_mob(const terrain::Terrain& f) const override;
        bool parse_actor(const actor::Actor& a) const override;
};

class BlocksActor : public MapParser
{
public:
        BlocksActor(const actor::Actor& actor, ParseActors parse_actors) :
                MapParser(ParseCells::yes, ParseMobs::yes, parse_actors),
                m_actor(actor) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;
        bool parse_mob(const terrain::Terrain& f) const override;
        bool parse_actor(const actor::Actor& a) const override;

        const actor::Actor& m_actor;
};

class BlocksProjectiles : public MapParser
{
public:
        BlocksProjectiles() :
                MapParser(ParseCells::yes, ParseMobs::yes, ParseActors::no) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;
        bool parse_mob(const terrain::Terrain& f) const override;
};

class BlocksSound : public MapParser
{
public:
        BlocksSound() :
                MapParser(ParseCells::yes, ParseMobs::yes, ParseActors::no) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;
        bool parse_mob(const terrain::Terrain& f) const override;
};

class LivingActorsAdjToPos : public MapParser
{
public:
        LivingActorsAdjToPos(const P& pos) :
                MapParser(ParseCells::no, ParseMobs::no, ParseActors::yes),
                m_pos(pos) {}

private:
        bool parse_actor(const actor::Actor& a) const override;

        const P& m_pos;
};

class BlocksItems : public MapParser
{
public:
        BlocksItems() :
                MapParser(ParseCells::yes, ParseMobs::yes, ParseActors::no) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;
        bool parse_mob(const terrain::Terrain& f) const override;
};

class BlocksTerrain : public MapParser
{
public:
        BlocksTerrain() :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;
};

class IsNotTerrain : public MapParser
{
public:
        IsNotTerrain(const terrain::Id id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrain(id) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;

        const terrain::Id m_terrain;
};

class IsAnyOfTerrains : public MapParser
{
public:
        IsAnyOfTerrains(const std::vector<terrain::Id>& terrains) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrains(terrains) {}

        IsAnyOfTerrains(const terrain::Id id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrains(std::vector<terrain::Id> {id}) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;

        std::vector<terrain::Id> m_terrains;
};

class AnyAdjIsAnyOfTerrains : public MapParser
{
public:
        AnyAdjIsAnyOfTerrains(const std::vector<terrain::Id>& terrains) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrains(terrains) {}

        AnyAdjIsAnyOfTerrains(const terrain::Id id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrains(std::vector<terrain::Id> {id}) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;

        std::vector<terrain::Id> m_terrains;
};

class AllAdjIsTerrain : public MapParser
{
public:
        AllAdjIsTerrain(const terrain::Id id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrain(id) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;

        const terrain::Id m_terrain;
};

class AllAdjIsAnyOfTerrains : public MapParser
{
public:
        AllAdjIsAnyOfTerrains(const std::vector<terrain::Id>& terrains) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrains(terrains) {}

        AllAdjIsAnyOfTerrains(const terrain::Id id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrains(std::vector<terrain::Id> {id}) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;

        std::vector<terrain::Id> m_terrains;
};

class AllAdjIsNotTerrain : public MapParser
{
public:
        AllAdjIsNotTerrain(const terrain::Id id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrain(id) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;

        const terrain::Id m_terrain;
};

class AllAdjIsNoneOfTerrains : public MapParser
{
public:
        AllAdjIsNoneOfTerrains(const std::vector<terrain::Id>& terrains) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrains(terrains) {}

        AllAdjIsNoneOfTerrains(const terrain::Id id) :
                MapParser(ParseCells::yes, ParseMobs::no, ParseActors::no),
                m_terrains(std::vector<terrain::Id> {id}) {}

private:
        bool parse_cell(const Cell& c, const P& pos) const override;

        std::vector<terrain::Id> m_terrains;
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

// Function struct for sorting STL containers by distance to a position
struct IsFurtherFromPos
{
public:
        IsFurtherFromPos(const P& p) :
                m_pos(p) {}

        bool operator()(const P& p1, const P& p2);

        P m_pos;
};

#endif // MAP_PARSING_HPP
