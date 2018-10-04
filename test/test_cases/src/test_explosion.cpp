#include "catch.hpp"

#include "actor.hpp"
#include "actor_death.hpp"
#include "actor_factory.hpp"
#include "explosion.hpp"
#include "feature_rigid.hpp"
#include "map.hpp"
#include "property.hpp"
#include "rl_utils.hpp"
#include "test_utils.hpp"

TEST_CASE("Explosions damage walls")
{
        test_utils::init_all();

        for (int x = 0; x < map::w(); ++x)
        {
                for (int y = 0; y < map::h(); ++y)
                {
                        map::put(new Wall({x, y}));
                }
        }

        const P origin(5, 7);

        map::put(new Floor(origin));

        // Run enough explosions to guarantee destroying adjacent walls
        for (int i = 0; i < 1000; ++i)
        {
                explosion::run(origin, ExplType::expl);
        }

        int nr_destroyed = 0;
        int nr_walls = 0;

        for (int x = (origin.x - 2); x <= (origin.x + 2); ++x)
        {
                for (int y = (origin.y - 2); y <= (origin.y + 2); ++y)
                {
                        const P p(x, y);

                        const int dist = king_dist(origin, p);

                        if (dist == 0)
                        {
                                continue;
                        }

                        const auto id = map::cells.at(p).rigid->id();

                        if (dist == 1)
                        {
                                // Adjacent to center - should be destroyed
                                REQUIRE(id != FeatureId::wall);
                        }
                        else
                        {
                                // Two steps away - should NOT be destroyed
                                REQUIRE(id == FeatureId::wall);
                        }

                        if (id == FeatureId::wall)
                        {
                                ++nr_walls;
                        }
                        else
                        {
                                ++nr_destroyed;
                        }
                }
        }

        REQUIRE(nr_destroyed == 8);
        REQUIRE(nr_walls == 16);

        test_utils::cleanup_all();
}

TEST_CASE("Explosions at map edge")
{
        // Check that explosions can handle the map edge correctly (e.g. that
        // they do not destroy the edge wall, or go outside the map - possibly
        // causing a crash)

        test_utils::init_all();

        for (int x = 0; x < map::w(); ++x)
        {
                for (int y = 0; y < map::h(); ++y)
                {
                        map::put(new Wall({x, y}));
                }
        }

        // North-west edge
        int x = 1;
        int y = 1;

        map::put(new Floor(P(x, y)));

        REQUIRE(map::cells.at(x + 1, y    ).rigid->id() == FeatureId::wall);
        REQUIRE(map::cells.at(x    , y + 1).rigid->id() == FeatureId::wall);
        REQUIRE(map::cells.at(x - 1, y    ).rigid->id() == FeatureId::wall);
        REQUIRE(map::cells.at(x    , y - 1).rigid->id() == FeatureId::wall);

        for (int i = 0; i < 1000; ++i)
        {
                explosion::run(P(x, y), ExplType::expl);
        }

        REQUIRE(map::cells.at(x + 1, y    ).rigid->id() != FeatureId::wall);
        REQUIRE(map::cells.at(x    , y + 1).rigid->id() != FeatureId::wall);
        REQUIRE(map::cells.at(x - 1, y    ).rigid->id() == FeatureId::wall);
        REQUIRE(map::cells.at(x    , y - 1).rigid->id() == FeatureId::wall);

        // South-east edge
        x = map::w() - 2;
        y = map::h() - 2;

        map::put(new Floor(P(x, y)));

        REQUIRE(map::cells.at(x - 1, y    ).rigid->id() == FeatureId::wall);
        REQUIRE(map::cells.at(x    , y - 1).rigid->id() == FeatureId::wall);
        REQUIRE(map::cells.at(x + 1, y    ).rigid->id() == FeatureId::wall);
        REQUIRE(map::cells.at(x    , y + 1).rigid->id() == FeatureId::wall);

        for (int i = 0; i < 1000; ++i)
        {
                explosion::run(P(x, y), ExplType::expl);
        }

        REQUIRE(map::cells.at(x - 1, y    ).rigid->id() != FeatureId::wall);
        REQUIRE(map::cells.at(x    , y - 1).rigid->id() != FeatureId::wall);
        REQUIRE(map::cells.at(x + 1, y    ).rigid->id() == FeatureId::wall);
        REQUIRE(map::cells.at(x    , y + 1).rigid->id() == FeatureId::wall);
}


TEST_CASE("Explosions damage actors")
{
        test_utils::init_all();

        const P origin(5, 7);

        Actor* a1 = actor_factory::make(ActorId::rat, origin.with_x_offset(1));

        REQUIRE(a1->state == ActorState::alive);

        explosion::run(origin, ExplType::expl);

        REQUIRE(a1->state == ActorState::destroyed);

        test_utils::cleanup_all();
}

TEST_CASE("Explosions damage corpses")
{
        test_utils::init_all();

        const P origin(5, 7);

        for (int i = 0; i < 1000; ++i)
        {
                explosion::run(origin, ExplType::expl);
        }

        const int nr_corpses = 3;

        Actor* corpses[nr_corpses];

        for (int i = 0; i < nr_corpses; ++i)
        {
                corpses[i] =
                        actor_factory::make(
                                ActorId::rat,
                                origin.with_x_offset(1));

                actor::kill(
                        *corpses[i],
                        IsDestroyed::no,
                        AllowGore::no,
                        AllowDropItems::no);
        }

        // Check that living and dead actors on the same cell can be destroyed
        Actor* a1 = actor_factory::make(ActorId::rat, origin.with_x_offset(1));

        for (int i = 0; i < nr_corpses; ++i)
        {
                REQUIRE(corpses[i]->state == ActorState::corpse);
        }

        explosion::run(origin, ExplType::expl);

        for (int i = 0; i < nr_corpses; ++i)
        {
                REQUIRE(corpses[i]->state == ActorState::destroyed);
        }

        REQUIRE(a1->state == ActorState::destroyed);

        test_utils::cleanup_all();
}

TEST_CASE("Fire explosion applies burning to actors")
{
        test_utils::init_all();

        const P origin(5, 7);

        const int nr_corpses = 3;

        Actor* corpses[nr_corpses];

        for (int i = 0; i < nr_corpses; ++i)
        {
                corpses[i] =
                        actor_factory::make(
                                ActorId::rat,
                                origin.with_x_offset(1));

                actor::kill(
                        *corpses[i],
                        IsDestroyed::no,
                        AllowGore::no,
                        AllowDropItems::no);
        }

        Actor* const a1 =
                actor_factory::make(
                        ActorId::rat,
                        origin.with_x_offset(-1));

        Actor* const a2 =
                actor_factory::make(
                        ActorId::rat,
                        origin.with_x_offset(1));

        explosion::run(
                origin,
                ExplType::apply_prop,
                EmitExplSnd::no,
                0,
                ExplExclCenter::no,
                {new PropBurning()});

        for (int i = 0; i < nr_corpses; ++i)
        {
                REQUIRE(corpses[i]->properties.has(PropId::burning));
        }

        REQUIRE(a1->properties.has(PropId::burning));
        REQUIRE(a2->properties.has(PropId::burning));

        test_utils::cleanup_all();
}
