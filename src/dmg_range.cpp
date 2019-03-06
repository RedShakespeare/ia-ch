// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "dmg_range.hpp"

std::string DmgRange::str_plus() const
{
        if (m_plus == 0)
        {
                return "";
        }
        else if (m_plus > 0)
        {
                return "+" + std::to_string(m_plus);
        }
        else
        {
                return "-" + std::to_string(m_plus);
        }

}
