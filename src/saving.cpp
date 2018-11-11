#include "saving.hpp"

#include <fstream>
#include <iostream>

#include "actor_player.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "insanity.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "item_potion.hpp"
#include "item_rod.hpp"
#include "item_scroll.hpp"
#include "map.hpp"
#include "map_templates.hpp"
#include "map_travel.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "paths.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "postmortem.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
#ifndef NDEBUG

// Only used to verify that the put/get methods are not called at the wrong time
enum class SaveLoadState
{
        saving,
        loading,
        stopped
};

static SaveLoadState state;

#endif // NDEBUG

static std::vector<std::string> lines;

static void save_modules()
{
        TRACE_FUNC_BEGIN;

        ASSERT(lines.empty());

        saving::put_str(map::player->name_a());

        game::save();
        scroll_handling::save();
        potion_handling::save();
        rod_handling::save();
        item_data::save();
        map::player->inv.save();
        map::player->save();
        insanity::save();
        player_bon::save();
        map_travel::save();
        map::save();
        actor_data::save();
        game_time::save();
        player_spells::save();
        map_templates::save();

        TRACE_FUNC_END;
}

static void load_modules()
{
        TRACE_FUNC_BEGIN;

        ASSERT(!lines.empty());

        const std::string player_name = saving::get_str();

        ASSERT(!player_name.empty());

        map::player->data->name_a = player_name;

        map::player->data->name_the = player_name;

        game::load();
        scroll_handling::load();
        potion_handling::load();
        rod_handling::load();
        item_data::load();
        map::player->inv.load();
        map::player->load();
        insanity::load();
        player_bon::load();
        map_travel::load();
        map::load();
        actor_data::load();
        game_time::load();
        player_spells::load();
        map_templates::load();

        TRACE_FUNC_END;
}

static void write_file()
{
        std::ofstream file;

        // Current file content is discarded
        file.open(paths::save_path, std::ios::trunc);

        if (file.is_open())
        {
                for (size_t i = 0; i < lines.size(); ++i)
                {
                        file << lines[i];

                        if (i != lines.size() - 1)
                        {
                                file << std::endl;
                        }
                }

                file.close();
        }
}

static void read_file()
{
        std::ifstream file(paths::save_path);

        if (file.is_open())
        {
                std::string current_line = "";

                while (getline(file, current_line))
                {
                        lines.push_back(current_line);
                }

                file.close();
        }
        else // Could not open save file
        {
                ASSERT(false && "Failed to open save file");
        }
}

// -----------------------------------------------------------------------------
// saving
// -----------------------------------------------------------------------------
namespace saving
{

void init()
{
        lines.clear();

#ifndef NDEBUG
        state = SaveLoadState::stopped;
#endif // NDEBUG
}

void save_game()
{
#ifndef NDEBUG
        ASSERT(state == SaveLoadState::stopped);
        ASSERT(lines.empty());

        state = SaveLoadState::saving;
#endif // NDEBUG

        // Tell all modules to append to the save lines (via this modules store
        // functions)
        save_modules();

#ifndef NDEBUG
        state = SaveLoadState::stopped;
#endif // NDEBUG

        // Write the save lines to the save file
        write_file();

        lines.clear();
}

void load_game()
{
#ifndef NDEBUG
        ASSERT(state == SaveLoadState::stopped);
        ASSERT(lines.empty());

        state = SaveLoadState::loading;
#endif // NDEBUG

        // Read the save file to the save lines
        read_file();

        ASSERT(!lines.empty());

        // Tell all modules to set up their state from the save lines (via the
        // read functions of this module)
        load_modules();

#ifndef NDEBUG
        state = SaveLoadState::stopped;
#endif // NDEBUG

        ASSERT(lines.empty());
}

void erase_save()
{
        lines.clear();

        // Write empty save file
        write_file();
}

bool is_save_available()
{
        std::ifstream file(paths::save_path);

        if (file.good())
        {
                const bool is_empty =
                        file.peek() == std::ifstream::traits_type::eof();

                file.close();

                return !is_empty;
        }
        else // Failed to open file
        {
                file.close();

                return false;
        }
}

void put_str(const std::string str)
{
#ifndef NDEBUG
        ASSERT(state == SaveLoadState::saving);
#endif // NDEBUG

        lines.push_back(str);
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
#ifndef NDEBUG
        ASSERT(state == SaveLoadState::loading);
#endif // NDEBUG

        ASSERT(!lines.empty());

        const std::string str = lines.front();

        lines.erase(std::begin(lines));

        return str;
}

int get_int()
{
        return to_int(get_str());
}

bool get_bool()
{
        return get_str() == "T";
}

} // save
