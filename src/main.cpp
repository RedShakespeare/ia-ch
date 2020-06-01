// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "init.hpp"

#include "config.hpp"
#include "debug.hpp"
#include "init.hpp"
#include "io.hpp"
#include "main_menu.hpp"
#include "random.hpp"
#include "state.hpp"

#ifdef _WIN32
#undef main
#endif
int main(int argc, char** argv)
{
        TRACE_FUNC_BEGIN;

        rnd::seed();

        init::init_io();

#ifdef NDEBUG
        (void)argc;
        (void)argv;
#else // NDEBUG
        for (int arg_nr = 0; arg_nr < argc; ++arg_nr) {
                const std::string arg_str = std::string(argv[arg_nr]);

                if (arg_str == "--demo-mapgen") {
                        init::g_is_demo_mapgen = true;
                }

                if (arg_str == "--bot") {
                        config::toggle_bot_playing();
                }

                // Extra challenge for user "GJ" from the Discord chat ;-)
                if (arg_str == "--gj") {
                        config::toggle_gj_mode();
                }
        }
#endif // NDEBUG

        init::init_game();

        std::unique_ptr<State> main_menu_state(new MainMenuState);

        states::push(std::move(main_menu_state));

        // Loop while there is at least one state
        while (!states::is_empty()) {
                states::start();

                if (states::is_empty()) {
                        break;
                }

                io::clear_screen();

                states::draw();

                io::update_screen();

                states::update();
        }

        init::cleanup_session();
        init::cleanup_game();
        init::cleanup_io();

        return 0;
}
