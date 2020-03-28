// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef DMG_RANGE_HPP
#define DMG_RANGE_HPP

#include <string>

#include "random.hpp"

class DmgRange {
public:
        DmgRange() = default;

        DmgRange(const int min, const int max, const int plus = 0) :
                m_min(min),
                m_max(max),
                m_plus(plus)
        {
        }

        Range total_range() const
        {
                return Range(m_min + m_plus, m_max + m_plus);
        }

        int base_min() const
        {
                return m_min;
        }

        int base_max() const
        {
                return m_max;
        }

        void set_base_min(const int v)
        {
                m_min = v;
        }

        void set_base_max(const int v)
        {
                m_max = v;
        }

        int plus() const
        {
                return m_plus;
        }

        void set_plus(const int v)
        {
                m_plus = v;
        }

        DmgRange scaled_pct(const int pct) const
        {
                int new_min = (m_min * pct) / 100;
                int new_max = (m_max * pct) / 100;
                int new_plus = (m_plus * pct) / 100;

                new_min = std::max(new_min, 1);
                new_max = std::max(new_max, 1);

                return {new_min, new_max, new_plus};
        }

        std::string str_plus() const;

private:
        int m_min {0};
        int m_max {0};
        int m_plus {0};
};

#endif // DMG_RANGE_HPP
