// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "highscore.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>

#include "actor_player.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "highscore.hpp"
#include "init.hpp"
#include "io.hpp"
#include "map.hpp"
#include "misc.hpp"
#include "panel.hpp"
#include "paths.hpp"
#include "popup.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static const int s_top_more_y = 1;

static const int s_entries_y0 = s_top_more_y + 1;


static int get_bottom_more_y()
{
        return panels::h(Panel::screen) - 1;
}

static int get_max_nr_entries_on_screen()
{
        return get_bottom_more_y() - s_entries_y0;
}

static void sort_entries(std::vector<HighscoreEntry>& entries)
{
        std::sort(
                std::begin(entries),
                std::end(entries),
                [](const HighscoreEntry & e1, const HighscoreEntry & e2)
                {
                        return e1.score() > e2.score();
                });
}

static void write_file(std::vector<HighscoreEntry>& entries)
{
        std::ofstream file;

        file.open(paths::highscores_file_path(), std::ios::trunc);

        for (const auto entry : entries)
        {
                const std::string win_str =
                        (entry.is_win() == IsWin::yes) ?
                        "1" :
                        "0";

                file << entry.game_summary_file_path() << std::endl;
                file << win_str << std::endl;
                file << entry.date() << std::endl;
                file << entry.name() << std::endl;
                file << entry.xp() << std::endl;
                file << entry.lvl() << std::endl;
                file << entry.dlvl() << std::endl;
                file << entry.turn_count() << std::endl;
                file << entry.ins() << std::endl;
                file << (int)entry.bg() << std::endl;
                file << (int)entry.occultist_domain() << std::endl;
        }
}

static std::vector<HighscoreEntry> read_highscores_file()
{
        TRACE_FUNC_BEGIN;

        std::vector<HighscoreEntry> entries;

        std::ifstream file;

        file.open(paths::highscores_file_path());

        if (!file.is_open())
        {
                return entries;
        }

        std::string line = "";

        while (getline(file, line))
        {
                const std::string game_summary_file = line;

                getline(file, line);

                IsWin is_win =
                        (line[0] == '1')
                        ? IsWin::yes
                        : IsWin::no;

                getline(file, line);
                const std::string date_and_time = line;

                getline(file, line);
                const std::string name = line;

                getline(file, line);
                const int xp = to_int(line);

                getline(file, line);
                const int lvl = to_int(line);

                getline(file, line);
                const int dlvl = to_int(line);

                getline(file, line);
                const int turn_count = to_int(line);

                getline(file, line);
                const int ins = to_int(line);

                getline(file, line);
                const auto bg = (Bg)to_int(line);

                getline(file, line);
                const auto occultist_domain = (OccultistDomain)to_int(line);

                entries.push_back(
                        HighscoreEntry(
                                game_summary_file,
                                date_and_time,
                                name,
                                xp,
                                lvl,
                                dlvl,
                                turn_count,
                                ins,
                                is_win,
                                bg,
                                occultist_domain));
        }

        file.close();

        TRACE_FUNC_END;

        return entries;
}

// -----------------------------------------------------------------------------
// Highscore entry
// -----------------------------------------------------------------------------
HighscoreEntry::HighscoreEntry(
        std::string game_summary_file_path,
        std::string date,
        std::string player_name,
        int player_xp,
        int player_lvl,
        int player_dlvl,
        int turn_count,
        int player_insanity,
        IsWin is_win,
        Bg player_bg,
        OccultistDomain player_occultist_domain) :

        m_game_summary_file_path(game_summary_file_path),
        m_date(date),
        m_name(player_name),
        m_xp(player_xp),
        m_lvl(player_lvl),
        m_dlvl(player_dlvl),
        m_turn_count(turn_count),
        m_ins(player_insanity),
        m_is_win(is_win),
        m_bg(player_bg),
        m_player_occultist_domain(player_occultist_domain)
{

}

int HighscoreEntry::score() const
{
        const double xp_db = (double)m_xp;
        const double dlvl_db = (double)m_dlvl;
        const double dlvl_last_db = (double)g_dlvl_last;
        const double turns_db = (double)m_turn_count;
        const double ins_db = (double)m_ins;
        const bool win = (m_is_win == IsWin::yes);

        auto calc_turns_factor = [](const double nr_turns_db) {
                return std::max(1.0, 3.0 - (nr_turns_db / 10000.0));
        };

        auto calc_sanity_factor = [](const double nr_ins_db) {
                return 2.0 - (nr_ins_db / 100.0);
        };

        const double xp_factor = 1.0 + xp_db;

        const double dlvl_factor = 1.0 + (dlvl_db / dlvl_last_db);

        const double turns_factor = calc_turns_factor(turns_db);

        const double turns_factor_win = win ? calc_turns_factor(0.0) : 1.0;

        const double sanity_factor = calc_sanity_factor(ins_db);

        const double sanity_factor_win = win ? calc_sanity_factor(0.0) : 1.0;

        return (int)(
                xp_factor *
                dlvl_factor *
                turns_factor *
                turns_factor_win *
                sanity_factor *
                sanity_factor_win);
}

// -----------------------------------------------------------------------------
// Highscore
// -----------------------------------------------------------------------------
namespace highscore
{

void init()
{

}

void cleanup()
{

}

HighscoreEntry make_entry_from_current_game_data(
        const std::string game_summary_file_path,
        const IsWin is_win)
{
        const auto date = current_time().time_str(TimeType::day, true);

        HighscoreEntry entry(
                game_summary_file_path,
                date,
                map::g_player->name_a(),
                game::xp_accumulated(),
                game::clvl(),
                map::g_dlvl,
                game_time::turn_nr(),
                map::g_player->ins(),
                is_win,
                player_bon::bg(),
                player_bon::occultist_domain());

        return entry;
}

void append_entry_to_highscores_file(const HighscoreEntry& entry)
{
        TRACE_FUNC_BEGIN;

        std::vector<HighscoreEntry> entries = entries_sorted();

        entries.push_back(entry);

        sort_entries(entries);

        write_file(entries);

        TRACE_FUNC_END;
}

std::vector<HighscoreEntry> entries_sorted()
{
        auto entries = read_highscores_file();

        if (!entries.empty())
        {
                sort_entries(entries);
        }

        return entries;
}

} // highscore

// -----------------------------------------------------------------------------
// Browse highscore
// -----------------------------------------------------------------------------
StateId BrowseHighscore::id()
{
        return StateId::highscore;
}

void BrowseHighscore::on_start()
{
        m_entries = read_highscores_file();

        sort_entries(m_entries);

        m_browser.reset(m_entries.size(), get_max_nr_entries_on_screen());
}

void BrowseHighscore::draw()
{
        if (m_entries.empty())
        {
                return;
        }

        const Panel panel = Panel::screen;

        const std::string title =
                "Browsing high scores [select] to view game summary";

        io::draw_text_center(
                title,
                panel,
                P(panels::center_x(Panel::screen), 0),
                colors::title(),
                false, // Do not draw background color
                colors::black(),
                true); // Allow pixel-level adjustment

        const Color& label_clr = colors::white();

        const int labels_y = 1;

        const int x_date = 1;
        const int x_name = x_date + 12;
        const int x_bg = x_name + g_player_name_max_len + 1;
        const int x_lvl = x_bg + 13;
        const int x_dlvl = x_lvl + 7;
        const int x_turns = x_dlvl + 7;
        const int x_ins = x_turns + 7;
        const int x_win = x_ins + 6;
        const int x_score = x_win + 5;

        const std::vector< std::pair<std::string, int> > labels
        {
                {"Level", x_lvl},
                {"Depth", x_dlvl},
                {"Turns", x_turns},
                {"Ins", x_ins},
                {"Win", x_win},
                {"Score", x_score}
        };

        for (const auto& label : labels)
        {
                io::draw_text(
                        label.first,
                        panel,
                        P(label.second, labels_y),
                        label_clr);
        }

        const int browser_y = m_browser.y();

        int y = s_entries_y0;

        const Range idx_range_shown = m_browser.range_shown();

        for (int i = idx_range_shown.min; i <= idx_range_shown.max; ++i)
        {
                const auto& entry = m_entries[i];

                const std::string date = entry.date();
                const std::string name = entry.name();

                std::string bg_title;

                if (entry.bg() == Bg::occultist)
                {
                        bg_title = player_bon::occultist_profession_title(
                                entry.occultist_domain());
                }
                else
                {
                        bg_title = player_bon::bg_title(entry.bg());
                }

                const std::string lvl = std::to_string(entry.lvl());
                const std::string dlvl = std::to_string(entry.dlvl());
                const std::string turns = std::to_string(entry.turn_count());
                const std::string ins = std::to_string(entry.ins());

                const std::string win =
                        (entry.is_win() == IsWin::yes)
                        ? "Yes"
                        : "No";

                const std::string score = std::to_string(entry.score());

                const bool is_idx_marked = browser_y == i;

                const Color& color =
                        is_idx_marked
                        ? colors::menu_highlight()
                        : colors::menu_dark();

                io::draw_text(date, panel, P(x_date, y), color);
                io::draw_text(name, panel, P(x_name, y), color);
                io::draw_text(bg_title, panel, P(x_bg, y), color);
                io::draw_text(lvl, panel, P(x_lvl, y), color);
                io::draw_text(dlvl, panel, P(x_dlvl, y), color);
                io::draw_text(turns, panel, P(x_turns, y), color);
                io::draw_text(ins + "%", panel, P(x_ins, y), color);
                io::draw_text(win, panel, P(x_win, y), color);
                io::draw_text(score, panel, P(x_score, y), color);

                ++y;
        }

        // Draw "more" labels
        if (!m_browser.is_on_top_page())
        {
                io::draw_text("(More - Page Up)",
                              Panel::screen,
                              P(0, s_top_more_y),
                              colors::light_white());
        }

        if (!m_browser.is_on_btm_page())
        {
                io::draw_text("(More - Page Down)",
                              Panel::screen,
                              P(0, get_bottom_more_y()),
                              colors::light_white());
        }
}

void BrowseHighscore::update()
{
        if (m_entries.empty())
        {
                popup::msg("No high score entries found.");

                // Exit screen
                states::pop();

                return;
        }

        const auto input = io::get();

        const MenuAction action =
                m_browser.read(
                        input,
                        MenuInputMode::scrolling);

        switch (action)
        {
        case MenuAction::selected:
        {
                const int browser_y = m_browser.y();

                ASSERT(browser_y < (int)m_entries.size());

                const auto& entry_marked = m_entries[browser_y];

                const std::string file_path =
                        entry_marked.game_summary_file_path();

                states::push(
                        std::make_unique<BrowseHighscoreEntry>(file_path));
        }
        break;

        case MenuAction::space:
        case MenuAction::esc:
        {
                // Exit screen
                states::pop();
        }
        break;

        default:
                break;
        }
}

// -----------------------------------------------------------------------------
// Browse highscore entry game summary file
// -----------------------------------------------------------------------------
BrowseHighscoreEntry::BrowseHighscoreEntry(const std::string& file_path) :
        InfoScreenState(),
        m_file_path(file_path),
        m_lines(),
        m_top_idx(0) {}

StateId BrowseHighscoreEntry::id()
{
        return StateId::browse_highscore_entry;
}

void BrowseHighscoreEntry::on_start()
{
        read_file();
}

void BrowseHighscoreEntry::draw()
{
        draw_interface();

        const int nr_lines_tot = m_lines.size();

        int btm_nr =
                std::min(
                        m_top_idx + max_nr_lines_on_screen() - 1,
                        nr_lines_tot - 1);

        int screen_y = 1;

        for (int i = m_top_idx; i <= btm_nr; ++i)
        {
                io::draw_text(
                        m_lines[i],
                        Panel::screen,
                        P(0, screen_y),
                        colors::text());

                ++screen_y;
        }
}

void BrowseHighscoreEntry::update()
{
        const int line_jump = 3;

        const int nr_lines_tot = m_lines.size();

        const auto input = io::get();

        switch (input.key)
        {
        case SDLK_KP_2:
        case SDLK_DOWN:
                m_top_idx += line_jump;

                if (nr_lines_tot <= max_nr_lines_on_screen())
                {
                        m_top_idx = 0;
                }
                else
                {
                        m_top_idx = std::min(
                                nr_lines_tot - max_nr_lines_on_screen(),
                                m_top_idx);
                }
                break;

        case SDLK_KP_8:
        case SDLK_UP:
                m_top_idx = std::max(0, m_top_idx - line_jump);
                break;

        case SDLK_SPACE:
        case SDLK_ESCAPE:
                // Exit screen
                states::pop();
                break;

        default:
                break;
        }
}

void BrowseHighscoreEntry::read_file()
{
        m_lines.clear();

        std::ifstream file(m_file_path);

        if (!file.is_open())
        {
                popup::msg(
                        "Path: \"" + m_file_path + "\"",
                        "Game summary file could not be opened",
                        SfxId::END,
                        20);

                states::pop();

                return;
        }

        std::string current_line;

        std::vector<std::string> formatted;

        while (getline(file, current_line))
        {
                m_lines.push_back(current_line);
        }

        file.close();
}
