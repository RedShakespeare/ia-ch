#include "postmortem.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "audio.hpp"
#include "init.hpp"
#include "io.hpp"
#include "actor_player.hpp"
#include "game.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "browser.hpp"
#include "highscore.hpp"
#include "player_bon.hpp"
#include "text_format.hpp"
#include "feature_rigid.hpp"
#include "saving.hpp"
#include "query.hpp"
#include "create_character.hpp"
#include "draw_map.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<ColoredString> info_lines_;

// -----------------------------------------------------------------------------
// Postmortem menu
// -----------------------------------------------------------------------------
PostmortemMenu::PostmortemMenu(const IsWin is_win) :
        State(),
        browser_(),
        is_win_(is_win)
{
        browser_.reset(6);
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
                map::player->name_a() +
                "_" +
                game_summary_time_stamp +
                ".txt";

        const std::string game_summary_file_path =
                "res/data/" +
                game_summary_filename;

        // Highscore entry
        const auto highscore_entry =
                highscore::make_entry_from_current_game_data(
                        game_summary_file_path,
                        is_win_);

        highscore::append_entry_to_highscores_file(highscore_entry);

        info_lines_.clear();

        const Color color_heading = colors::light_white();

        const Color color_info = colors::white();

        const std::string offset = "   ";

        TRACE << "Finding number of killed monsters" << std::endl;

        std::vector<std::string> unique_killed_names;

        int nr_kills_tot_all_mon = 0;

        for (const auto& d : actor_data::data)
        {
                if (d.id != ActorId::player && d.nr_kills > 0)
                {
                        nr_kills_tot_all_mon += d.nr_kills;

                        if (d.is_unique)
                        {
                                unique_killed_names.push_back(d.name_a);
                        }
                }
        }

        const std::string name = highscore_entry.name();

        const std::string bg = player_bon::bg_title(highscore_entry.bg());

        info_lines_.push_back({name + " (" + bg + ")", color_heading});

        const int dlvl = highscore_entry.dlvl();

        if (dlvl == 0)
        {
                info_lines_.push_back(
                        ColoredString(
                                offset +
                                "Died before entering the dungeon",
                                color_info));
        }
        else // DLVL is at least 1
        {
                info_lines_.push_back(
                        ColoredString(
                                offset +
                                "Explored to dungeon level " +
                                std::to_string(dlvl),
                                color_info));

        }

        const int turn_count = highscore_entry.turn_count();

        info_lines_.push_back(
                ColoredString(
                        offset +
                        "Spent " +
                        std::to_string(turn_count) +
                        " turns",
                        color_info));

        const int ins = highscore_entry.ins();

        info_lines_.push_back(
                ColoredString(
                        offset + "Was " + std::to_string(ins) + "% insane",
                        color_info));

        info_lines_.push_back(
                ColoredString(
                        offset +
                        "Killed " +
                        std::to_string(nr_kills_tot_all_mon) +
                        " monsters",
                        color_info));

        const int xp = highscore_entry.xp();

        info_lines_.push_back(
                ColoredString(
                        offset +
                        "Gained " +
                        std::to_string(xp) +
                        " experience points",
                        color_info));

        const int score = highscore_entry.score();

        info_lines_.push_back(
                ColoredString(
                        offset + "Gained a score of " + std::to_string(score),
                        color_info));

        const std::vector<const InsSympt*> sympts =
                insanity::active_sympts();

        if (!sympts.empty())
        {
                for (const InsSympt* const sympt : sympts)
                {
                        const std::string sympt_descr = sympt->postmortem_msg();

                        if (!sympt_descr.empty())
                        {
                                info_lines_.push_back(
                                        ColoredString(
                                                offset + sympt_descr,
                                                color_info));
                        }
                }
        }

        info_lines_.push_back({"", color_info});

        info_lines_.push_back({"Traits gained:", color_heading});

        std::string traits_line =
                player_bon::all_picked_traits_titles_line();

        if (traits_line.empty())
        {
                info_lines_.push_back({offset + "None", color_info});
        }
        else
        {
                const auto abilities_lines =
                        text_format::split(traits_line, 60);

                for (const std::string& str : abilities_lines)
                {
                        info_lines_.push_back({offset + str, color_info});
                }
        }

        info_lines_.push_back({"", color_info});

        info_lines_.push_back(
                ColoredString(
                        "Unique monsters killed:",
                        color_heading));

        if (unique_killed_names.empty())
        {
                info_lines_.push_back(
                        ColoredString(
                                offset + "None",
                                color_info));
        }
        else
        {
                for (std::string& monster_name : unique_killed_names)
                {
                        info_lines_.push_back(
                                ColoredString(
                                        offset + "" + monster_name,
                                        color_info));
                }
        }

        info_lines_.push_back({"", color_info});

        info_lines_.push_back(
                ColoredString(
                        "History of " + map::player->name_the(),
                        color_heading));

        const std::vector<HistoryEvent>& events =
                game::history();

        int longest_turn_w = 0;

        for (const auto& event : events)
        {
                const int turn_w = std::to_string(event.turn).size();

                longest_turn_w = std::max(turn_w, longest_turn_w);
        }

        for (const auto& event : events)
        {
                std::string ev_str = std::to_string(event.turn);

                const int turn_w = ev_str.size();

                ev_str.append(longest_turn_w - turn_w, ' ');

                ev_str += " " + event.msg;

                info_lines_.push_back(
                        ColoredString(
                                offset + ev_str,
                                color_info));
        }

        info_lines_.push_back({"", color_info});

        info_lines_.push_back(
                ColoredString(
                        "Last messages:",
                        color_heading));

        const std::vector< std::vector<Msg> >& msg_history =
                msg_log::history();

        const int max_nr_messages_to_show = 20;

        int history_start_idx = std::max(
                0,
                (int)msg_history.size() - max_nr_messages_to_show);

        for (size_t history_line_idx = history_start_idx;
             history_line_idx < msg_history.size();
             ++history_line_idx)
        {
                std::string row = "";

                const auto& history_line = msg_history[history_line_idx];

                for (const auto& msg : history_line)
                {
                        const std::string msg_str = msg.text_with_repeats();

                        row += msg_str + " ";
                }

                info_lines_.push_back(
                        ColoredString(
                                offset + row,
                                color_info));
        }

        info_lines_.push_back({"", color_info});

        // Also dump the lines to a memorial file
        make_memorial_file(game_summary_file_path);

        // If running text mode, load the graveyard ascii art
        if (!config::is_tiles_mode())
        {
                ascii_graveyard_lines_.clear();

                std::string current_line;

                std::ifstream file("res/ascii_graveyard");

                if (file.is_open())
                {
                        while (getline(file, current_line))
                        {
                                ascii_graveyard_lines_.push_back(current_line);
                        }
                }
                else
                {
                        TRACE << "Failed to open ascii graveyard file"
                              << std::endl;
                        ASSERT(false);
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

        if (config::is_tiles_mode())
        {
                io::draw_box(panels::get_area(Panel::screen));

                menu_pos =
                        panels::get_center(Panel::screen)
                        .with_offsets(-9, -4);
        }
        else // Text mode
        {
                // The last line is the longest (grass)
                const int ascii_graveyard_w =
                        ascii_graveyard_lines_.back().size();

                const int ascii_graveyard_h =
                        ascii_graveyard_lines_.size();

                const int screen_center_x = panels::get_center_x(Panel::screen);

                const int screen_h = panels::get_h(Panel::screen);

                const int ascii_graveyard_x0 =
                        screen_center_x - (ascii_graveyard_w / 2);

                const int ascii_graveyard_y0 = screen_h - ascii_graveyard_h;

                int y = ascii_graveyard_y0;

                for (const auto& line : ascii_graveyard_lines_)
                {
                        io::draw_text(
                                line,
                                Panel::screen,
                                P(ascii_graveyard_x0, y),
                                colors::gray());

                        ++y;
                }

                const std::string name = map::player->name_the();

                const int name_x =
                        ascii_graveyard_x0
                        + 44
                        - ((name.length() - 1) / 2);

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
                "Quit the game"
        };

        for (size_t i = 0; i < labels.size(); ++i)
        {
                const std::string& label = labels[i];

                const Color& color =
                        browser_.is_at_idx(i)
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
        for (const ColoredString& line : info_lines_)
        {
                file << line.str << std::endl;
        }

        file.close();
}

void PostmortemMenu::update()
{
        const auto input = io::get(true);

        const MenuAction action =
                browser_.read(input, MenuInputMode::scrolling);

        switch (action)
        {
        case MenuAction::selected:

                // Display postmortem info
                switch (browser_.y())
                {
                case 0:
                {
                        // Exit screen
                        states::pop();

                        init::init_session();

                        states::push(std::make_unique<NewGameState>());

                        audio::fade_out_music();
                }
                break;

                case 1:
                {
                        states::push(std::make_unique<PostmortemInfo>());
                }
                break;

                // Show highscores
                case 2:
                {
                        states::push(std::make_unique<BrowseHighscore>());
                }
                break;

                // Display message history
                case 3:
                {
                        states::push(std::make_unique<MsgHistoryState>());
                }
                break;

                // Return to main menu
                case 4:
                {
                        // Exit screen
                        states::pop();
                }
                break;

                // Quit game
                case 5:
                {
                        // Bye!
                        states::pop_all();
                }
                break;
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

        const int nr_lines = (int)info_lines_.size();

        int screen_y = 1;

        for (int i = top_idx_;
             (i < nr_lines) && ((i - top_idx_) < max_nr_lines_on_screen());
             ++i)
        {
                const auto& line = info_lines_[i];

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

        const int nr_lines = info_lines_.size();

        const auto input = io::get(false);

        switch (input.key)
        {
        case SDLK_DOWN:
        case '2':
        case 'j':
                top_idx_ += line_jump;

                if (nr_lines <= max_nr_lines_on_screen())
                {
                        top_idx_ = 0;
                }
                else
                {
                        top_idx_ = std::min(
                                nr_lines - max_nr_lines_on_screen(),
                                top_idx_);
                }
                break;

        case SDLK_UP:
        case '8':
        case 'k':
                top_idx_ = std::max(0, top_idx_ - line_jump);
                break;

        case SDLK_SPACE:
        case SDLK_ESCAPE:
                // Exit screen
                states::pop();
                break;
        }
}
