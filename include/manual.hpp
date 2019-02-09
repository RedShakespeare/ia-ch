// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MANUAL_HPP
#define MANUAL_HPP

#include <vector>
#include <string>

#include "state.hpp"
#include "info_screen_state.hpp"
#include "global.hpp"
#include "browser.hpp"

struct ManualPage
{
        std::string title = "";

        std::vector<std::string> lines = {};
};

class BrowseManual: public State
{
public:
        BrowseManual() :
                State(),
                m_browser(),
                m_pages(),
                m_top_idx(0) {}

        void on_start() override;

        void draw() override;

        void on_window_resized() override;

        void update() override;

        StateId id() override;

private:
        MenuBrowser m_browser;

        std::vector<std::string> m_raw_lines;

        std::vector<ManualPage> m_pages;

        int m_top_idx;
};

class BrowseManualPage: public InfoScreenState
{
public:
        BrowseManualPage(const ManualPage& page) :
                InfoScreenState(),
                m_page(page),
                m_top_idx(0) {}

        void draw() override;

        void update() override;

        StateId id() override;

private:
        std::string title() const override;

        InfoScreenType type() const override
        {
                return InfoScreenType::scrolling;
        }

        const ManualPage& m_page;

        int m_top_idx;
};

#endif // MANUAL_HPP
