// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

#include "browser.hpp"
#include "state.hpp"

enum class InputMode
{
        standard,
        vi_keys,

        END
};

namespace config
{
void init();

InputMode input_mode();

bool is_tiles_mode();

std::string font_name();

void set_fullscreen( bool value );

bool is_fullscreen();

bool is_2x_scale_fullscreen_requested();

void set_2x_scale_fullscreen_enabled( bool value );

bool is_2x_scale_fullscreen_enabled();

void set_screen_px_w( int w );

void set_screen_px_h( int h );

int screen_px_w();

int screen_px_h();

int gui_cell_px_w();

int gui_cell_px_h();

int map_cell_px_w();

int map_cell_px_h();

bool is_text_mode_wall_full_square();

bool is_tiles_wall_full_square();

bool is_audio_enabled();

bool is_amb_audio_enabled();

bool is_amb_audio_preloaded();

bool is_bot_playing();

void toggle_bot_playing();

bool is_gj_mode();

void toggle_gj_mode();

bool is_light_explosive_prompt();

bool is_drink_malign_pot_prompt();

bool is_ranged_wpn_meleee_prompt();

bool is_ranged_wpn_auto_reload();

bool is_intro_lvl_skipped();

bool is_intro_popup_skipped();

bool is_any_key_confirm_more();

bool should_display_hints();

bool always_warn_new_mon();

int delay_projectile_draw();

int delay_shotgun();

int delay_explosion();

void set_default_player_name( const std::string& name );

std::string default_player_name();

}  // namespace config

class ConfigState : public State
{
public:
        ConfigState();

        void update() override;

        void draw() override;

        StateId id() const override;

private:
        MenuBrowser m_browser;
};

#endif  // CONFIG_HPP
