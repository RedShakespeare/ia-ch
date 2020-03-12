// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
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

void sleep(uint32_t duration);

std::string sdl_pref_dir();

} // namespace sdl_base

#endif // SDL_BASE_HPP
