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
        MenuBrowser browser_ {};

        std::vector<Bg> bgs_ {};
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
        MenuBrowser browser_ {};

        std::vector<OccultistDomain> domains_ {};
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
        MenuBrowser browser_traits_avail_ {};
        MenuBrowser browser_traits_unavail_ {};

        std::vector<Trait> traits_avail_ {};
        std::vector<Trait> traits_unavail_ {};

        TraitScreenMode screen_mode_ {TraitScreenMode::pick_new};
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
        std::string current_str_ {};
};

#endif // CREATE_CHARACTER_HPP
