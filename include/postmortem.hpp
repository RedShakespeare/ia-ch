// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef POSTMORTEM_HPP
#define POSTMORTEM_HPP

#include "state.hpp"
#include "info_screen_state.hpp"
#include "browser.hpp"
#include "global.hpp"

class PostmortemMenu: public State
{
public:
        explicit PostmortemMenu(const IsWin is_win);

        void on_start() override;

        void on_popped() override;

        void draw() override;

        void update() override;

        StateId id() override;

private:
        void make_memorial_file(const std::string& path) const;

        MenuBrowser m_browser;

        IsWin m_is_win;

        std::vector<std::string> m_ascii_graveyard_lines;
};

class PostmortemInfo: public InfoScreenState
{
public:
        PostmortemInfo() :
                InfoScreenState()
                {}

        void draw() override;

        void update() override;

        StateId id() override;

private:
        std::string title() const override
        {
                return "Game summary";
        }

        InfoScreenType type() const override
        {
                return InfoScreenType::scrolling;
        }

        int m_top_idx{0};
};

#endif // POSTMORTEM_HPP
