// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <string>

#include "global.hpp"
#include "state.hpp"
#include "time.hpp"


namespace actor
{
class Actor;
}


struct HistoryEvent
{
        HistoryEvent(const std::string history_msg, const int turn_nr) :
                msg(history_msg),
                turn(turn_nr) {}

        const std::string msg;
        const int turn;
};

namespace game
{

void init();

void save();
void load();

int clvl();
int xp_pct();
int xp_accumulated();
TimeData start_time();

void on_mon_seen(actor::Actor& actor);

void on_mon_killed(actor::Actor& actor);

void win_game();

void set_start_time_to_now();

void incr_player_xp(
        const int xp_gained,
        const Verbosity verbosity = Verbosity::verbose);

void decr_player_xp(int xp_lost);

// This function has no side effects except for incrementing the clvl value
void incr_clvl_number();

void add_history_event(const std::string msg);

const std::vector<HistoryEvent>& history();

} // game

// -----------------------------------------------------------------------------
// Game state
// -----------------------------------------------------------------------------
class GameState: public State
{
public:
        GameState(GameEntryMode entry_mode) :
                State(),
                m_entry_mode(entry_mode) {}

        void on_start() override;

        void draw() override;

        void update() override;

        StateId id() override;

private:
        void query_quit();

        const GameEntryMode m_entry_mode;
};

#endif // GAME_HPP
