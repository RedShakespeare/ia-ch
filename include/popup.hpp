// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef POPUP_HPP
#define POPUP_HPP

#include <string>
#include <vector>

#include "audio.hpp"

namespace popup
{

void msg(
        const std::string& msg,
        const std::string& title = "",
        SfxId sfx = SfxId::END,
        int w_change = 0);

int menu(
        const std::string& msg,
        const std::vector<std::string>& choices,
        const std::string& title = "",
        int w_change = 0,
        SfxId sfx = SfxId::END);

} // namespace popup

#endif // POPUP_HPP
