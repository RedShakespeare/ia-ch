// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "debug.hpp"

#include <cassert>

void assert_impl(
        const bool check,
        const char* check_str,
        const char* file,
        const int line,
        const char* func)
{
        if (!check) {
                std::cerr << std::endl
                          << file << ", "
                          << line << ", "
                          << func << "():"
                          << std::endl
                          << std::endl
                          << "*** ASSERTION FAILED! ***"
                          << std::endl
                          << std::endl
                          << "Check that failed:"
                          << std::endl
                          << "\"" << check_str << "\""
                          << std::endl
                          << std::endl;

                assert(false);
        }
}
