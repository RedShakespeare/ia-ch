// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef POSTMORTEM_HPP
#define POSTMORTEM_HPP

#include "browser.hpp"
#include "global.hpp"
#include "info_screen_state.hpp"
#include "state.hpp"

class PostmortemMenu : public State {
public:
        PostmortemMenu(IsWin is_win);

        void on_start() override;

        void on_popped() override;

        void draw() override;

        void update() override;

        StateId id() override;

private:
        void make_memorial_file(std::string path) const;

        MenuBrowser m_browser;

        IsWin m_is_win;

        std::vector<std::string> m_ascii_graveyard_lines;
};

class PostmortemInfo : public InfoScreenState {
public:
        PostmortemInfo() :
                m_top_idx(0)
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

        int m_top_idx;
};

#endif // POSTMORTEM_HPP
