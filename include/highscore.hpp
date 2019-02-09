// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef HIGHSCORE_HPP
#define HIGHSCORE_HPP

#include <vector>
#include <string>

#include "state.hpp"
#include "info_screen_state.hpp"
#include "player_bon.hpp"
#include "global.hpp"
#include "browser.hpp"

class HighscoreEntry
{
public:
        HighscoreEntry(
                std::string game_summary_file_path,
                std::string entry_date,
                std::string player_name,
                int player_xp,
                int player_lvl,
                int player_dlvl,
                int turn_count,
                int player_insanity,
                IsWin is_win,
                Bg player_bg,
                OccultistDomain player_occultist_domain);

        ~HighscoreEntry() {}

        int score() const;

        std::string game_summary_file_path() const
        {
                return m_game_summary_file_path;
        }

        std::string date() const
        {
                return m_date;
        }

        std::string name() const
        {
                return m_name;
        }

        int xp() const
        {
                return m_xp;
        }

        int lvl() const
        {
                return m_lvl;
        }

        int dlvl() const
        {
                return m_dlvl;
        }

        int turn_count() const
        {
                return m_turn_count;
        }

        int ins() const
        {
                return m_ins;
        }

        IsWin is_win() const
        {
                return m_is_win;
        }

        Bg bg() const
        {
                return m_bg;
        }

        OccultistDomain occultist_domain() const
        {
                return m_player_occultist_domain;
        }

private:
        std::string m_game_summary_file_path;
        std::string m_date;
        std::string m_name;
        int m_xp;
        int m_lvl;
        int m_dlvl;
        int m_turn_count;
        int m_ins;
        IsWin m_is_win;
        Bg m_bg;
        OccultistDomain m_player_occultist_domain;
};

namespace highscore
{

void init();
void cleanup();

// NOTE: All this does is construct a HighscoreEntry object, populated with
// highscore info based on the current game - it has no side effects
HighscoreEntry make_entry_from_current_game_data(
        const std::string game_summary_file_path,
        const IsWin is_win);

void append_entry_to_highscores_file(const HighscoreEntry& entry);

std::vector<HighscoreEntry> entries_sorted();

} // highscore

class BrowseHighscore: public State
{
public:
        BrowseHighscore() :
                State(),
                m_entries(),
                m_browser() {}

        void on_start() override;

        void draw() override;

        void update() override;

        bool draw_overlayed() const override
        {
                // If there are no entries, we draw an overlayed popup
                return m_entries.empty();
        }

        StateId id() override;

private:
        std::vector<HighscoreEntry> m_entries;

        MenuBrowser m_browser;
};

class BrowseHighscoreEntry: public InfoScreenState
{
public:
        BrowseHighscoreEntry(const std::string& file_path);

        void on_start() override;

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

        void read_file();

        const std::string m_file_path;

        std::vector<std::string> m_lines;

        int m_top_idx;
};

#endif // HIGHSCORE_HPP
