#include "catch.hpp"

#include "misc.hpp"

TEST_CASE("King distance")
{
        REQUIRE(king_dist(P(1, 2), P(2, 3)) == 1);
        REQUIRE(king_dist(P(1, 2), P(2, 4)) == 2);
        REQUIRE(king_dist(P(1, 2), P(1, 2)) == 0);
        REQUIRE(king_dist(P(10, 3), P(1, 4)) == 9);
}
