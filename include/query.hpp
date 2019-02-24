// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef QUERY_HPP
#define QUERY_HPP

#include "colors.hpp"
#include "direction.hpp"


struct InputData;


enum class BinaryAnswer
{
        no,
        yes,
        special
};

enum class AllowCenter
{
        no,
        yes
};


namespace query
{

void init();

void wait_for_key_press();

BinaryAnswer yes_or_no(char key_for_special_event = -1);

Dir dir(const AllowCenter allow_center);

void wait_for_msg_more();

void wait_for_confirm();

InputData letter(const bool accept_enter);

int number(const P& pos,
           const Color color,
           const int min,
           const int max_nr_digits,
           const int default_value,
           const bool cancel_returns_default);

} // query

#endif // QUERY_HPP
