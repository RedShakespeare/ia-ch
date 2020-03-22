// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "postmortem.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "actor_player.hpp"
#include "audio.hpp"
#include "browser.hpp"
#include "create_character.hpp"
#include "draw_map.hpp"
#include "game.hpp"
#include "highscore.hpp"
#include "init.hpp"
#include "io.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "paths.hpp"
#include "player_bon.hpp"
#include "query.hpp"
#include "saving.hpp"
#include "terrain.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<ColoredString> s_info_lines;

// -----------------------------------------------------------------------------
// Postmortem menu
// -----------------------------------------------------------------------------
PostmortemMenu::PostmortemMenu(const IsWin is_win) :

        m_is_win(is_win)
{
        m_browser.reset(6);
}

StateId PostmortemMenu::id()
{
        return StateId::postmortem_menu;
}

void PostmortemMenu::on_start()
{
        // Game summary file path
        const std::string game_summary_time_stamp =
                game::start_time().time_str(TimeType::second, false);

        const std::string game_summary_filename =
                map::g_player->name_a() +
                "_" +
                game_summary_time_stamp +
                ".txt";

        const std::string game_summary_file_path =
                paths::user_dir() +
                game_summary_filename;

        // Highscore entry
        auto highscore_entry =
                highscore::make_entry_from_current_game_data(
                        game_summary_file_path,
                        m_is_win);

        highscore::append_entry_to_highscores_file(highscore_entry);

        s_info_lines.clear();

        const Color color_heading = colors::light_white();

        const Color color_info = colors::white();

        const std::string offset = "   ";

        TRACE << "Finding number of killed monsters" << std::endl;

        std::vector<std::string> unique_killed_names;

        int nr_kills_tot_all_mon = 0;

        for (const auto& d : actor::g_data) {
                if ((d.id != actor::Id::player) && (d.nr_kills > 0)) {
                        nr_kills_tot_all_mon += d.nr_kills;

                        if (d.is_unique) {
                                unique_killed_names.push_back(d.name_a);
                        }
                }
        }

        const std::string name = highscore_entry.name;

        std::string bg_title;

        if (highscore_entry.bg == Bg::occultist) {
                const auto domain = highscore_entry.player_occultist_domain;

                bg_title = player_bon::occultist_profession_title(domain);
        } else {
                bg_title = player_bon::bg_title(highscore_entry.bg);
        }

        s_info_lines.emplace_back(name + " (" + bg_title + ")", color_heading);

        const int dlvl = highscore_entry.dlvl;

        if (dlvl == 0) {
                s_info_lines.emplace_back(
                        offset +
                                "Died before entering the dungeon",
                        color_info);
        } else {
                // DLVL is at least 1
                s_info_lines.emplace_back(
                        offset +
                                "Explored to dungeon level " +
                                std::to_string(dlvl),
                        color_info);
        }

        const int turn_count = highscore_entry.turn_count;

        s_info_lines.emplace_back(
                offset +
                        "Spent " +
                        std::to_string(turn_count) +
                        " turns",
                color_info);

        const int ins = highscore_entry.ins;

        s_info_lines.emplace_back(
                offset + "Was " + std::to_string(ins) + "% insane",
                color_info);

        s_info_lines.emplace_back(
                offset +
                        "Killed " +
                        std::to_string(nr_kills_tot_all_mon) +
                        " monsters",
                color_info);

        const int xp = highscore_entry.xp;

        s_info_lines.emplace_back(
                offset +
                        "Gained " +
                        std::to_string(xp) +
                        " experience points",
                color_info);

        const int score = highscore_entry.calculate_score();

        s_info_lines.emplace_back(
                offset + "Gained a score of " + std::to_string(score),
                color_info);

        const std::vector<const InsSympt*> sympts =
                insanity::active_sympts();

        if (!sympts.empty()) {
                for (const InsSympt* const sympt : sympts) {
                        const std::string sympt_descr = sympt->postmortem_msg();

                        if (!sympt_descr.empty()) {
                                s_info_lines.emplace_back(
                                        offset + sympt_descr,
                                        color_info);
                        }
                }
        }

        s_info_lines.emplace_back("", color_info);

        s_info_lines.emplace_back(
                "Traits gained (at character level):",
                color_heading);

        const auto trait_log = player_bon::trait_log();

        if (trait_log.empty()) {
                s_info_lines.emplace_back(offset + "None", color_info);
        } else {
                bool is_double_digit =
                        std::find_if(
                                std::begin(trait_log),
                                std::end(trait_log),
                                [](const auto& e) {
                                        return e.clvl_picked >= 10;
                                }) != std::end(trait_log);

                for (const auto& e : trait_log) {
                        std::string clvl_str = std::to_string(e.clvl_picked);

                        if (is_double_digit) {
                                clvl_str =
                                        text_format::pad_before(
                                                std::to_string(e.clvl_picked),
                                                2);
                        }

                        const std::string title =
                                player_bon::trait_title(e.trait_id);

                        const std::string str =
                                clvl_str +
                                " " +
                                title;

                        s_info_lines.emplace_back(offset + str, color_info);
                }
        }

        s_info_lines.emplace_back("", color_info);

        s_info_lines.emplace_back(
                "Unique monsters killed:",
                color_heading);

        if (unique_killed_names.empty()) {
                s_info_lines.emplace_back(
                        offset + "None",
                        color_info);
        } else {
                for (std::string& monster_name : unique_killed_names) {
                        s_info_lines.emplace_back(
                                offset + "" + monster_name,
                                color_info);
                }
        }

        s_info_lines.emplace_back("", color_info);

        s_info_lines.emplace_back(
                "History of " + map::g_player->name_the(),
                color_heading);

        const std::vector<HistoryEvent>& events =
                game::history();

        int longest_turn_w = 0;

        for (const auto& event : events) {
                const int turn_w = std::to_string(event.turn).size();

                longest_turn_w = std::max(turn_w, longest_turn_w);
        }

        for (const auto& event : events) {
                std::string ev_str = std::to_string(event.turn);

                const int turn_w = ev_str.size();

                ev_str.append(longest_turn_w - turn_w, ' ');

                ev_str += " " + event.msg;

                s_info_lines.emplace_back(
                        offset + ev_str,
                        color_info);
        }

        s_info_lines.emplace_back("", color_info);

        s_info_lines.emplace_back(
                "Last messages:",
                color_heading);

        const auto& msg_history = msg_log::history();

        const int max_nr_messages_to_show = 20;

        int history_start_idx = std::max(
                0,
                (int)msg_history.size() - max_nr_messages_to_show);

        for (size_t history_idx = history_start_idx;
             history_idx < msg_history.size();
             ++history_idx) {
                const auto& msg = msg_history[history_idx];

                s_info_lines.emplace_back(
                        offset + msg.text_with_repeats(),
                        color_info);
        }

        s_info_lines.emplace_back("", color_info);

        // Also dump the lines to a memorial file
        make_memorial_file(game_summary_file_path);

        // If running text mode, load the graveyard ascii art
        if (!config::is_tiles_mode()) {
                m_ascii_graveyard_lines.clear();

                std::string current_line;

                std::ifstream file(paths::data_dir() + "ascii_graveyard");

                if (file.is_open()) {
                        while (getline(file, current_line)) {
                                m_ascii_graveyard_lines.push_back(current_line);
                        }
                } else {
                        TRACE << "Failed to open ascii graveyard file"
                              << std::endl;

                        PANIC;
                }

                file.close();
        }
}

void PostmortemMenu::on_popped()
{
        // The postmortem is the last state which needs to access session data
        init::cleanup_session();
}

void PostmortemMenu::draw()
{
        P menu_pos;

        if (config::is_tiles_mode()) {
                io::draw_box(panels::area(Panel::screen));

                menu_pos =
                        panels::center(Panel::screen)
                                .with_offsets(-9, -4);
        } else {
                // Text mode

                // The last line is the longest (grass)
                const int ascii_graveyard_w =
                        m_ascii_graveyard_lines.back().size();

                const int ascii_graveyard_h =
                        m_ascii_graveyard_lines.size();

                const int screen_center_x = panels::center_x(Panel::screen);

                const int screen_h = panels::h(Panel::screen);

                const int ascii_graveyard_x0 =
                        screen_center_x - (ascii_graveyard_w / 2);

                const int ascii_graveyard_y0 = screen_h - ascii_graveyard_h;

                int y = ascii_graveyard_y0;

                for (const auto& line : m_ascii_graveyard_lines) {
                        io::draw_text(
                                line,
                                Panel::screen,
                                P(ascii_graveyard_x0, y),
                                colors::gray());

                        ++y;
                }

                const std::string name = map::g_player->name_the();

                const int name_x =
                        ascii_graveyard_x0 + 44 - (((int)name.length() - 1) / 2);

                io::draw_text(
                        name,
                        Panel::screen,
                        P(name_x, ascii_graveyard_y0 + 20),
                        colors::gray());

                menu_pos.set(
                        ascii_graveyard_x0 + 56,
                        ascii_graveyard_y0 + 13);
        }

        std::vector<std::string> labels = {
                "New journey",
                "Show game summary",
                "View high scores",
                "View message log",
                "Return to main menu",
                "Quit the game"};

        for (size_t i = 0; i < labels.size(); ++i) {
                const std::string& label = labels[i];

                const Color& color =
                        m_browser.is_at_idx(i)
                        ? colors::menu_highlight()
                        : colors::menu_dark();

                io::draw_text(
                        label,
                        Panel::screen,
                        menu_pos,
                        color);

                ++menu_pos.y;
        }
}

void PostmortemMenu::make_memorial_file(const std::string path) const
{
        // Write memorial file
        std::ofstream file;

        file.open(path.c_str(), std::ios::trunc);

        // Add info lines to file
        for (const ColoredString& line : s_info_lines) {
                file << line.str << std::endl;
        }

        file.close();
}

void PostmortemMenu::update()
{
        const auto input = io::get();

        const MenuAction action =
                m_browser.read(input, MenuInputMode::scrolling);

        switch (action) {
        case MenuAction::selected:

                // Display postmortem info
                switch (m_browser.y()) {
                case 0: {
                        // Exit screen
                        states::pop();

                        init::init_session();

                        states::push(std::make_unique<NewGameState>());

                        audio::fade_out_music();
                } break;

                case 1: {
                        states::push(std::make_unique<PostmortemInfo>());
                } break;

                // Show highscores
                case 2: {
                        states::push(std::make_unique<BrowseHighscore>());
                } break;

                // Display message history
                case 3: {
                        states::push(std::make_unique<MsgHistoryState>());
                } break;

                // Return to main menu
                case 4: {
                        // Exit screen
                        states::pop();
                } break;

                // Quit game
                case 5: {
                        // Bye!
                        states::pop_all();
                } break;
                }
                break;

        default:
                break;
        }
}

// -----------------------------------------------------------------------------
// Postmortem info
// -----------------------------------------------------------------------------
StateId PostmortemInfo::id()
{
        return StateId::postmortem_info;
}

void PostmortemInfo::draw()
{
        io::clear_screen();

        draw_interface();

        const int nr_lines = (int)s_info_lines.size();

        int screen_y = 1;

        for (int i = m_top_idx;
             (i < nr_lines) && ((i - m_top_idx) < max_nr_lines_on_screen());
             ++i) {
                const auto& line = s_info_lines[i];

                io::draw_text(
                        line.str,
                        Panel::screen,
                        P(0, screen_y),
                        line.color);

                ++screen_y;
        }

        io::update_screen();
}

void PostmortemInfo::update()
{
        const int line_jump = 3;

        const int nr_lines = s_info_lines.size();

        const auto input = io::get();

        switch (input.key) {
        case SDLK_DOWN:
        case SDLK_KP_2:
                m_top_idx += line_jump;

                if (nr_lines <= max_nr_lines_on_screen()) {
                        m_top_idx = 0;
                } else {
                        m_top_idx = std::min(
                                nr_lines - max_nr_lines_on_screen(),
                                m_top_idx);
                }
                break;

        case SDLK_UP:
        case SDLK_KP_8:
                m_top_idx = std::max(0, m_top_idx - line_jump);
                break;

        case SDLK_SPACE:
        case SDLK_ESCAPE:
                // Exit screen
                states::pop();
                break;
        }
}
