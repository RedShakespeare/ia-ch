// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "catch.hpp"

#include "direction.hpp"
#include "pos.hpp"

TEST_CASE("Compass direction name")
{
        const P p(20, 20);

        REQUIRE(dir_utils::compass_dir_name(p, p.with_x_offset(1))
              == "E");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(1, 1))
              == "SE");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_y_offset(1))
              == "S");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(-1, 1))
              == "SW");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_x_offset(-1))
              == "W");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(-1, - 1))
              == "NW");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_y_offset(-1))
              == "N");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(1, -1))
              == "NE");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(3, 1))
              == "E");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(2, 3))
              == "SE");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(1, 3))
              == "S");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(-3, 2))
              == "SW");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(-3, 1))
              == "W");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(-3, -2))
              == "NW");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(1, -3))
              == "N");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(3, -2))
              == "NE");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_x_offset(10000))
              == "E");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(10000, 10000))
              == "SE");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_y_offset(10000))
              == "S");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(-10000, 10000))
              == "SW");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_x_offset(-10000))
              == "W");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(-10000, -10000))
              == "NW");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_y_offset(-10000))
              == "N");

        REQUIRE(dir_utils::compass_dir_name(p, p.with_offsets(10000, -10000))
              == "NE");
}
