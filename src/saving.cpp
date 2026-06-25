// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "saving.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <vector>

#include "actor.hpp"
#include "actor_data.hpp"
#include "debug.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "hints.hpp"
#include "insanity.hpp"
#include "inventory.hpp"
#include "item_curse.hpp"
#include "item_data.hpp"
#include "item_potion.hpp"
#include "item_rod.hpp"
#include "item_scroll.hpp"
#include "map.hpp"
#include "map_templates.hpp"
#include "map_travel.hpp"
#include "paths.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "smell.hpp"
#include "terrain_pylon.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
// Only used to verify that the put/get methods are not called at the wrong time
enum class SaveLoadState
{
    saving,
    loading,
    stopped
};

static SaveLoadState s_state;

static std::vector<std::string> s_lines;

static std::string s_last_load_error;

class LoadError : public std::runtime_error
{
public:
    explicit LoadError(const std::string& msg) :
        std::runtime_error(msg)
    {
    }
};

[[noreturn]] static void fail_load(const std::string& msg)
{
    if (s_last_load_error.empty()) {
        s_last_load_error = msg;
    }

    TRACE_ERROR_RELEASE << msg << "\n";

    throw LoadError(msg);
}

static bool parse_int(const std::string& str, int& result)
{
    const char* const begin = str.data();
    const char* const end = begin + str.size();

    const auto parse_result = std::from_chars(begin, end, result);

    return (
        parse_result.ec == std::errc() &&
        parse_result.ptr == end);
}

static void write_save_insanity_file()
{
    std::ofstream file(paths::save_insanity_file_path(), std::ios::trunc);

    if (file.is_open()) {
        file << map::g_player->insanity();
    }
}

static void save_modules()
{
    TRACE_FUNC_BEGIN;

    ASSERT(s_lines.empty());

    saving::put_str(actor::name_a(*map::g_player));

    game::save();
    scroll::save();
    potion::save();
    rod::save();
    item::save();
    item_curse::save();
    terrain::pylon::save();
    map::g_player->m_inv.save();
    map::g_player->save();
    insanity::save();
    player_bon::save();
    map_travel::save();
    map::save();
    actor::save();
    game_time::save();
    player_spells::save();
    map_templates::save();
    hints::save();
    smell::save();

    TRACE_FUNC_END;
}

static void load_modules()
{
    TRACE_FUNC_BEGIN;

    const std::string player_name = saving::get_str();

    if (player_name.empty()) {
        fail_load("Failed to load save file: player name is empty");
    }

    map::g_player->m_data->name_a = player_name;

    map::g_player->m_data->name_the = player_name;

    game::load();
    scroll::load();
    potion::load();
    rod::load();
    item::load();
    item_curse::load();
    terrain::pylon::load();
    map::g_player->m_inv.load();
    map::g_player->load();
    insanity::load();
    player_bon::load();
    map_travel::load();
    map::load();
    actor::load();
    game_time::load();
    player_spells::load();
    map_templates::load();
    hints::load();
    smell::load();

    TRACE_FUNC_END;
}

static void validate_loaded_state()
{
    if (!map::g_player) {
        fail_load("Failed to load save file: missing player");
    }

    if (!map::is_pos_inside_map(map::g_player->m_pos)) {
        fail_load("Failed to load save file: player position is outside the map");
    }
}

static void write_file()
{
    std::ofstream file;

    // Current file content is discarded
    file.open(paths::save_file_path(), std::ios::trunc);

    if (file.is_open()) {
        for (size_t i = 0; i < s_lines.size(); ++i) {
            file << s_lines[i];

            if (i != s_lines.size() - 1) {
                file << "\n";
            }
        }

        file.close();
    }
}

static bool read_file()
{
    std::ifstream file(paths::save_file_path());

    if (file.is_open()) {
        std::string current_line;

        while (getline(file, current_line)) {
            s_lines.push_back(current_line);
        }

        file.close();

        return true;
    }
    else {
        TRACE_ERROR_RELEASE
            << "Failed to open save file: "
            << paths::save_file_path()
            << "\n";

        return false;
    }
}

// -----------------------------------------------------------------------------
// saving
// -----------------------------------------------------------------------------
namespace saving
{
void init()
{
    s_lines.clear();
    s_last_load_error.clear();

    s_state = SaveLoadState::stopped;
}

void save_game()
{
    ASSERT(s_state == SaveLoadState::stopped);
    ASSERT(s_lines.empty());

    s_state = SaveLoadState::saving;

    // Tell all modules to append to the save lines (via this modules store
    // functions)
    save_modules();

    s_state = SaveLoadState::stopped;

    // Write the save lines to the save file
    write_file();
    write_save_insanity_file();

    s_lines.clear();
}

bool load_game()
{
    ASSERT(s_state == SaveLoadState::stopped);
    ASSERT(s_lines.empty());

    s_state = SaveLoadState::loading;
    s_last_load_error.clear();

    try {
        // Read the save file to the save lines
        if (!read_file()) {
            fail_load("Failed to load save file: could not open file");
        }

        if (s_lines.empty()) {
            fail_load("Failed to load save file: file is empty");
        }

        // Tell all modules to set up their state from the save lines (via the
        // read functions of this module)
        load_modules();
        validate_loaded_state();

        s_state = SaveLoadState::stopped;

        if (!s_lines.empty()) {
            TRACE_ERROR_RELEASE
                << "Save file contained "
                << s_lines.size()
                << " unread trailing line(s)"
                << "\n";
        }

        s_lines.clear();

        return true;
    } catch (const LoadError&) {
        s_state = SaveLoadState::stopped;
        s_lines.clear();

        if (s_last_load_error.empty()) {
            s_last_load_error = "Failed to load save file";
        }

        return false;
    } catch (const std::exception& e) {
        s_state = SaveLoadState::stopped;
        s_lines.clear();

        s_last_load_error =
            std::string("Failed to load save file: ") + e.what();

        TRACE_ERROR_RELEASE << s_last_load_error << "\n";

        return false;
    }
}

const std::string& last_load_error()
{
    return s_last_load_error;
}

void erase_save()
{
    s_lines.clear();

    // Write empty save file
    write_file();

    std::error_code err;
    std::filesystem::remove(paths::save_insanity_file_path(), err);
}

bool is_save_available()
{
    std::ifstream file(paths::save_file_path());

    if (file.good()) {
        const bool is_empty =
            file.peek() == std::ifstream::traits_type::eof();

        file.close();

        return !is_empty;
    }
    else {
        // Failed to open file
        file.close();

        return false;
    }
}

int save_file_insanity_for_menu()
{
    std::ifstream file(paths::save_insanity_file_path());

    if (!file.good()) {
        return 0;
    }

    int insanity = 0;
    file >> insanity;

    return std::clamp(insanity, 0, 100);
}

bool is_loading()
{
    return s_state == SaveLoadState::loading;
}

void put_str(const std::string& str)
{
    ASSERT(s_state == SaveLoadState::saving);

    s_lines.push_back(str);
}

void put_int(const int v)
{
    put_str(std::to_string(v));
}

void put_bool(const bool v)
{
    const std::string str = v ? "T" : "F";

    put_str(str);
}

std::string get_str()
{
    ASSERT(s_state == SaveLoadState::loading);

    if (s_state != SaveLoadState::loading) {
        fail_load("Failed to load save file: attempted to read outside load state");
    }

    if (s_lines.empty()) {
        fail_load("Failed to load save file: unexpected end of file");
    }

    auto str = s_lines.front();

    s_lines.erase(std::begin(s_lines));

    return str;
}

int get_int()
{
    const std::string str = get_str();

    int result = 0;

    if (!parse_int(str, result)) {
        fail_load("Failed to load save file: invalid integer value '" + str + "'");
    }

    return result;
}

bool get_bool()
{
    const std::string str = get_str();

    if (str == "T") {
        return true;
    }

    if (str == "F") {
        return false;
    }

    fail_load("Failed to load save file: invalid boolean value '" + str + "'");
}

}  // namespace saving
