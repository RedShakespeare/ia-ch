// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "pos.hpp"

P::P(const Dir dir) :
        x(0),
        y(0)
{
        set(dir_utils::offset(dir));
}

P& P::operator=(const Dir dir)
{
        set(dir_utils::offset(dir));

        return *this;
}

P& P::operator+=(const Dir dir)
{
        *this += dir_utils::offset(dir);

        return *this;
}

P P::operator+(const Dir dir) const
{
        P result(*this);

        result += dir;

        return result;
}

Dir P::to_dir()
{
        return dir_utils::dir(*this);
}
