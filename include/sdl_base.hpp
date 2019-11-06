// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SDL_BASE_HPP
#define SDL_BASE_HPP

#include <string>

#include "SDL.h"

// TODO: This can probably be merged with the io namespace

namespace sdl_base
{

void init();

void cleanup();

void sleep(const uint32_t duration);

std::string sdl_pref_dir();

}

#endif // SDL_BASE_HPP
