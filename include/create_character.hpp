// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef CREATE_CHARACTER_HPP
#define CREATE_CHARACTER_HPP

#include "state.hpp"
#include "player_bon.hpp"
#include "browser.hpp"

enum class TraitScreenMode
{
        pick_new,
        view_unavail
};

class NewGameState: public State
{
public:
        void on_pushed() override;

        void on_resume() override;

        StateId id() override
        {
                return StateId::new_game;
        }
};

class PickBgState: public State
{
public:
        void on_start() override;

        void update() override;

        void draw() override;

        StateId id() override
        {
                return StateId::pick_background;
        }

private:
        MenuBrowser m_browser {};

        std::vector<Bg> m_bgs {};
};

class PickOccultistState: public State
{
public:
        void on_start() override;

        void update() override;

        void draw() override;

        StateId id() override
        {
                return StateId::pick_background_occultist;
        }

private:
        MenuBrowser m_browser {};

        std::vector<OccultistDomain> m_domains {};
};

class PickTraitState: public State
{
public:
        void on_start() override;

        void update() override;

        void draw() override;

        StateId id() override
        {
                return StateId::pick_trait;
        }

private:
        MenuBrowser m_browser_traits_avail {};
        MenuBrowser m_browser_traits_unavail {};

        std::vector<Trait> m_traits_avail {};
        std::vector<Trait> m_traits_unavail {};

        TraitScreenMode m_screen_mode {TraitScreenMode::pick_new};
};

class EnterNameState: public State
{
public:
        void on_start() override;

        void update() override;

        void draw() override;

        StateId id() override
        {
                return StateId::pick_name;
        }

private:
        std::string m_current_str {};
};

#endif // CREATE_CHARACTER_HPP
