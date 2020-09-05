// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef POSTMORTEM_HPP
#define POSTMORTEM_HPP

#include <string>
#include <vector>

#include "browser.hpp"
#include "colors.hpp"
#include "global.hpp"
#include "info_screen_state.hpp"
#include "state.hpp"

class PostmortemInfo : public InfoScreenState
{
public:
        PostmortemInfo( const std::vector<ColoredString>& lines ) :
                m_lines( lines ) {}

        void draw() override;

        void update() override;

        StateId id() const override;

private:
        std::string title() const override
        {
                return "Game summary";
        }

        InfoScreenType type() const override
        {
                return InfoScreenType::scrolling;
        }

        const std::vector<ColoredString> m_lines {};
        int m_top_idx { 0 };
};

#endif  // POSTMORTEM_HPP
