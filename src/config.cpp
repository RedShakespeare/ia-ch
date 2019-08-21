// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "config.hpp"

#include "SDL_image.h"
#include <fstream>

#include "audio.hpp"
#include "browser.hpp"
#include "debug.hpp"
#include "terrain_data.hpp"
#include "init.hpp"
#include "io.hpp"
#include "misc.hpp"
#include "panel.hpp"
#include "paths.hpp"
#include "pos.hpp"
#include "query.hpp"
#include "sdl_base.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Config
// -----------------------------------------------------------------------------
namespace config
{

static const std::vector<std::string> font_image_names = {
        "8x12_DOS.png",
        "11x19.png",
        "11x22.png",
        "12x24.png",
        "16x24_v1.png",
        "16x24_v2.png",
        "16x24_v3.png",
        "16x24_DOS.png",
        "16x24_typewriter_v1.png",
        "16x24_typewriter_v2.png",
};

static const int s_opt_y0 = 1;
static const int s_opt_values_x_pos = 44;

static InputMode s_input_mode = InputMode::standard;
static std::string s_font_name = "";
static bool s_is_fullscreen = false;
static bool s_is_native_resolution_fullscreen = false;
static bool s_is_tiles_wall_full_square = false;
static bool s_is_text_mode_wall_full_square = false;
static bool s_is_light_explosive_prompt = false;
static bool s_is_drink_malign_pot_prompt = false;
static bool s_is_ranged_wpn_meleee_prompt = false;
static bool s_is_ranged_wpn_auto_reload = false;
static bool s_is_intro_lvl_skipped = false;
static bool s_is_any_key_confirm_more = false;
static bool s_always_warn_new_mon = false;
static int s_delay_projectile_draw = -1;
static int s_delay_shotgun = -1;
static int s_delay_explosion = -1;
static std::string s_default_player_name = "";
static bool s_is_bot_playing = false;
static bool s_is_audio_enabled = false;
static bool s_is_amb_audio_enabled = false;
static bool s_is_amb_audio_preloaded = false;
static bool s_is_tiles_mode = false;
static int s_screen_px_w = -1;
static int s_screen_px_h = -1;
static int s_gui_cell_px_w = -1;
static int s_gui_cell_px_h = -1;
static int s_map_cell_px_w = -1;
static int s_map_cell_px_h = -1;

static P parse_dims_from_font_name(std::string font_name)
{
        char ch = font_name.front();

        while (ch < '0' || ch > '9')
        {
                font_name.erase(begin(font_name));

                ch = font_name.front();
        }

        std::string w_str = "";

        while (ch != 'x')
        {
                font_name.erase(begin(font_name));

                w_str += ch;

                ch = font_name.front();
        }

        font_name.erase(begin(font_name));

        ch = font_name.front();

        std::string h_str = "";

        while (ch != '_' && ch != '.')
        {
                font_name.erase(begin(font_name));

                h_str += ch;

                ch = font_name.front();
        }

        TRACE << "Parsed font image name, found dims: "
              << w_str << "x" << h_str << std::endl;

        const int w = to_int(w_str);
        const int h = to_int(h_str);

        return P(w, h);
}

static void update_render_dims()
{
        TRACE_FUNC_BEGIN;

        if (s_is_tiles_mode)
        {
                // TODO: Temporary solution
                const P font_dims = parse_dims_from_font_name(s_font_name);

                s_gui_cell_px_w = font_dims.x;
                s_gui_cell_px_h = font_dims.y;
                s_map_cell_px_w = 24;
                s_map_cell_px_h = 24;
        }
        else
        {
                const P font_dims = parse_dims_from_font_name(s_font_name);

                s_gui_cell_px_w = s_map_cell_px_w = font_dims.x;
                s_gui_cell_px_h = s_map_cell_px_h = font_dims.y;
        }

        TRACE << "GUI cell size: "
              << s_gui_cell_px_w
              << ", "
              << s_gui_cell_px_h
              << std::endl;

        TRACE << "Map cell size: "
              << s_map_cell_px_w
              << ", "
              << s_map_cell_px_h
              << std::endl;

        TRACE_FUNC_END;
}

static void set_default_variables()
{
        TRACE_FUNC_BEGIN;

        s_input_mode = InputMode::standard;

        s_is_tiles_mode = true;
        s_font_name = "12x24.png";

        update_render_dims();

        const int default_nr_gui_cells_x = 96;
        const int default_nr_gui_cells_y = 30;

        s_screen_px_w = s_gui_cell_px_w * default_nr_gui_cells_x;
        s_screen_px_h = s_gui_cell_px_h * default_nr_gui_cells_y;

        s_is_audio_enabled = true;
        s_is_amb_audio_enabled = true;
        s_is_amb_audio_preloaded = false;
        s_is_fullscreen = false;
        s_is_native_resolution_fullscreen = false;
        s_is_tiles_wall_full_square = false;
        s_is_text_mode_wall_full_square = true;
        s_is_intro_lvl_skipped = false;
        s_is_any_key_confirm_more = false;
        s_always_warn_new_mon = true;
        s_is_light_explosive_prompt = false;
        s_is_drink_malign_pot_prompt = true;
        s_is_ranged_wpn_meleee_prompt = true;
        s_is_ranged_wpn_auto_reload = false;
        s_delay_projectile_draw = 20;
        s_delay_shotgun = 75;
        s_delay_explosion = 300;
        s_default_player_name = "";

        TRACE_FUNC_END;
}

static void player_sets_option(const MenuBrowser& browser)
{
        switch (browser.y())
        {
        case 0: // Input mode
        {
                const auto input_mode_nr = (int)s_input_mode;

                const auto nr_input_modes = (int)InputMode::END;

                s_input_mode = (InputMode)
                        ((input_mode_nr + 1) % nr_input_modes);
        }
        break;

        case 1: // Audio
        {
                s_is_audio_enabled = !s_is_audio_enabled;

                audio::init();
        }
        break;

        case 2: // Ambient audio
        {
                s_is_amb_audio_enabled = !s_is_amb_audio_enabled;

                audio::init();
        }
        break;

        case 3: // Ambient audio
        {
                s_is_amb_audio_preloaded = !s_is_amb_audio_preloaded;
        }
        break;

        case 4: // Tiles mode
        {
                s_is_tiles_mode = !s_is_tiles_mode;

                // If we do not have a font loaded with the same size as the
                // tiles, use the first font with matching size
                // if (s_is_tiles_mode &&
                //     ((s_map_cell_px_w != tile_px_w) ||
                //      (s_map_cell_px_h != tile_px_h)))
                // {
                //         for (const auto& font_name : font_image_names)
                //         {
                //                 const P font_dims =
                //                         parse_dims_from_font_name(font_name);

                //                 if (font_dims == P(tile_px_w, tile_px_h))
                //                 {
                //                         s_font_name = font_name;

                //                         break;
                //                 }
                //         }
                // }

                update_render_dims();

                sdl_base::init();

                io::init();
        }
        break;

        case 5: // Font
        {
                // Set next font
                for (size_t i = 0; i < font_image_names.size(); ++i)
                {
                        if (s_font_name == font_image_names[i])
                        {
                                s_font_name =
                                        (i == (font_image_names.size() - 1)) ?
                                        font_image_names.front() :
                                        font_image_names[i + 1];
                                break;
                        }
                }

                update_render_dims();

                // In tiles mode, find a font with matching size
                if (s_is_tiles_mode)
                {
                        // Find index of currently used font
                        size_t font_idx = 0;

                        for (; font_idx < font_image_names.size(); ++font_idx)
                        {
                                if (s_font_name == font_image_names[font_idx])
                                {
                                        break;
                                }
                        }

                        // Try fonts until a matching one is found
                        // while ((s_map_cell_px_w != tile_px_w) ||
                        //        (s_map_cell_px_h != tile_px_h))
                        // {
                        //         font_idx =
                        //                 (font_idx + 1) %
                        //                 font_image_names.size();

                        //         s_font_name = font_image_names[font_idx];

                        //         update_render_dims();
                        // }
                }

                sdl_base::init();

                io::init();
        }
        break;

        case 6: // Fullscreen
        {
                set_fullscreen(!s_is_fullscreen);

                io::on_fullscreen_toggled();
        }
        break;

        case 7: // Use native resolution in fullscreen
        {
                s_is_native_resolution_fullscreen =
                        !s_is_native_resolution_fullscreen;

                if (s_is_fullscreen)
                {
                        io::on_fullscreen_toggled();
                }
        }
        break;

        case 8: // Tiles mode wall symbol
        {
                s_is_tiles_wall_full_square = !s_is_tiles_wall_full_square;
        }
        break;

        case 9: // Text mode wall symbol
        {
                s_is_text_mode_wall_full_square =
                        !s_is_text_mode_wall_full_square;
        }
        break;

        case 10: // Skip intro level
        {
                s_is_intro_lvl_skipped = !s_is_intro_lvl_skipped;
        }
        break;

        case 11: // Confirm "more" with any key
        {
                s_is_any_key_confirm_more = !s_is_any_key_confirm_more;
        }
        break;

        case 12: // Always warn when a new monster appears
        {
                s_always_warn_new_mon = !s_always_warn_new_mon;
        }
        break;

        case 13: // Print warning when lighting explovies
        {
                s_is_light_explosive_prompt = !s_is_light_explosive_prompt;
        }
        break;

        case 14: // Print warning when drinking known malign potions
        {
                s_is_drink_malign_pot_prompt = !s_is_drink_malign_pot_prompt;
        }
        break;

        case 15: // Print warning when melee attacking with ranged weapons
        {
                s_is_ranged_wpn_meleee_prompt = !s_is_ranged_wpn_meleee_prompt;
        }
        break;

        case 16: // Ranged weapon auto reload
        {
                s_is_ranged_wpn_auto_reload = !s_is_ranged_wpn_auto_reload;
        }
        break;

        case 17: // Projectile delay
        {
                const P p(s_opt_values_x_pos, s_opt_y0 + browser.y());

                const int nr = query::number(
                        p,
                        colors::menu_highlight(),
                        1,
                        3,
                        s_delay_projectile_draw,
                        true);

                if (nr != -1)
                {
                        s_delay_projectile_draw = nr;
                }
        }
        break;

        case 18: // Shotgun delay
        {
                const P p(s_opt_values_x_pos, s_opt_y0 + browser.y());

                const int nr = query::number(
                        p,
                        colors::menu_highlight(),
                        1,
                        3,
                        s_delay_shotgun,
                        true);

                if (nr != -1)
                {
                        s_delay_shotgun = nr;
                }
        }
        break;

        case 19: // Explosion delay
        {
                const P p(s_opt_values_x_pos, s_opt_y0 + browser.y());

                const int nr = query::number(
                        p,
                        colors::menu_highlight(),
                        1,
                        3,
                        s_delay_explosion,
                        true);

                if (nr != -1)
                {
                        s_delay_explosion = nr;
                }
        }
        break;

        case 20: // Reset to defaults
        {
                set_default_variables();

                update_render_dims();

                sdl_base::init();

                io::init();

                audio::init();
        }
        break;

        default:
        {
                ASSERT(false);
        }
        break;
        }
}

static void read_file(std::vector<std::string>& lines)
{
        std::ifstream file;
        file.open(paths::config_file_path());

        if (file.is_open())
        {
                std::string line;

                while (getline(file, line))
                {
                        lines.push_back(line);
                }

                file.close();
        }
}

static void set_variables_from_lines(std::vector<std::string>& lines)
{
        TRACE_FUNC_BEGIN;

        s_input_mode = (InputMode)to_int(lines.front());
        lines.erase(std::begin(lines));

        s_is_audio_enabled = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_amb_audio_enabled = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_amb_audio_preloaded = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_screen_px_w = to_int(lines.front());
        lines.erase(std::begin(lines));

        s_screen_px_h = to_int(lines.front());
        lines.erase(std::begin(lines));

        s_is_tiles_mode = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_font_name = lines.front();
        lines.erase(std::begin(lines));

        update_render_dims();

        s_is_fullscreen = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_native_resolution_fullscreen = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_tiles_wall_full_square = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_text_mode_wall_full_square = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_intro_lvl_skipped = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_any_key_confirm_more = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_always_warn_new_mon = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_light_explosive_prompt = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_drink_malign_pot_prompt = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_ranged_wpn_meleee_prompt = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_is_ranged_wpn_auto_reload = lines.front() == "1";
        lines.erase(std::begin(lines));

        s_delay_projectile_draw = to_int(lines.front());
        lines.erase(std::begin(lines));

        s_delay_shotgun = to_int(lines.front());
        lines.erase(std::begin(lines));

        s_delay_explosion = to_int(lines.front());
        lines.erase(std::begin(lines));

        s_default_player_name = "";

        if (lines.front() == "1")
        {
                lines.erase(std::begin(lines));

                s_default_player_name = lines.front();
        }

        lines.erase(std::begin(lines));

        ASSERT(lines.empty());

        TRACE_FUNC_END;
}

static void write_lines_to_file(const std::vector<std::string>& lines)
{
        std::ofstream file;
        file.open(paths::config_file_path(), std::ios::trunc);

        for (size_t i = 0; i < lines.size(); ++i)
        {
                file << lines[i];

                if (i != (lines.size() - 1))
                {
                        file << std::endl;
                }
        }

        file.close();
}

static std::vector<std::string> lines_from_variables()
{
        TRACE_FUNC_BEGIN;

        std::vector<std::string> lines;

        lines.push_back(std::to_string((int)s_input_mode));
        lines.push_back(s_is_audio_enabled ? "1" : "0");
        lines.push_back(s_is_amb_audio_enabled ? "1" : "0");
        lines.push_back(s_is_amb_audio_preloaded ? "1" : "0");
        lines.push_back(std::to_string(s_screen_px_w));
        lines.push_back(std::to_string(s_screen_px_h));
        lines.push_back(s_is_tiles_mode ? "1" : "0");
        lines.push_back(s_font_name);
        lines.push_back(s_is_fullscreen ? "1" : "0");
        lines.push_back(s_is_native_resolution_fullscreen ? "1" : "0");
        lines.push_back(s_is_tiles_wall_full_square ? "1" : "0");
        lines.push_back(s_is_text_mode_wall_full_square ? "1" : "0");
        lines.push_back(s_is_intro_lvl_skipped ? "1" : "0");
        lines.push_back(s_is_any_key_confirm_more ? "1" : "0");
        lines.push_back(s_always_warn_new_mon ? "1" : "0");
        lines.push_back(s_is_light_explosive_prompt ? "1" : "0");
        lines.push_back(s_is_drink_malign_pot_prompt ? "1" : "0");
        lines.push_back(s_is_ranged_wpn_meleee_prompt ? "1" : "0");
        lines.push_back(s_is_ranged_wpn_auto_reload ? "1" : "0");
        lines.push_back(std::to_string(s_delay_projectile_draw));
        lines.push_back(std::to_string(s_delay_shotgun));
        lines.push_back(std::to_string(s_delay_explosion));

        if (s_default_player_name.empty())
        {
                lines.push_back("0");
        }
        else // Default player name has been set
        {
                lines.push_back("1");

                lines.push_back(s_default_player_name);
        }

        TRACE_FUNC_END;

        return lines;
}

void init()
{
        s_font_name = "";
        s_is_bot_playing = false;

        set_default_variables();

        std::vector<std::string> lines;

        // Load config file, if it exists
        read_file(lines);

        if (!lines.empty())
        {
                // A config file exists, set values from parsed config lines
                set_variables_from_lines(lines);
        }

        update_render_dims();
}

InputMode input_mode()
{
        return s_input_mode;
}

bool is_tiles_mode()
{
        return s_is_tiles_mode;
}

std::string font_name()
{
        return s_font_name;
}

bool is_fullscreen()
{
        return s_is_fullscreen;
}

bool is_native_resolution_fullscreen()
{
        return s_is_native_resolution_fullscreen;
}

void set_screen_px_w(const int w)
{
        s_screen_px_w = w;

        const auto lines = lines_from_variables();

        write_lines_to_file(lines);
}

void set_screen_px_h(const int h)
{
        s_screen_px_h = h;

        const auto lines = lines_from_variables();

        write_lines_to_file(lines);
}

int screen_px_w()
{
        return s_screen_px_w;
}

int screen_px_h()
{
        return s_screen_px_h;
}

int gui_cell_px_w()
{
        return s_gui_cell_px_w;
}

int gui_cell_px_h()
{
        return s_gui_cell_px_h;
}

int map_cell_px_w()
{
        return s_map_cell_px_w;
}

int map_cell_px_h()
{
        return s_map_cell_px_h;
}

bool is_tiles_wall_full_square()
{
        return s_is_tiles_wall_full_square;
}

bool is_text_mode_wall_full_square()
{
        return s_is_text_mode_wall_full_square;
}

bool is_audio_enabled()
{
        return s_is_audio_enabled;
}

bool is_amb_audio_enabled()
{
        return s_is_amb_audio_enabled;
}

bool is_amb_audio_preloaded()
{
        return s_is_amb_audio_preloaded;
}

bool is_bot_playing()
{
        return s_is_bot_playing;
}

void toggle_bot_playing()
{
        s_is_bot_playing = !s_is_bot_playing;
}

bool is_light_explosive_prompt()
{
        return s_is_light_explosive_prompt;
}

bool is_drink_malign_pot_prompt()
{
        return s_is_drink_malign_pot_prompt;
}

bool is_ranged_wpn_meleee_prompt()
{
        return s_is_ranged_wpn_meleee_prompt;
}

bool is_ranged_wpn_auto_reload()
{
        return s_is_ranged_wpn_auto_reload;
}

bool is_intro_lvl_skipped()
{
        return s_is_intro_lvl_skipped;
}

bool is_any_key_confirm_more()
{
        return s_is_any_key_confirm_more;
}

bool always_warn_new_mon()
{
        return s_always_warn_new_mon;
}

int delay_projectile_draw()
{
        return s_delay_projectile_draw;
}
int delay_shotgun()
{
        return s_delay_shotgun;
}

int delay_explosion()
{
        return s_delay_explosion;
}

void set_default_player_name(const std::string& name)
{
        s_default_player_name = name;

        const auto lines = lines_from_variables();

        write_lines_to_file(lines);
}

std::string default_player_name()
{
        return s_default_player_name;
}

void set_fullscreen(const bool value)
{
        s_is_fullscreen = value;

        const auto lines = lines_from_variables();

        write_lines_to_file(lines);
}

} // config

// -----------------------------------------------------------------------------
// Config state
// -----------------------------------------------------------------------------
ConfigState::ConfigState() :
        State(),
        m_browser(21)
{

}

StateId ConfigState::id()
{
        return StateId::config;
}

void ConfigState::update()
{
        const auto input = io::get();

        const MenuAction action =
                m_browser.read(input, MenuInputMode::scrolling);

        switch (action)
        {
        case MenuAction::esc:
        case MenuAction::space:
        {
                // Since text mode wall symbol may have changed, we need to
                // redefine the terrain data list
                terrain::init();

                states::pop();

                return;
        }
        break;

        case MenuAction::selected:
        {
                config::player_sets_option(m_browser);

                const auto lines = config::lines_from_variables();

                config::write_lines_to_file(lines);

                io::flush_input();
        }
        break;

        default:
                break;
        }
}

void ConfigState::draw()
{
        const int x1 = config::s_opt_values_x_pos;

        std::string str = "";

        io::draw_text(
                "-Options-",
                Panel::screen,
                P(1, 0),
                colors::white());

        std::string font_disp_name = config::s_font_name;

        std::string input_mode_value_str = "";

        switch (config::s_input_mode)
        {
        case InputMode::standard:
                input_mode_value_str = "Default (numpad or arrows)";
                break;

        case InputMode::vi_keys:
                input_mode_value_str = "Vi-keys";
                break;

        case InputMode::END:
                PANIC;
                break;
        }

        const std::vector< std::pair< std::string, std::string > > labels = {
                {
                        "Input mode",
                        input_mode_value_str
                },

                {
                        "Enable audio",
                        config::s_is_audio_enabled
                        ? "Yes"
                        : "No"
                },

                {
                        "Play ambient sounds",
                        config::s_is_amb_audio_enabled
                        ? "Yes"
                        : "No"
                },

                {
                        "Preload ambient sounds at game startup",
                        config::s_is_amb_audio_preloaded
                        ? "Yes"
                        : "No"
                },

                {
                        "Use tile set",
                        config::s_is_tiles_mode
                        ? "Yes"
                        : "No"
                },

                {
                        "Font", font_disp_name
                },

                {
                        "Fullscreen",
                        config::s_is_fullscreen
                        ? "Yes"
                        : "No"
                },

                {
                        "Use native resolution in fullscreen",
                        config::s_is_native_resolution_fullscreen
                        ? "Yes"
                        : "No (Stretch)"
                },

                {
                        "Tiles mode wall symbol",
                        config::s_is_tiles_wall_full_square
                        ? "Full square"
                        : "Pseudo-3D"
                },

                {
                        "Text mode wall symbol",
                        config::s_is_text_mode_wall_full_square
                        ? "Full square"
                        : "Hash sign"
                },

                {
                        "Skip intro level",
                        config::s_is_intro_lvl_skipped
                        ? "Yes"
                        : "No"
                },

                {
                        "Any key confirms \"-More-\" prompts",
                        config::s_is_any_key_confirm_more
                        ? "Yes"
                        : "No"
                },

                {
                        "Always warn when new monster is seen",
                        config::s_always_warn_new_mon
                        ? "Yes"
                        : "No"
                },

                {
                        "Warn when lighting explosives",
                        config::s_is_light_explosive_prompt
                        ? "Yes"
                        : "No"
                },

                {
                        "Warn when drinking malign potions",
                        config::s_is_drink_malign_pot_prompt
                        ? "Yes"
                        : "No"
                },

                {
                        "Ranged weapon melee attack warning",
                        config::s_is_ranged_wpn_meleee_prompt
                        ? "Yes"
                        : "No"
                },

                {
                        "Ranged weapon auto reload",
                        config::s_is_ranged_wpn_auto_reload
                        ? "Yes"
                        : "No"
                },

                {
                        "Projectile delay (ms)",
                        std::to_string(config::s_delay_projectile_draw)
                },

                {
                        "Shotgun delay (ms)",
                        std::to_string(config::s_delay_shotgun)
                },

                {
                        "Explosion delay (ms)",
                        std::to_string(config::s_delay_explosion)
                },

                {
                        "Reset to defaults",
                        ""
                }
        };

        for (size_t i = 0; i < labels.size(); ++i)
        {
                const auto& label = labels[i];

                const std::string& str_l = label.first;
                const std::string& str_r = label.second;

                const auto& color =
                        (m_browser.y() == (int)i)
                        ? colors::menu_highlight()
                        : colors::menu_dark();


                const int y = config::s_opt_y0 + i;

                io::draw_text(
                        str_l,
                        Panel::screen,
                        P(1, y),
                        color);

                if (str_r != "")
                {
                        io::draw_text(
                                ":",
                                Panel::screen,
                                P(x1 - 2, y),
                                color);

                        io::draw_text(
                                str_r,
                                Panel::screen,
                                P(x1, y),
                                color);
                }
        } // for each label

        io::draw_text(
                "[enter] to set option [space/esc] to exit",
                Panel::screen,
                P(1, 23),
                colors::white());
}
