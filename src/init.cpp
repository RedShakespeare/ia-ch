// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "init.hpp"

#include <cassert>

#include "audio.hpp"
#include "bot.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "draw_map.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "highscore.hpp"
#include "insanity.hpp"
#include "io.hpp"
#include "item_artifact.hpp"
#include "item_potion.hpp"
#include "item_rod.hpp"
#include "item_scroll.hpp"
#include "line_calc.hpp"
#include "manual.hpp"
#include "map.hpp"
#include "map_templates.hpp"
#include "map_travel.hpp"
#include "msg_log.hpp"
#include "pact.hpp"
#include "panel.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "property_data.hpp"
#include "query.hpp"
#include "saving.hpp"
#include "sdl_base.hpp"

namespace init
{

bool g_is_cheat_vision_enabled = false;

bool g_is_demo_mapgen = false;


void init_io()
{
        TRACE_FUNC_BEGIN;

        sdl_base::init();
        config::init();
        io::init();
        colors::init();

        io::clear_screen();

        // TODO: Use more creative loading messages
        io::draw_text_center(
                "Loading...",
                Panel::screen,
                panels::center(Panel::screen),
                colors::menu_dark());

        io::update_screen();

        query::init();
        audio::init();

        TRACE_FUNC_END;
}

void cleanup_io()
{
        TRACE_FUNC_BEGIN;

        audio::cleanup();
        io::cleanup();
        sdl_base::cleanup();

        TRACE_FUNC_END;
}

void init_game()
{
        TRACE_FUNC_BEGIN;

        saving::init();
        line_calc::init();
        map_templates::init();

        TRACE_FUNC_END;
}

void cleanup_game()
{
        TRACE_FUNC_BEGIN;

        TRACE_FUNC_END;
}

void init_session()
{
        TRACE_FUNC_BEGIN;

        actor::init();
        terrain::init();
        property_data::init();
        item::init();
        scroll::init();
        potion::init();
        rod::init();
        game_time::init();
        map_travel::init();
        map::init();
        player_bon::init();
        insanity::init();
        msg_log::init();
        draw_map::clear();
        game::init();
        bot::init();
        player_spells::init();
        highscore::init();
        pact::init();

        TRACE_FUNC_END;
}

void cleanup_session()
{
        TRACE_FUNC_BEGIN;

        map_templates::clear_base_room_templates_used();

        pact::cleanup();
        highscore::cleanup();
        player_spells::cleanup();
        insanity::cleanup();
        map::cleanup();
        game_time::cleanup();
        item::cleanup();

        TRACE_FUNC_END;
}

} // init
