// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <algorithm>
#include <string>
#include <vector>

#include "SDL_keycode.h"
#include "SDL_stdinc.h"
#include "audio.hpp"
#include "audio_data.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "direction.hpp"
#include "gfx.hpp"
#include "io.hpp"
#include "io_internal.hpp"
#include "panel.hpp"
#include "pos.hpp"
#include "rect.hpp"
#include "test_utils.hpp"
#include "utf8.hpp"

namespace actor
{
class Actor;
}  // namespace actor

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace
{
bool s_is_wide_cjk_text_stub_enabled = false;
int s_cjk_advance_override_px = 0;
std::string s_sdl_base_dir_stub = "./";
std::vector<test_utils::CapturedTextDraw> s_captured_text_draws;

bool is_cjk_codepoint_for_test(const uint32_t codepoint)
{
    return codepoint >= 0x4e00U && codepoint <= 0x9fffU;
}

int stub_glyph_advance_px(const uint32_t codepoint)
{
    const int cell_px_w = std::max(1, config::gui_cell_px_w());

    if (s_cjk_advance_override_px > 0 && is_cjk_codepoint_for_test(codepoint)) {
        return s_cjk_advance_override_px;
    }

    if (s_is_wide_cjk_text_stub_enabled && is_cjk_codepoint_for_test(codepoint)) {
        return cell_px_w * 2;
    }

    return cell_px_w;
}

std::string text_to_string(Text text)
{
    std::string result;

    for (const auto& action : text.actions()) {
        if (action.id == TextActionId::write_str) {
            result += action.str;
        }
    }

    return result;
}
}  // namespace

namespace io
{
bool g_allow_render = true;

bool is_right_held = false;
bool is_left_held = false;
bool is_up_held = false;
bool is_down_held = false;
bool is_up_right_held = false;
bool is_up_left_held = false;
bool is_down_right_held = false;
bool is_down_left_held = false;

void init_sdl() {}

void init_other()
{
    panels::init({100, 100});
}

void reload_logo() {}

void cleanup_sdl() {}

void cleanup_other() {}

void init_sdl_audio() {}

void cleanup_sdl_audio() {}

void update_screen() {}

void clear_screen() {}

P get_native_resolution()
{
    return {};
}

float get_dpi_scale_factor()
{
    return 1.0f;
}

void on_user_toggle_fullscreen() {}
void on_user_toggle_scaling() {}

R gui_to_px_rect(const R&)
{
    return {};
}

int gui_to_px_coords_x(const int)
{
    return 0;
}

int gui_to_px_coords_y(const int)
{
    return 0;
}

int map_to_px_coords_x(const int)
{
    return 0;
}

int map_to_px_coords_y(const int)
{
    return 0;
}

P gui_to_px_coords(const P&)
{
    return {};
}

P gui_to_px_coords(const int, const int)
{
    return {};
}

P map_to_px_coords(const P&)
{
    return {};
}

P map_to_px_coords(const int, const int)
{
    return {};
}

P px_to_gui_coords(const P&)
{
    return {};
}

P px_to_map_coords(const P&)
{
    return {};
}

P gui_to_map_coords(const P&)
{
    return {};
}

P gui_to_px_coords(const Panel, const P&)
{
    return {};
}

P map_to_px_coords(const Panel, const P&)
{
    return {};
}

void draw_map_obj(const MapDrawObj&) {}

void draw_tile(const TileDrawObj&) {}

void draw_character(const CharacterDrawObj&) {}

void draw_text(
    Text text,
    Panel panel,
    P pos,
    Color,
    const DrawBg,
    const Color&)
{
    s_captured_text_draws.push_back({text_to_string(text), panel, pos});
}

void draw_text_plain(
    const std::string& str,
    Panel panel,
    P pos,
    const Color&,
    const DrawBg,
    const Color&)
{
    s_captured_text_draws.push_back({str, panel, pos});
}

void draw_text_plain_at_px(
    const std::string& str,
    const P pos,
    const Color&,
    const DrawBg,
    const Color&)
{
    // The message log renders at pixel positions relative to the log panel.
    s_captured_text_draws.push_back({str, Panel::log, pos});
}

void draw_text_plain_center(
    const std::string&,
    const Panel,
    P,
    const Color&,
    const DrawBg,
    const Color&,
    const bool) {}

void draw_text_plain_right(
    const std::string&,
    const Panel,
    P,
    const Color&,
    const DrawBg,
    const Color&) {}

void draw_text_center(
    const std::string&,
    const Panel,
    P,
    const Color&,
    const DrawBg,
    const Color&,
    const bool) {}

void draw_text_right(
    const std::string&,
    const Panel,
    P,
    const Color&,
    const DrawBg,
    const Color&) {}

int text_advance_px(const std::string& str)
{
    int result = 0;

    for (size_t pos = 0; pos < str.size();) {
        const size_t cp_size = utf8::codepoint_size(str, pos);
        if ((cp_size == 0) || ((pos + cp_size) > str.size())) {
            break;
        }

        result += stub_glyph_advance_px(utf8::codepoint_at(str, pos).value_or('?'));
        pos += cp_size;
    }

    return result;
}

void clear_text_width_cache() {}

void clear_texture_color_mod_cache() {}

void set_texture_color_mod_if_needed(
    SDL_Texture*,
    const Color&) {}

void cover_cell(const Panel, const P&) {}

void cover_panel(
    const Panel,
    const Color&) {}

void cover_area(
    const Panel,
    const R&,
    const Color&) {}

void cover_area(
    const Panel,
    const P&,
    const P&,
    const Color&) {}

void draw_rectangle(R, const Color&) {}

void draw_rectangle_filled(R, const Color&, const uint8_t) {}

void draw_rectangle_filled_mod_blending(R, const Color&, uint8_t) {}

void TileDrawObj::draw() const {}

void CharacterDrawObj::draw() const {}

void MapDrawObj::draw() const {}

void draw_logo(Color) {}

void clear_input() {}

InputData read_input()
{
    InputData d = {};

    d.key = SDLK_SPACE;

    return d;
}

Dir controller_support_mode_dir_held()
{
    return Dir::END;
}

int graphics_cycle_nr(const GraphicsCycle)
{
    return 0;
}

void flash_at(const P&, const Color&, const int)
{
}

void flash_at_actor(const actor::Actor&, const Color&, const int)
{
}

void draw_flash_animations() {}

void clear_all_flash_animations() {}

std::string sdl_pref_dir()
{
    return "./";
}

std::string sdl_base_dir()
{
    return s_sdl_base_dir_stub;
}

void sleep(const Uint32) {}

}  // namespace io

namespace test_utils
{
void enable_wide_cjk_text_stub(const bool is_enabled)
{
    s_is_wide_cjk_text_stub_enabled = is_enabled;
}

void set_cjk_advance_override_px(const int px)
{
    s_cjk_advance_override_px = px;
}

void set_sdl_base_dir_stub(const std::string& path)
{
    s_sdl_base_dir_stub = path;
}

void reset_sdl_path_stubs()
{
    s_sdl_base_dir_stub = "./";
}

void clear_captured_text_draws()
{
    s_captured_text_draws.clear();
}

const std::vector<CapturedTextDraw>& captured_text_draws()
{
    return s_captured_text_draws;
}
}  // namespace test_utils

// -----------------------------------------------------------------------------
// audio
// -----------------------------------------------------------------------------
namespace audio
{
void init() {}

void cleanup() {}

void play(const SfxId, const int, const int) {}

void play_from_direction(const SfxId, const Dir, const int) {}

void try_play_ambient(const int) {}

void stop_ambient() {}

void play_music(const MusId) {}

void set_music_volume(const int) {}

void fade_out_music() {}

}  // namespace audio
