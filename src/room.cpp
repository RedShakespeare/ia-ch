#include "room.hpp"

#include <algorithm>
#include <climits>
#include <numeric>

#include "actor_factory.hpp"
#include "actor_player.hpp"
#include "feature_rigid.hpp"
#include "game_time.hpp"
#include "gods.hpp"
#include "init.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "mapgen.hpp"
#include "populate_monsters.hpp"

#ifndef NDEBUG
#include "io.hpp"
#include "sdl_base.hpp"
#endif // NDEBUG

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<RoomType> room_bucket_;

static void add_to_room_bucket(const RoomType type, const size_t nr)
{
        if (nr > 0)
        {
                room_bucket_.reserve(room_bucket_.size() + nr);

                for (size_t i = 0; i < nr; ++i)
                {
                        room_bucket_.push_back(type);
                }
        }
}

// NOTE: This cannot be a virtual class method, since a room of a certain
// RoomType doesn't have to be an instance of the corresponding Room child class
// (it could be for example a TemplateRoom object with RoomType "ritual", in
// that case we still want the same chance to make it dark).
static int base_pct_chance_dark(const RoomType room_type)
{
        switch (room_type)
        {
        case RoomType::plain:
                return 5;

        case RoomType::human:
                return 10;

        case RoomType::ritual:
                return 15;

        case RoomType::jail:
                return 60;

        case RoomType::spider:
                return 50;

        case RoomType::snake_pit:
                return 75;

        case RoomType::crypt:
                return 60;

        case RoomType::monster:
                return 80;

        case RoomType::damp:
                return 25;

        case RoomType::pool:
                return 25;

        case RoomType::cave:
                return 30;

        case RoomType::chasm:
                return 25;

        case RoomType::forest:
                return 10;

        case RoomType::END_OF_STD_ROOMS:
        case RoomType::corr_link:
        case RoomType::crumble_room:
        case RoomType::river:
                break;
        }

        return 0;
}

// -----------------------------------------------------------------------------
// Room factory
// -----------------------------------------------------------------------------
namespace room_factory
{

void init_room_bucket()
{
        TRACE_FUNC_BEGIN;

        room_bucket_.clear();

        const int dlvl = map::dlvl;

        if (dlvl <= dlvl_last_early_game)
        {
                add_to_room_bucket(RoomType::human, rnd::range(4, 5));
                add_to_room_bucket(RoomType::jail, 1);
                add_to_room_bucket(RoomType::ritual, 1);
                add_to_room_bucket(RoomType::crypt, rnd::range(2, 3));
                add_to_room_bucket(RoomType::monster, 1);
                add_to_room_bucket(RoomType::damp, rnd::range(1, 2));
                add_to_room_bucket(RoomType::pool, rnd::range(1, 2));
                add_to_room_bucket(RoomType::snake_pit, rnd::one_in(3) ? 1 : 0);

                const size_t nr_plain_rooms = room_bucket_.size() * 2;

                add_to_room_bucket(RoomType::plain, nr_plain_rooms);
        }
        else if (dlvl <= dlvl_last_mid_game)
        {
                add_to_room_bucket(RoomType::human, rnd::range(2, 3));
                add_to_room_bucket(RoomType::jail, rnd::range(1, 2));
                add_to_room_bucket(RoomType::ritual, 1);
                add_to_room_bucket(RoomType::spider, rnd::range(1, 3));
                add_to_room_bucket(RoomType::snake_pit, 1);
                add_to_room_bucket(RoomType::crypt, 4);
                add_to_room_bucket(RoomType::monster, 2);
                add_to_room_bucket(RoomType::damp, rnd::range(1, 3));
                add_to_room_bucket(RoomType::pool, rnd::range(1, 3));
                add_to_room_bucket(RoomType::cave, 2);
                add_to_room_bucket(RoomType::chasm, 1);
                add_to_room_bucket(RoomType::forest, 2);

                const size_t nr_plain_rooms = room_bucket_.size() * 2;

                add_to_room_bucket(RoomType::plain, nr_plain_rooms);
        }
        else // Late game
        {
                add_to_room_bucket(RoomType::monster, 1);
                add_to_room_bucket(RoomType::spider, 1);
                add_to_room_bucket(RoomType::snake_pit, 1);
                add_to_room_bucket(RoomType::damp, rnd::range(1, 3));
                add_to_room_bucket(RoomType::pool, rnd::range(1, 3));
                add_to_room_bucket(RoomType::chasm, 2);
                add_to_room_bucket(RoomType::forest, 2);

                const size_t nr_cave_rooms = room_bucket_.size() * 2;

                add_to_room_bucket(RoomType::cave, nr_cave_rooms);
        }

        rnd::shuffle(room_bucket_);

        TRACE_FUNC_END;
}

Room* make(const RoomType type, const R& r)
{
        switch (type)
        {
        case RoomType::cave:
                return new CaveRoom(r);

        case RoomType::chasm:
                return new ChasmRoom(r);

        case RoomType::crypt:
                return new CryptRoom(r);

        case RoomType::damp:
                return new DampRoom(r);

        case RoomType::pool:
                return new PoolRoom(r);

        case RoomType::human:
                return new HumanRoom(r);

        case RoomType::jail:
                return new JailRoom(r);

        case RoomType::monster:
                return new MonsterRoom(r);

        case RoomType::snake_pit:
                return new SnakePitRoom(r);

        case RoomType::plain:
                return new PlainRoom(r);

        case RoomType::ritual:
                return new RitualRoom(r);

        case RoomType::spider:
                return new SpiderRoom(r);

        case RoomType::forest:
                return new ForestRoom(r);

        case RoomType::corr_link:
                return new CorrLinkRoom(r);

        case RoomType::crumble_room:
                return new CrumbleRoom(r);

        case RoomType::river:
                return new RiverRoom(r);

                // Does not have room classes
        case RoomType::END_OF_STD_ROOMS:
                TRACE << "Illegal room type id: " << (int)type << std::endl;

                ASSERT(false);

                return nullptr;
        }

        ASSERT(false);

        return nullptr;
}

Room* make_random_room(const R& r, const IsSubRoom is_subroom)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        auto room_bucket_it = begin(room_bucket_);

        Room* room = nullptr;

        while (true)
        {
                if (room_bucket_it == end(room_bucket_))
                {
                        // No more rooms to pick from, generate a new bucket
                        init_room_bucket();

                        room_bucket_it = begin(room_bucket_);
                }
                else // There are still room types in the bucket
                {
                        const RoomType room_type = *room_bucket_it;

                        room = make(room_type, r);

                        // NOTE: This must be set before "is_allowed()" below is
                        // called (some room types should never exist as sub
                        // rooms)
                        room->is_sub_room_ = is_subroom == IsSubRoom::yes;

                        StdRoom* const std_room = static_cast<StdRoom*>(room);

                        if (std_room->is_allowed())
                        {
                                room_bucket_.erase(room_bucket_it);

                                TRACE_FUNC_END_VERBOSE << "Made room type: "
                                                       << (int)room_type
                                                       << std::endl;
                                break;
                        }
                        else // Room not allowed (e.g. wrong dimensions)
                        {
                                delete room;

                                // Try next room type in the bucket
                                ++room_bucket_it;
                        }
                }
        }

        TRACE_FUNC_END_VERBOSE;

        return room;
}

} // room_factory

// -----------------------------------------------------------------------------
// Room
// -----------------------------------------------------------------------------
Room::Room(R r, RoomType type) :
        r_(r),
        type_(type),
        is_sub_room_(false) {}

std::vector<P> Room::positions_in_room() const
{
        std::vector<P> positions;

        positions.reserve(r_.w() * r_.h());

        for (int x = r_.p0.x; x <= r_.p1.x; ++x)
        {
                for (int y = r_.p0.y; y <= r_.p1.y; ++y)
                {
                        if (map::room_map.at(x, y) == this)
                        {
                                positions.push_back(P(x, y));
                        }
                }
        }

        return positions;
}

void Room::make_dark() const
{
        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                if (map::room_map.at(i) == this)
                {
                        map::dark.at(i) = true;
                }
        }

        // Also make sub rooms dark
        for (Room* const sub_room : sub_rooms_)
        {
                sub_room->make_dark();
        }
}

// -----------------------------------------------------------------------------
// Standard room
// -----------------------------------------------------------------------------
void StdRoom::on_pre_connect(Array2<bool>& door_proposals)
{
        on_pre_connect_hook(door_proposals);
}

void StdRoom::on_post_connect(Array2<bool>& door_proposals)
{
        place_auto_features();

        on_post_connect_hook(door_proposals);

        // Make the room dark?

        // Do not make the room dark if it has a light source

        bool has_light_source = false;

        for (int x = r_.p0.x; x <= r_.p1.x; ++x)
        {
                for (int y = r_.p0.y; y <= r_.p1.y; ++y)
                {
                        // TODO: This should really be handled in a more generic
                        // way, but currently the only map features that are
                        // light sources are braziers - so it works for now.

                        const auto id = map::cells.at(x, y).rigid->id();

                        if (id == FeatureId::brazier)
                        {
                                has_light_source = true;
                        }
                }
        }

        if (!has_light_source)
        {
                int pct_chance_dark = base_pct_chance_dark(type_) - 15;

                // Increase chance with deeper dungeon levels
                pct_chance_dark += map::dlvl;

                set_constr_in_range(0, pct_chance_dark, 100);

                if (rnd::percent(pct_chance_dark))
                {
                        make_dark();
                }
        }
}

P StdRoom::find_auto_feature_placement(const std::vector<P>& adj_to_walls,
                                       const std::vector<P>& away_from_walls,
                                       const FeatureId id) const
{
        TRACE_FUNC_BEGIN_VERBOSE;

        const bool is_adj_to_walls_avail = !adj_to_walls.empty();
        const bool is_away_from_walls_avail = !away_from_walls.empty();

        if (!is_adj_to_walls_avail &&
            !is_away_from_walls_avail)
        {
                TRACE_FUNC_END_VERBOSE << "No eligible cells found"
                                       << std::endl;

                return P(-1, -1);
        }

        // TODO: This method is crap, use a bucket instead!

        const int nr_attempts_to_find_pos = 100;

        for (int i = 0; i < nr_attempts_to_find_pos; ++i)
        {
                const FeatureData& d = feature_data::data(id);

                if (is_adj_to_walls_avail &&
                    d.auto_spawn_placement == FeaturePlacement::adj_to_walls)
                {
                        TRACE_FUNC_END_VERBOSE;
                        return rnd::element(adj_to_walls);
                }

                if (is_away_from_walls_avail &&
                    d.auto_spawn_placement == FeaturePlacement::away_from_walls)
                {
                        TRACE_FUNC_END_VERBOSE;
                        return rnd::element(away_from_walls);
                }

                if (d.auto_spawn_placement == FeaturePlacement::either)
                {
                        if (rnd::coin_toss())
                        {
                                if (is_adj_to_walls_avail)
                                {
                                        TRACE_FUNC_END_VERBOSE;
                                        return rnd::element(adj_to_walls);

                                }
                        }
                        else // Coint toss
                        {
                                if (is_away_from_walls_avail)
                                {
                                        TRACE_FUNC_END_VERBOSE;
                                        return rnd::element(away_from_walls);
                                }
                        }
                }
        }

        TRACE_FUNC_END_VERBOSE;
        return P(-1, -1);
}

void StdRoom::place_auto_features()
{
        TRACE_FUNC_BEGIN;

        // Make a feature bucket
        std::vector<FeatureId> feature_bucket;

        const auto rules = auto_features_allowed();

        for (const auto& rule : rules)
        {
                // Insert N elements of the given Feature ID
                feature_bucket.insert(end(feature_bucket),
                                      rule.nr_allowed,
                                      rule.feature_id);
        }

        std::vector<P> adj_to_walls_bucket;
        std::vector<P> away_from_walls_bucket;

        map_patterns::cells_in_room(*this,
                                    adj_to_walls_bucket,
                                    away_from_walls_bucket);

        while (!feature_bucket.empty())
        {
                // TODO: Do a random shuffle of the bucket instead, and pop
                // elements
                const size_t feature_idx =
                        rnd::range(0, feature_bucket.size() - 1);

                const FeatureId id =
                        feature_bucket[feature_idx];

                feature_bucket.erase(begin(feature_bucket) + feature_idx);

                const P p = find_auto_feature_placement(adj_to_walls_bucket,
                                                        away_from_walls_bucket,
                                                        id);

                if (p.x >= 0)
                {
                        // A good position was found

                        const FeatureData& d = feature_data::data(id);

                        TRACE_VERBOSE << "Placing feature" << std::endl;

                        ASSERT(map::is_pos_inside_outer_walls(p));

                        map::put(static_cast<Rigid*>(d.make_obj(p)));

                        // Erase all adjacent positions
                        auto is_adj = [&](const P& other_p)
                                {
                                        return is_pos_adj(p, other_p, true);
                                };

                        adj_to_walls_bucket.erase(
                                remove_if(begin(adj_to_walls_bucket),
                                          end(adj_to_walls_bucket),
                                          is_adj),
                                end(adj_to_walls_bucket));

                        away_from_walls_bucket.erase(
                                remove_if(begin(away_from_walls_bucket),
                                          end(away_from_walls_bucket),
                                          is_adj),
                                end(away_from_walls_bucket));
                }
        }
}

// -----------------------------------------------------------------------------
// Plain room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> PlainRoom::auto_features_allowed() const
{
        const int fountain_one_in_n =
                (map::dlvl <= 4) ? 7 :
                (map::dlvl <= dlvl_last_mid_game) ? 10 :
                20;

        return
        {
                {FeatureId::brazier, rnd::one_in(4) ? 1 : 0},
                {FeatureId::statue, rnd::one_in(7) ? rnd::range(1, 2) : 0},
                {FeatureId::fountain, rnd::one_in(fountain_one_in_n) ? 1 : 0},
                {FeatureId::chains, rnd::one_in(7) ? rnd::range(1, 2) : 0}
        };
}

void PlainRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                mapgen::cut_room_corners(*this);
        }

        if (rnd::fraction(1, 4))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void PlainRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;
}

// -----------------------------------------------------------------------------
// Human room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> HumanRoom::auto_features_allowed() const
{
        std::vector<RoomAutoFeatureRule> result;

        result.push_back({FeatureId::brazier,   rnd::range(0, 2)});
        result.push_back({FeatureId::statue,    rnd::range(0, 2)});

        // Control how many item container features that can spawn in the room
        std::vector<FeatureId> item_containers =
                {
                        FeatureId::chest,
                        FeatureId::cabinet,
                        FeatureId::bookshelf,
                        FeatureId::alchemist_bench
                };

        const int nr_item_containers = rnd::range(0, 2);

        for (int i = 0; i < nr_item_containers; ++i)
        {
                result.push_back({rnd::element(item_containers), 1});
        }

        return result;
}

bool HumanRoom::is_allowed() const
{
        return
                r_.min_dim() >= 3 &&
                r_.max_dim() <= 8;
}

void HumanRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                mapgen::cut_room_corners(*this);
        }

        if (rnd::fraction(1, 4))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void HumanRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                Array2<bool> blocked(map::dims());

                map_parsers::BlocksMoveCommon(ParseActors::no)
                        .run(blocked, blocked.rect());

                for (int x = r_.p0.x + 1; x <= r_.p1.x - 1; ++x)
                {
                        for (int y = r_.p0.y + 1; y <= r_.p1.y - 1; ++y)
                        {
                                if (!blocked.at(x, y) &&
                                    map::room_map.at(x, y) == this)
                                {
                                        Carpet* const carpet =
                                                new Carpet(P(x, y));

                                        map::put(carpet);
                                }
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// Jail room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> JailRoom::auto_features_allowed() const
{
        return
        {
                {FeatureId::chains, rnd::range(2, 8)},
                {FeatureId::brazier, rnd::one_in(4) ? 1 : 0},
                {FeatureId::rubble_low, rnd::range(1, 4)}
        };
}

void JailRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                mapgen::cut_room_corners(*this);
        }

        if (rnd::fraction(1, 4))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void JailRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;
}

// -----------------------------------------------------------------------------
// Ritual room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> RitualRoom::auto_features_allowed() const
{
        return
        {
                {FeatureId::altar, 1},
                {FeatureId::alchemist_bench, rnd::one_in(4) ? 1 : 0},
                {FeatureId::brazier, rnd::range(2, 4)},
                {FeatureId::chains, rnd::one_in(7) ? rnd::range(1, 2) : 0}
        };
}

bool RitualRoom::is_allowed() const
{
        return
                r_.min_dim() >= 4 &&
                r_.max_dim() <= 8;
}

void RitualRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                mapgen::cut_room_corners(*this);
        }

        if (rnd::fraction(1, 4))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void RitualRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksMoveCommon(ParseActors::no)
                .run(blocked, blocked.rect());

        const int bloody_chamber_pct = 60;

        if (rnd::percent(bloody_chamber_pct))
        {
                P origin(-1, -1);
                std::vector<P> origin_bucket;

                for (int y = r_.p0.y; y <= r_.p1.y; ++y)
                {
                        for (int x = r_.p0.x; x <= r_.p1.x; ++x)
                        {
                                if (map::cells.at(x, y).rigid->id() ==
                                    FeatureId::altar)
                                {
                                        origin = P(x, y);
                                        y = 999;
                                        x = 999;
                                }
                                else
                                {
                                        if (!blocked.at(x, y))
                                        {
                                                origin_bucket.push_back(
                                                        P(x, y));
                                        }
                                }
                        }
                }

                if (!origin_bucket.empty())
                {
                        if (origin.x == -1)
                        {
                                const int element =
                                        rnd::range(0, origin_bucket.size() - 1);

                                origin = origin_bucket[element];
                        }

                        for (const P& d : dir_utils::dir_list)
                        {
                                if (rnd::percent(bloody_chamber_pct / 2))
                                {
                                        const P pos = origin + d;

                                        if (!blocked.at(pos))
                                        {
                                                map::make_gore(pos);
                                                map::make_blood(pos);
                                        }
                                }
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// Spider room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> SpiderRoom::auto_features_allowed() const
{
        return
        {
                {FeatureId::cocoon, rnd::range(0, 3)}
        };
}

bool SpiderRoom::is_allowed() const
{
        return
                r_.min_dim() >= 3 &&
                r_.max_dim() <= 8;
}

void SpiderRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        const bool is_early = map::dlvl <= dlvl_last_early_game;

        const bool is_mid = !is_early && (map::dlvl <= dlvl_last_mid_game);

        if (is_early || (is_mid && rnd::coin_toss()))
        {
                if (rnd::coin_toss())
                {
                        mapgen::cut_room_corners(*this);
                }
        }
        else
        {
                mapgen::cavify_room(*this);
        }

        if ((is_early || is_mid) && rnd::fraction(1, 4))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void SpiderRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;
}

// -----------------------------------------------------------------------------
// Snake pit room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> SnakePitRoom::auto_features_allowed() const
{
        return {};
}

bool SnakePitRoom::is_allowed() const
{
        return
                r_.min_dim() >= 2 &&
                r_.max_dim() <= 6;
}

void SnakePitRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        const bool is_early = map::dlvl <= dlvl_last_early_game;
        const bool is_mid = !is_early && map::dlvl <= dlvl_last_mid_game;

        if (is_early || is_mid)
        {
                if (rnd::fraction(3, 4))
                {
                        mapgen::make_pillars_in_room(*this);
                }
        }
        else // Is late game
        {
                mapgen::cavify_room(*this);
        }
}

void SnakePitRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        // Put lots of rubble, to make the room more "pit like"
        for (int x = r_.p0.x; x <= r_.p1.x; ++x)
        {
                for (int y = r_.p0.y; y <= r_.p1.y; ++y)
                {
                        if (map::room_map.at(x, y) != this)
                        {
                                continue;
                        }

                        const P p(x, y);

                        if (map::cells.at(x, y).rigid->can_have_rigid() &&
                            rnd::coin_toss())
                        {
                                map::put(new RubbleLow(p));
                        }
                }
        }

        // Put some monsters in the room
        std::vector<ActorId> actor_id_bucket;

        for (size_t i = 0; i < (size_t)ActorId::END; ++i)
        {
                const ActorData& d = actor_data::data[i];

                // NOTE: We do not allow Spitting Cobras in snake pits, because
                // it's VERY tedious to fight swarms of them (attack, get
                // blinded, back away, repeat...)
                if (d.is_snake &&
                    (d.id != ActorId::spitting_cobra))
                {
                        actor_id_bucket.push_back(d.id);
                }
        }

        // Hijacking snake pit rooms to make a worm room...
        if (map::dlvl <= dlvl_last_mid_game)
        {
                actor_id_bucket.push_back(ActorId::worm_mass);
        }

        if (map::dlvl >= 3)
        {
                actor_id_bucket.push_back(ActorId::mind_worms);
        }

        const auto actor_id = rnd::element(actor_id_bucket);

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksMoveCommon(ParseActors::yes)
                .run(blocked,
                     blocked.rect(),
                     MapParseMode::overwrite);

        const int min_dist_to_player = fov_radi_int + 1;

        const P& player_pos = map::player->pos;

        const int x0 = std::max(
                0,
                player_pos.x - min_dist_to_player);

        const int y0 = std::max(
                0,
                player_pos.y - min_dist_to_player);

        const int x1 = std::min(
                map::w() - 1,
                player_pos.x + min_dist_to_player);

        const int y1 = std::min(
                map::h() - 1,
                player_pos.y + min_dist_to_player);

        for (int x = x0; x <= x1; ++x)
        {
                for (int y = y0; y <= y1; ++y)
                {
                        blocked.at(x, y) = true;
                }
        }

        const int nr_groups = rnd::range(1, 2);

        for (int group_idx = 0; group_idx < nr_groups; ++group_idx)
        {
                // Find an origin
                std::vector<P> origin_bucket;

                for (int x = r_.p0.x; x <= r_.p1.x; ++x)
                {
                        for (int y = r_.p0.y; y <= r_.p1.y; ++y)
                        {
                                if (!blocked.at(x, y) &&
                                    (map::room_map.at(x, y) == this))
                                {
                                        origin_bucket.push_back(P(x, y));
                                }
                        }
                }

                if (origin_bucket.empty())
                {
                        return;
                }

                const P origin(rnd::element(origin_bucket));

                const auto sorted_free_cells =
                        populate_mon::make_sorted_free_cells(origin, blocked);

                if (sorted_free_cells.empty())
                {
                        return;
                }

                populate_mon::make_group_at(
                        actor_id,
                        sorted_free_cells,
                        &blocked, // New blocked cells (output)
                        MonRoamingAllowed::yes);
        }
}

// -----------------------------------------------------------------------------
// Crypt room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> CryptRoom::auto_features_allowed() const
{
        return
        {
                {FeatureId::tomb, rnd::one_in(6) ? 2 : 1},
                {FeatureId::rubble_low, rnd::range(1, 4)}
        };
}

bool CryptRoom::is_allowed() const
{
        return
                r_.min_dim() >= 2 &&
                r_.max_dim() <= 12;
}

void CryptRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        if (rnd::coin_toss())
        {
                mapgen::cut_room_corners(*this);
        }

        if (rnd::fraction(1, 3))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void CryptRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;
}

// -----------------------------------------------------------------------------
// Monster room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> MonsterRoom::auto_features_allowed() const
{
        return
        {
                {FeatureId::rubble_low, rnd::range(3, 6)}
        };
}

bool MonsterRoom::is_allowed() const
{
        return
                r_.min_dim() >= 4 &&
                r_.max_dim() <= 8;
}

void MonsterRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        const bool is_early =
                map::dlvl <= dlvl_last_early_game;

        const bool is_mid =
                !is_early &&
                (map::dlvl <= dlvl_last_mid_game);

        if (is_early || is_mid)
        {
                if (rnd::fraction(3, 4))
                {
                        mapgen::cut_room_corners(*this);
                }

                if (rnd::fraction(1, 3))
                {
                        mapgen::make_pillars_in_room(*this);
                }
        }
        else // Is late game
        {
                mapgen::cavify_room(*this);
        }
}

void MonsterRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksMoveCommon(ParseActors::no)
                .run(blocked, blocked.rect());

        int nr_blood_put = 0;

        // TODO: Hacky, needs improving
        const int nr_tries = 1000;

        for (int i = 0; i < nr_tries; ++i)
        {
                for (int x = r_.p0.x; x <= r_.p1.x; ++x)
                {
                        for (int y = r_.p0.y; y <= r_.p1.y; ++y)
                        {
                                if (!blocked.at(x, y) &&
                                    map::room_map.at(x, y) == this &&
                                    rnd::fraction(2, 5))
                                {
                                        map::make_gore(P(x, y));
                                        map::make_blood(P(x, y));
                                        nr_blood_put++;
                                }
                        }
                }

                if (nr_blood_put > 0)
                {
                        break;
                }
        }
}

// -----------------------------------------------------------------------------
// Damp room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> DampRoom::auto_features_allowed() const
{
        return
        {
                {FeatureId::vines, rnd::coin_toss() ? rnd::range(2, 8) : 0}
        };
}

bool DampRoom::is_allowed() const
{
        return true;
}

void DampRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        const bool is_early = map::dlvl <= dlvl_last_early_game;

        const bool is_mid = !is_early && (map::dlvl <= dlvl_last_mid_game);

        if (is_early || (is_mid && rnd::coin_toss()))
        {
                if (rnd::coin_toss())
                {
                        mapgen::cut_room_corners(*this);
                }
        }
        else
        {
                mapgen::cavify_room(*this);
        }

        if ((is_early || is_mid) && rnd::fraction(1, 3))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void DampRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksMoveCommon(ParseActors::no)
                .run(blocked, blocked.rect());

        const int liquid_one_in_n = rnd::range(2, 5);

        const auto liquid_type =
                rnd::fraction(3, 4)
                ? LiquidType::water
                : LiquidType::mud;

        for (int x = r_.p0.x; x <= r_.p1.x; ++x)
        {
                for (int y = r_.p0.y; y <= r_.p1.y; ++y)
                {
                        if (!blocked.at(x, y) &&
                            map::room_map.at(x, y) == this &&
                            rnd::one_in(liquid_one_in_n))
                        {
                                auto* const liquid = new LiquidShallow(P(x, y));

                                liquid->type_ = liquid_type;

                                map::put(liquid);
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// Pool room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> PoolRoom::auto_features_allowed() const
{
        return
        {
                {FeatureId::vines, rnd::coin_toss() ? rnd::range(2, 8) : 0}
        };
}

bool PoolRoom::is_allowed() const
{
        return r_.min_dim() >= 5;
}

void PoolRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        const bool is_early = map::dlvl <= dlvl_last_early_game;

        const bool is_mid = !is_early && (map::dlvl <= dlvl_last_mid_game);

        if (is_early ||
            (is_mid && rnd::coin_toss()) ||
            is_sub_room_)
        {
                if (rnd::coin_toss())
                {
                        mapgen::cut_room_corners(*this);
                }
        }
        else
        {
                mapgen::cavify_room(*this);
        }

        if ((is_early || is_mid) && rnd::fraction(1, 3))
        {
                mapgen::make_pillars_in_room(*this);
        }
}

void PoolRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksMoveCommon(ParseActors::no)
                .run(blocked, blocked.rect());

        std::vector<P> origin_bucket;

        origin_bucket.reserve(r_.w() * r_.h());

        for (int x = 0; x < blocked.w(); ++x)
        {
                for (int y = 0; y < blocked.w(); ++y)
                {
                        if (blocked.at(x, y))
                        {
                                continue;
                        }

                        if (map::room_map.at(x, y) == this)
                        {
                                origin_bucket.push_back(P(x, y));
                        }
                        else
                        {
                                blocked.at(x, y) = true;
                        }
                }
        }

        if (origin_bucket.empty())
        {
                // There are no free room positions
                ASSERT(false);

                return;
        }

        const P origin = rnd::element(origin_bucket);

        const int flood_travel_limit = 12;

        const bool allow_diagonal_flood = true;

        const auto flood =
                floodfill(
                        origin,
                        blocked,
                        flood_travel_limit,
                        P(-1, -1), // Target cell
                        allow_diagonal_flood);

        auto should_put_liquid = [](
                const P& pos,
                const Array2<int>& flood,
                const P& origin) {

                return (flood.at(pos)) > 0 || (pos == origin);
        };

        // Do not place any liquid if we cannot place at least a certain amount
        {
                const int min_nr_liquid_cells = 9;

                int nr_liquid_cells = 0;

                for (int x = 0; x < flood.w(); ++x)
                {
                        for (int y = 0; y < flood.w(); ++y)
                        {
                                if (should_put_liquid(P(x, y), flood, origin))
                                {
                                        ++nr_liquid_cells;
                                }
                        }
                }

                if (nr_liquid_cells < min_nr_liquid_cells)
                {
                        return;
                }
        }

        // OK, we can place liquid

        bool is_natural_pool = false;

        if (map::dlvl >= dlvl_first_late_game)
        {
                is_natural_pool = true;
        }
        else if (map::dlvl >= dlvl_first_mid_game)
        {
                is_natural_pool = rnd::coin_toss();
        }
        else
        {
                is_natural_pool = rnd::fraction(1, 4);
        }

        for (int x = 0; x < flood.w(); ++x)
        {
                for (int y = 0; y < flood.w(); ++y)
                {
                        const P p(x, y);

                        if (!should_put_liquid(p, flood, origin))
                        {
                                continue;
                        }

                        if (!is_natural_pool ||
                            (flood.at(p) < (flood_travel_limit / 2)) ||
                            rnd::coin_toss())
                        {
                                auto* const liquid = new LiquidDeep(p);

                                liquid->type_ = LiquidType::water;

                                map::put(liquid);
                        }
                        else if (rnd::fraction(2, 3))
                        {
                                auto* const liquid = new LiquidShallow(p);

                                liquid->type_ = LiquidType::water;

                                map::put(liquid);
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// Cave room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> CaveRoom::auto_features_allowed() const
{
        return
        {
                {FeatureId::rubble_low, rnd::range(2, 4)},
                {FeatureId::stalagmite, rnd::range(1, 4)}
        };
}

bool CaveRoom::is_allowed() const
{
        return !is_sub_room_;
}

void CaveRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        mapgen::cavify_room(*this);
}

void CaveRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;
}

// -----------------------------------------------------------------------------
// Forest room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> ForestRoom::auto_features_allowed() const
{
        return
        {
                {FeatureId::brazier, rnd::range(0, 1)}
        };
}

bool ForestRoom::is_allowed() const
{
        // TODO: Also check sub_rooms_.empty() ?
        return
                !is_sub_room_ &&
                r_.min_dim() >= 5;
}

void ForestRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        mapgen::cavify_room(*this);
}

void ForestRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksMoveCommon(ParseActors::no)
                .run(blocked, blocked.rect());

        // Do not consider doors blocking
        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                if (map::cells.at(i).rigid->id() == FeatureId::door)
                {
                        blocked.at(i) = false;
                }
        }

        std::vector<P> tree_pos_bucket;

        for (int x = r_.p0.x; x <= r_.p1.x; ++x)
        {
                for (int y = r_.p0.y; y <= r_.p1.y; ++y)
                {
                        if (!blocked.at(x, y) &&
                            map::room_map.at(x, y) == this)
                        {
                                const P p(x, y);

                                tree_pos_bucket.push_back(p);

                                if (rnd::one_in(10))
                                {
                                        map::put(new Bush(p));
                                }
                                else
                                {
                                        map::put(new Grass(p));
                                }
                        }
                }
        }

        rnd::shuffle(tree_pos_bucket);

        int nr_trees_placed = 0;

        const int tree_one_in_n = rnd::range(3, 10);

        while (!tree_pos_bucket.empty())
        {
                const P p = tree_pos_bucket.back();

                tree_pos_bucket.pop_back();

                if (rnd::one_in(tree_one_in_n))
                {
                        blocked.at(p) = true;

                        if (map_parsers::is_map_connected(blocked))
                        {
                                map::put(new Tree(p));

                                ++nr_trees_placed;
                        }
                        else
                        {
                                blocked.at(p) = false;
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// Chasm room
// -----------------------------------------------------------------------------
std::vector<RoomAutoFeatureRule> ChasmRoom::auto_features_allowed() const
{
        return {};
}

bool ChasmRoom::is_allowed() const
{
        return
                r_.min_dim() >= 5 &&
                r_.max_dim() <= 9;
}

void ChasmRoom::on_pre_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        mapgen::cavify_room(*this);
}

void ChasmRoom::on_post_connect_hook(Array2<bool>& door_proposals)
{
        (void)door_proposals;

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksMoveCommon(ParseActors::no)
                .run(blocked, blocked.rect());

        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                if (map::room_map.at(i) != this)
                {
                        blocked.at(i) = true;
                }
        }

        blocked = map_parsers::expand(blocked, blocked.rect());

        P origin;

        for (int x = r_.p0.x; x <= r_.p1.x; ++x)
        {
                for (int y = r_.p0.y; y <= r_.p1.y; ++y)
                {
                        if (!blocked.at(x, y))
                        {
                                origin.set(x, y);
                        }
                }
        }

        const auto flood = floodfill(
                origin,
                blocked,
                10000,
                P(-1, -1),
                false);

        for (int x = r_.p0.x; x <= r_.p1.x; ++x)
        {
                for (int y = r_.p0.y; y <= r_.p1.y; ++y)
                {
                        const P p(x, y);

                        if (p == origin || flood.at(x, y) != 0)
                        {
                                map::put(new Chasm(p));
                        }
                }
        }
}

// -----------------------------------------------------------------------------
// River room
// -----------------------------------------------------------------------------
void RiverRoom::on_pre_connect(Array2<bool>& door_proposals)
{
        TRACE_FUNC_BEGIN;

        // Strategy: Expand the the river on both sides until parallel to the
        // closest center cell of another room

        const bool is_hor = axis_ == Axis::hor;

        TRACE << "Finding room centers" << std::endl;
        Array2<bool> centers(map::dims());

        for (Room* const room : map::room_list)
        {
                if (room != this)
                {
                        const P c_pos(room->r_.center());

                        centers.at(c_pos) = true;
                }
        }

        TRACE <<
                "Finding closest room center coordinates on both sides "
                "(y coordinate if horizontal river, x if vertical)"
              << std::endl;

        int closest_center0 = -1;
        int closest_center1 = -1;

        // Using nestled scope to avoid declaring x and y at function scope
        {
                int x, y;

                // i_outer and i_inner should be references to x or y.
                auto find_closest_center0 =
                        [&](const Range & r_outer,
                            const Range & r_inner,
                            int& i_outer,
                            int& i_inner) {

                        for (i_outer = r_outer.min;
                             i_outer >= r_outer.max;
                             --i_outer)
                        {
                                for (i_inner = r_inner.min;
                                     i_inner <= r_inner.max;
                                     ++i_inner)
                                {
                                        if (centers.at(x, y))
                                        {
                                                closest_center0 = i_outer;
                                                break;
                                        }
                                }

                                if (closest_center0 != -1)
                                {
                                        break;
                                }
                        }
                };

                auto find_closest_center1 =
                        [&](const Range & r_outer,
                            const Range & r_inner,
                            int& i_outer,
                            int& i_inner) {

                        for (i_outer = r_outer.min;
                             i_outer <= r_outer.max;
                             ++i_outer)
                        {
                                for (i_inner = r_inner.min;
                                     i_inner <= r_inner.max;
                                     ++i_inner)
                                {
                                        if (centers.at(x, y))
                                        {
                                                closest_center1 = i_outer;
                                                break;
                                        }
                                }

                                if (closest_center1 != -1) {break;}
                        }
                };

                if (is_hor)
                {
                        const int river_y = r_.p0.y;

                        find_closest_center0(
                                Range(river_y - 1, 1),
                                Range(1, map::w() - 2), y, x);

                        find_closest_center1(
                                Range(river_y + 1, map::h() - 2),
                                Range(1, map::w() - 2), y, x);
                }
                else // Vertical
                {
                        const int river_x = r_.p0.x;

                        find_closest_center0(
                                Range(river_x - 1, 1),
                                Range(1, map::h() - 2), x, y);

                        find_closest_center1(
                                Range(river_x + 1, map::w() - 2),
                                Range(1, map::h() - 2), x, y);
                }
        }

        TRACE << "Expanding and filling river" << std::endl;

        Array2<bool> blocked(map::dims());

        // Within the expansion limits, mark all cells not belonging to another
        // room as free. All other cells are considered as blocking.
        for (int x = 0; x < map::w(); ++x)
        {
                for (int y = 0; y < map::h(); ++y)
                {
                        blocked.at(x, y) = true;

                        if ((axis_ == Axis::hor &&
                             (y >= closest_center0 && y <= closest_center1)) ||
                            (axis_ == Axis::ver &&
                             (x >= closest_center0 && x <= closest_center1)))
                        {
                                Room* r = map::room_map.at(x, y);

                                blocked.at(x, y) = r && r != this;
                        }
                }
        }

        blocked = map_parsers::expand(blocked, blocked.rect());

        const P origin(r_.center());

        const auto flood = floodfill(
                origin,
                blocked,
                INT_MAX,
                P(-1, -1),
                true);

        for (int x = 0; x < map::w(); ++x)
        {
                for (int y = 0; y < map::h(); ++y)
                {
                        const P p(x, y);

                        if (flood.at(x, y) > 0 || p == origin)
                        {
                                map::put(new Chasm(p));
                                map::room_map.at(x, y) = this;
                                r_.p0.x = std::min(r_.p0.x, x);
                                r_.p0.y = std::min(r_.p0.y, y);
                                r_.p1.x = std::max(r_.p1.x, x);
                                r_.p1.y = std::max(r_.p1.y, y);
                        }
                }
        }

        TRACE << "Making bridge(s)" << std::endl;

        // Mark which side each cell belongs to
        enum Side
        {
                in_river,
                side0,
                side1
        };

        Array2<Side> sides(map::dims());

        // Scoping to avoid declaring x and y at function scope
        {
                int x, y;

                // i_outer and i_inner should be references to x or y.
                auto mark_sides =
                        [&](const Range & r_outer,
                            const Range & r_inner,
                            int& i_outer,
                            int& i_inner) {

                        for (i_outer = r_outer.min;
                             i_outer <= r_outer.max;
                             ++i_outer)
                        {
                                bool is_on_side0 = true;

                                for (i_inner = r_inner.min;
                                     i_inner <= r_inner.max;
                                     ++i_inner)
                                {
                                        if (map::room_map.at(x, y) == this)
                                        {
                                                is_on_side0 = false;
                                                sides.at(x, y) = in_river;
                                        }
                                        else
                                        {
                                                sides.at(x, y) =
                                                        is_on_side0
                                                        ? side0
                                                        : side1;
                                        }
                                }
                        }
                };

                if (axis_ == Axis::hor)
                {
                        mark_sides(Range(1, map::w() - 2),
                                   Range(1, map::h() - 2), x, y);
                }
                else
                {
                        mark_sides(Range(1, map::h() - 2),
                                   Range(1, map::w() - 2), y, x);
                }
        }

        Array2<bool> valid_room_entries0(map::dims());
        Array2<bool> valid_room_entries1(map::dims());

        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                valid_room_entries0.at(i) = valid_room_entries1.at(i) = false;
        }

        const int edge_d = 4;

        for (int x = edge_d; x < map::w() - edge_d; ++x)
        {
                for (int y = edge_d; y < map::h() - edge_d; ++y)
                {
                        const FeatureId feature_id =
                                map::cells.at(x, y).rigid->id();

                        if ((feature_id != FeatureId::wall) ||
                            map::room_map.at(x, y))
                        {
                                continue;
                        }

                        const P p(x, y);
                        int nr_cardinal_floor = 0;
                        int nr_cardinal_river = 0;

                        for (const auto& d : dir_utils::cardinal_list)
                        {
                                const auto p_adj(p + d);

                                const auto* const f =
                                        map::cells.at(p_adj).rigid;

                                if (f->id() == FeatureId::floor)
                                {
                                        nr_cardinal_floor++;
                                }

                                if (map::room_map.at(p_adj) == this)
                                {
                                        nr_cardinal_river++;
                                }
                        }

                        if (nr_cardinal_floor == 1 &&
                            nr_cardinal_river == 1)
                        {
                                switch (sides.at(x, y))
                                {
                                case side0:
                                        valid_room_entries0.at(x, y) = true;
                                        break;

                                case side1:
                                        valid_room_entries1.at(x, y) = true;
                                        break;

                                case in_river:
                                        break;
                                }
                        }
                }
        }

#ifndef NDEBUG
        if (init::is_demo_mapgen)
        {
                states::draw();

                for (int y = 1; y < map::h() - 1; ++y)
                {
                        for (int x = 1; x < map::w() - 1; ++x)
                        {
                                P p(x, y);

                                if (valid_room_entries0.at(x, y))
                                {
                                        io::draw_character(
                                                '0',
                                                Panel::map,
                                                p,
                                                colors::light_red());
                                }

                                if (valid_room_entries1.at(x, y))
                                {
                                        io::draw_character(
                                                '1',
                                                Panel::map,
                                                p,
                                                colors::light_red());
                                }

                                if (valid_room_entries0.at(x, y) ||
                                    valid_room_entries1.at(x, y))
                                {
                                        io::update_screen();
                                        sdl_base::sleep(100);
                                }
                        }
                }
        }
#endif // NDEBUG

        std::vector<int> positions(
                is_hor
                ? map::w()
                : map::h());

        std::iota(begin(positions), end(positions), 0);

        rnd::shuffle(positions);

        std::vector<int> c_built;

        const int min_edge_dist = 6;

        const int max_nr_bridges = rnd::range(1, 3);

        for (const int bridge_n : positions)
        {
                if ((bridge_n < min_edge_dist) ||
                    (is_hor && (bridge_n > (map::w() - 1 - min_edge_dist))) ||
                    (!is_hor && (bridge_n > (map::h() - 1 - min_edge_dist))))
                {
                        continue;
                }

                bool is_too_close_to_other_bridge = false;

                const int min_d = 2;

                for (int c_other : c_built)
                {
                        const Range r = Range(c_other - min_d,
                                              c_other + min_d);

                        const bool is_in_range =
                                is_val_in_range(bridge_n, r);

                        if (is_in_range)
                        {
                                is_too_close_to_other_bridge = true;
                                break;
                        }
                }

                if (is_too_close_to_other_bridge)
                {
                        continue;
                }

                // Check if current bridge coord would connect matching room
                // connections. If so both room_con0 and room_con1 will be set.
                P room_con0(-1, -1);
                P room_con1(-1, -1);

                const int c0_0 = is_hor ? r_.p1.y : r_.p1.x;
                const int c1_0 = is_hor ? r_.p0.y : r_.p0.x;

                for (int c = c0_0; c != c1_0; --c)
                {
                        if ((is_hor && sides.at(bridge_n, c) == side0) ||
                            (!is_hor && sides.at(c, bridge_n) == side0))
                        {
                                break;
                        }

                        const P p_nxt =
                                is_hor ?
                                P(bridge_n, c - 1) :
                                P(c - 1, bridge_n);

                        if (valid_room_entries0.at(p_nxt))
                        {
                                room_con0 = p_nxt;
                                break;
                        }
                }

                const int c0_1 = is_hor ? r_.p0.y : r_.p0.x;
                const int c1_1 = is_hor ? r_.p1.y : r_.p1.x;

                for (int c = c0_1; c != c1_1; ++c)
                {
                        if ((is_hor && sides.at(bridge_n, c) == side1) ||
                            (!is_hor && sides.at(c, bridge_n) == side1))
                        {
                                break;
                        }

                        const P p_nxt =
                                is_hor ?
                                P(bridge_n, c + 1) :
                                P(c + 1, bridge_n);

                        if (valid_room_entries1.at(p_nxt))
                        {
                                room_con1 = p_nxt;
                                break;
                        }
                }

                // Make the bridge if valid connection pairs found
                if (room_con0.x != -1 && room_con1.x != -1)
                {
#ifndef NDEBUG
                        if (init::is_demo_mapgen)
                        {
                                states::draw();

                                io::draw_character('0',
                                                   Panel::map,
                                                   room_con0,
                                                   colors::light_green());

                                io::draw_character('1',
                                                   Panel::map,
                                                   room_con1,
                                                   colors::yellow());

                                io::update_screen();

                                sdl_base::sleep(2000);
                        }
#endif // NDEBUG

                        TRACE << "Found valid connection pair at: "
                              << room_con0.x << "," << room_con0.y << " / "
                              << room_con1.x << "," << room_con1.y << std::endl
                              << "Making bridge at pos: " << bridge_n
                              << std::endl;

                        if (is_hor)
                        {
                                for (int y = room_con0.y; y <= room_con1.y; ++y)
                                {
                                        if (map::room_map.at(bridge_n, y) ==
                                            this)
                                        {
                                                auto* const floor =
                                                        new Floor(
                                                                {bridge_n, y});

                                                floor->type_ =
                                                        FloorType::common;

                                                map::put(floor);
                                        }
                                }
                        }
                        else // Vertical
                        {
                                for (int x = room_con0.x; x <= room_con1.x; ++x)
                                {
                                        if (map::room_map.at(x, bridge_n) ==
                                            this)
                                        {
                                                auto* const floor =
                                                        new Floor(
                                                                {x, bridge_n});

                                                floor->type_ =
                                                        FloorType::common;

                                                map::put(floor);
                                        }
                                }
                        }

                        map::put(new Floor(room_con0));
                        map::put(new Floor(room_con1));

                        door_proposals.at(room_con0) = true;
                        door_proposals.at(room_con1) = true;

                        c_built.push_back(bridge_n);
                }

                if (int (c_built.size()) >= max_nr_bridges)
                {
                        TRACE << "Enough bridges built" << std::endl;
                        break;
                }
        }

        TRACE << "Bridges built/attempted: "
              << c_built.size() << "/"
              << max_nr_bridges << std::endl;

        if (c_built.empty())
        {
                mapgen::is_map_valid = false;
        }
        else // Map is valid (at least one bridge was built)
        {
                Array2<bool> valid_room_entries(map::dims());

                for (int x = 0; x < map::w(); ++x)
                {
                        for (int y = 0; y < map::h(); ++y)
                        {
                                valid_room_entries.at(x, y) =
                                        valid_room_entries0.at(x, y) ||
                                        valid_room_entries1.at(x, y);

                                // Convert some remaining valid room entries
                                if (valid_room_entries.at(x, y) &&
                                    (find(begin(c_built), end(c_built), x) ==
                                     end(c_built)))
                                {
                                        map::put(new Floor(P(x, y)));

                                        map::room_map.at(x, y) = this;
                                }
                        }
                }

                // Convert wall cells adjacent to river cells to river
                valid_room_entries = map_parsers::expand(valid_room_entries, 2);

                for (int x = 2; x < map::w() - 2; ++x)
                {
                        for (int y = 2; y < map::h() - 2; ++y)
                        {
                                if (valid_room_entries.at(x, y) &&
                                    map::room_map.at(x, y) == this)
                                {
                                        auto* const floor = new Floor(P(x, y));

                                        floor->type_ = FloorType::common;

                                        map::put(floor);

                                        map::room_map.at(x, y) = nullptr;
                                }
                        }
                }
        }

        TRACE_FUNC_END;

} // RiverRoom::on_pre_connect
