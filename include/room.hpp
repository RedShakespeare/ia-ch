// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ROOM_HPP
#define ROOM_HPP

#include <unordered_map>
#include <vector>

#include "global.hpp"
#include "rect.hpp"
#include "terrain_data.hpp"

// Room theming occurs both pre- and post-connect (before/after corridors).
//
// > In pre-connect, reshaping is performed, e.g. plus-shape, cavern-shape,
//   pillars, etc)
//
//   When pre-connect starts, it is assumed that all (standard) rooms are
//   rectangular with unbroken walls.
//
// > In post-connect, auto-terrains such as chests and altars are placed, as
//   well as room-specific stuff like trees, altars, etc. It can then be
//   verified for each terrain that the map is still connected.
//
// As a rule of thumb, place walkable terrains in the pre-connect step, and
// blocking terrains in the post-connect step.
//

// NOTE: There are both 'RoomType' ids, and 'Room' classes. A room of a certain
// RoomType id does NOT have to be an instance of the corresponding Room child
// class. For example, templated rooms are always created as the TemplateRoom
// class, but they may have any standard room RoomType id. There may even be
// RoomType ids which doesn't have a corresponding Room class at all.


class Room;

template<typename T>
class Array2;


enum class RoomType
{
        // Standard rooms (standardized terrain spawning and reshaping)
        plain, // NOTE: "plain" must be the first type
        human,
        ritual,
        jail,
        spider,
        snake_pit,
        crypt,
        monster,
        damp, // Shallow water/mud scattered over the room
        pool, // Larger body of water - artificial or natural pools or lakes
        cave,
        chasm,
        forest,
        END_OF_STD_ROOMS,

        // Special room types
        corr_link,
        crumble_room,
        river
};

const std::unordered_map<std::string, RoomType> g_str_to_room_type_map = {
        {"plain", RoomType::plain},
        {"human", RoomType::human},
        {"ritual", RoomType::ritual},
        {"jail", RoomType::jail},
        {"spider", RoomType::spider},
        {"snake_pit", RoomType::snake_pit},
        {"crypt", RoomType::crypt},
        {"monster", RoomType::monster},
        {"damp", RoomType::damp},
        {"pool", RoomType::pool},
        {"cave", RoomType::cave},
        {"chasm", RoomType::chasm},
        {"forest", RoomType::forest}
};

struct RoomAutoTerrainRule
{
        RoomAutoTerrainRule() :
                id(terrain::Id::END),
                nr_allowed(0) {}

        RoomAutoTerrainRule(
                const terrain::Id terrain_id,
                const int nr_terrains_allowed) :
                id(terrain_id),
                nr_allowed(nr_terrains_allowed) {}

        terrain::Id id;
        int nr_allowed;
};


namespace room_factory
{

void init_room_bucket();

// NOTE: These functions do not make rooms on the map, just create Room objects.
// Use the "make_room..." functions in the map generator for a convenient way to
// generate rooms on the map.
Room* make(RoomType type, const R& r);

Room* make_random_room(const R& r, IsSubRoom is_subroom);

} // namespace room_factory

class Room
{
public:
        Room(R r, RoomType type);

        Room() = delete;

        virtual ~Room() = default;

        std::vector<P> positions_in_room() const;

        virtual void on_pre_connect(Array2<bool>& door_proposals) = 0;
        virtual void on_post_connect(Array2<bool>& door_proposals) = 0;

        virtual void populate_monsters() const {}

        virtual int max_nr_mon_groups_spawned() const
        {
                return 3;
        }

        virtual bool allow_sub_rooms() const
        {
                return true;
        }

        R m_r;
        const RoomType m_type;
        bool m_is_sub_room;
        std::vector<Room*> m_rooms_con_to;
        std::vector<Room*> m_sub_rooms;

protected:
        void make_dark() const;
};

class StdRoom : public Room
{
public:
        StdRoom(R r, RoomType type) : Room(r, type) {}

        virtual ~StdRoom() = default;

        void on_pre_connect(Array2<bool>& door_proposals) final;
        void on_post_connect(Array2<bool>& door_proposals) final;

        virtual bool is_allowed() const
        {
                return true;
        }

protected:
        virtual std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const
        {
                return {};
        }

        P find_auto_terrain_placement(
                const std::vector<P>& adj_to_walls,
                const std::vector<P>& away_from_walls,
                terrain::Id id) const;

        void place_auto_terrains();

        virtual void on_pre_connect_hook(Array2<bool>& door_proposals)
        {
                (void)door_proposals;
        }

        virtual void on_post_connect_hook(Array2<bool>& door_proposals)
        {
                (void)door_proposals;
        }
};

class PlainRoom: public StdRoom
{
public:
        PlainRoom(R r) : StdRoom(r, RoomType::plain) {}

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class HumanRoom: public StdRoom
{
public:
        HumanRoom(R r) :
                StdRoom(r, RoomType::human) {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class JailRoom: public StdRoom
{
public:
        JailRoom(R r) :
                StdRoom(r, RoomType::jail) {}

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class RitualRoom: public StdRoom
{
public:
        RitualRoom(R r) :
                StdRoom(r, RoomType::ritual) {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class SpiderRoom: public StdRoom
{
public:
        SpiderRoom(R r) :
                StdRoom(r, RoomType::spider) {}

        bool is_allowed() const override;

        int max_nr_mon_groups_spawned() const override
        {
                return 1;
        }

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class SnakePitRoom: public StdRoom
{
public:
        SnakePitRoom(R r) :
                StdRoom(r, RoomType::monster) {}

        bool is_allowed() const override;

        void populate_monsters() const override;

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class CryptRoom: public StdRoom
{
public:
        CryptRoom(R r) :
                StdRoom(r, RoomType::crypt) {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class MonsterRoom: public StdRoom
{
public:
        MonsterRoom(R r) :
                StdRoom(r, RoomType::monster) {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class DampRoom: public StdRoom
{
public:
        DampRoom(R r) :
                StdRoom(r, RoomType::damp) {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class PoolRoom: public StdRoom
{
public:
        PoolRoom(R r) :
                StdRoom(r, RoomType::pool) {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class CaveRoom: public StdRoom
{
public:
        CaveRoom(R r) :
                StdRoom(r, RoomType::cave) {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class ChasmRoom: public StdRoom
{
public:
        ChasmRoom(R r) :
                StdRoom(r, RoomType::chasm) {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class ForestRoom: public StdRoom
{
public:
        ForestRoom(R r) :
                StdRoom(r, RoomType::forest) {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoTerrainRule> auto_terrains_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class TemplateRoom: public StdRoom
{
public:
        TemplateRoom(const R& r, RoomType type) :
                StdRoom(r, type) {}

        bool allow_sub_rooms() const override
        {
                return false;
        }
};

class CorrLinkRoom: public Room
{
public:
        CorrLinkRoom(const R& r) :
                Room(r, RoomType::corr_link) {}

        void on_pre_connect(Array2<bool>& door_proposals) override
        {
                (void)door_proposals;
        }

        void on_post_connect(Array2<bool>& door_proposals) override
        {
                (void)door_proposals;
        }
};

class CrumbleRoom: public Room
{
public:
        CrumbleRoom(const R& r) :
                Room(r, RoomType::crumble_room) {}

        void on_pre_connect(Array2<bool>& door_proposals) override
        {
                (void)door_proposals;
        }

        void on_post_connect(Array2<bool>& door_proposals) override
        {
                (void)door_proposals;
        }
};

class RiverRoom: public Room
{
public:
        RiverRoom(const R& r) :
                Room(r, RoomType::river),
                m_axis(Axis::hor) {}

        void on_pre_connect(Array2<bool>& door_proposals) override;

        void on_post_connect(Array2<bool>& door_proposals) override
        {
                (void)door_proposals;
        }

        Axis m_axis;
};

#endif // ROOM_HPP
