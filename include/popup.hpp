// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef POPUP_HPP
#define POPUP_HPP

#include <vector>
#include <string>

#include "audio.hpp"

namespace popup
{

void msg(const std::string& msg,
         const std::string& title = "",
         const SfxId sfx = SfxId::END,
         const int w_change = 0);

int menu(const std::string& msg,
         const std::vector<std::string>& choices,
         const std::string& title = "",
         const SfxId sfx = SfxId::END);

} // popup

#endif // POPUP_HPP
