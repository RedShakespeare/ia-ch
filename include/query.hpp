// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef QUERY_HPP
#define QUERY_HPP

#include "colors.hpp"
#include "direction.hpp"
#include "random.hpp"

struct InputData;

enum class BinaryAnswer {
        no,
        yes,
        special
};

enum class AllowCenter {
        no,
        yes
};

enum class AllowSpaceCancel {
        no,
        yes
};

namespace query {

void init();

void cleanup();

void wait_for_key_press();

BinaryAnswer yes_or_no(
        char key_for_special_event = -1,
        AllowSpaceCancel allow_space_cancel = AllowSpaceCancel::yes);

Dir dir(AllowCenter allow_center);

void wait_for_msg_more();

void wait_for_confirm();

InputData letter(bool accept_enter);

int number(
        const P& pos,
        Color color,
        const Range& allowed_range,
        int default_value,
        bool cancel_returns_default);

} // namespace query

#endif // QUERY_HPP
