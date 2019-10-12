// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MAIN_MENU_HPP
#define MAIN_MENU_HPP

#include "state.hpp"
#include "browser.hpp"

class MainMenuState: public State
{
public:
    MainMenuState();

    ~MainMenuState() override;

    void draw() override;

    void update() override;

    void on_start() override;

    void on_resume() override;

    StateId id() override;

private:
    MenuBrowser m_browser;
};

#endif // MAIN_MENU_HPP
