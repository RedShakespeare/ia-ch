#ifndef ROOM_HPP
#define ROOM_HPP

#include <vector>
#include <unordered_map>

#include "rl_utils.hpp"
#include "global.hpp"
#include "feature_data.hpp"

// Room theming occurs both pre- and post-connect (before/after corridors).
//
// > In pre-connect, reshaping is performed, e.g. plus-shape, cavern-shape,
//   pillars, etc)
//
//   When pre-connect starts, it is assumed that all (standard) rooms are
//   rectangular with unbroken walls.
//
// > In post-connect, auto-features such as chests and altars are placed, as
//   well as room-specific stuff like trees, altars, etc. It can then be
//   verified for each feature that the map is still connected.
//
// As a rule of thumb, place walkable features in the pre-connect step, and
// blocking features in the post-connect step.
//

// NOTE: There are both 'RoomType' ids, and 'Room' classes. A room of a certain
// RoomType id does NOT have to be an instance of the corresponding Room child
// class. For example, templated rooms are always created as the TemplateRoom
// class, but they may have any standard room RoomType id. There may even be
// RoomType ids which doesn't have a corresponding Room class at all.

struct FeatureDataT;
class Room;

enum class RoomType
{
        // Standard rooms (standardized feature spawning and reshaping)
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

const std::unordered_map<std::string, RoomType> str_to_room_type_map = {
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

struct RoomAutoFeatureRule
{
        RoomAutoFeatureRule() :
                feature_id(FeatureId::END),
                nr_allowed(0) {}

        RoomAutoFeatureRule(const FeatureId id, const int nr_allowed) :
                feature_id(id),
                nr_allowed(nr_allowed) {}

        FeatureId feature_id;
        int nr_allowed;
};

namespace room_factory
{

void init_room_bucket();

// NOTE: These functions do not make rooms on the map, just create Room objects.
// Use the "make_room..." functions in the map generator for a convenient way to
// generate rooms on the map.
Room* make(const RoomType type, const R& r);

Room* make_random_room(const R& r, const IsSubRoom is_subroom);

} // room_factory

class Room
{
public:
        Room(R r, RoomType type);

        Room() = delete;

        virtual ~Room() {}

        std::vector<P> positions_in_room() const;

        virtual void on_pre_connect(Array2<bool>& door_proposals) = 0;
        virtual void on_post_connect(Array2<bool>& door_proposals) = 0;

        virtual int max_nr_mon_groups_spawned() const
        {
                return 3;
        }

        virtual bool allow_sub_rooms() const
        {
                return true;
        }

        R r_;
        const RoomType type_;
        bool is_sub_room_;
        std::vector<Room*> rooms_con_to_;
        std::vector<Room*> sub_rooms_;

protected:
        void make_dark() const;
};

class StdRoom : public Room
{
public:
        StdRoom(R r, RoomType type) : Room(r, type) {}

        virtual ~StdRoom() {}

        void on_pre_connect(Array2<bool>& door_proposals) override final;
        void on_post_connect(Array2<bool>& door_proposals) override final;

        virtual bool is_allowed() const
        {
                return true;
        }

protected:
        virtual std::vector<RoomAutoFeatureRule> auto_features_allowed() const
        {
                return {};
        }

        P find_auto_feature_placement(const std::vector<P>& adj_to_walls,
                                      const std::vector<P>& away_from_walls,
                                      const FeatureId id) const;

        void place_auto_features();

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

        ~PlainRoom() {}

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class HumanRoom: public StdRoom
{
public:
        HumanRoom(R r) :
                StdRoom(r, RoomType::human) {}

        ~HumanRoom() {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class JailRoom: public StdRoom
{
public:
        JailRoom(R r) :
                StdRoom(r, RoomType::jail) {}

        ~JailRoom() {}

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class RitualRoom: public StdRoom
{
public:
        RitualRoom(R r) :
                StdRoom(r, RoomType::ritual) {}

        ~RitualRoom() {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class SpiderRoom: public StdRoom
{
public:
        SpiderRoom(R r) :
                StdRoom(r, RoomType::spider) {}

        ~SpiderRoom() {}

        bool is_allowed() const override;

        int max_nr_mon_groups_spawned() const override
        {
                return 1;
        }

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class SnakePitRoom: public StdRoom
{
public:
        SnakePitRoom(R r) :
                StdRoom(r, RoomType::monster) {}

        ~SnakePitRoom() {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class CryptRoom: public StdRoom
{
public:
        CryptRoom(R r) :
                StdRoom(r, RoomType::crypt) {}

        ~CryptRoom() {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class MonsterRoom: public StdRoom
{
public:
        MonsterRoom(R r) :
                StdRoom(r, RoomType::monster) {}

        ~MonsterRoom() {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class DampRoom: public StdRoom
{
public:
        DampRoom(R r) :
                StdRoom(r, RoomType::damp) {}

        ~DampRoom() {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class PoolRoom: public StdRoom
{
public:
        PoolRoom(R r) :
                StdRoom(r, RoomType::pool) {}

        ~PoolRoom() {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class CaveRoom: public StdRoom
{
public:
        CaveRoom(R r) :
                StdRoom(r, RoomType::cave) {}

        ~CaveRoom() {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class ChasmRoom: public StdRoom
{
public:
        ChasmRoom(R r) :
                StdRoom(r, RoomType::chasm) {}

        ~ChasmRoom() {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class ForestRoom: public StdRoom
{
public:
        ForestRoom(R r) :
                StdRoom(r, RoomType::forest) {}

        ~ForestRoom() {}

        bool is_allowed() const override;

protected:
        std::vector<RoomAutoFeatureRule> auto_features_allowed() const override;

        void on_pre_connect_hook(Array2<bool>& door_proposals) override;

        void on_post_connect_hook(Array2<bool>& door_proposals) override;
};

class TemplateRoom: public StdRoom
{
public:
        TemplateRoom(const R& r, RoomType type) :
                StdRoom(r, type) {}

        ~TemplateRoom() {}

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

        ~CorrLinkRoom() {}

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

        ~CrumbleRoom() {}

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
                axis_(Axis::hor) {}

        ~RiverRoom() {}

        void on_pre_connect(Array2<bool>& door_proposals) override;

        void on_post_connect(Array2<bool>& door_proposals) override
        {
                (void)door_proposals;
        }

        Axis axis_;
};

#endif // ROOM_HPP
