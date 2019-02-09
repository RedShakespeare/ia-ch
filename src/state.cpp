// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "state.hpp"

#include "io.hpp"

#include "debug.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector< std::unique_ptr<State> > s_current_states;


//-----------------------------------------------------------------------------
// State keeping
//-----------------------------------------------------------------------------
namespace states
{

void init()
{
        TRACE_FUNC_BEGIN;

        cleanup();

        TRACE_FUNC_END;
}

void cleanup()
{
        TRACE_FUNC_BEGIN;

        s_current_states.resize(0);

        TRACE_FUNC_END;
}

void start()
{
        while (!s_current_states.empty() &&
               !s_current_states.back()->has_started())
        {
                auto& state = s_current_states.back();

                state->set_started();

                // NOTE: This may cause states to be pushed/popped - do not use
                // the "state" pointer beyond this call!
                state->on_start();
        }
}

void draw()
{
        if (s_current_states.empty())
        {
                return;
        }

        // Find the first state from the end which is NOT drawn overlayed
        auto draw_from = std::end(s_current_states);

        while (draw_from != std::begin(s_current_states))
        {
                --draw_from;

                const auto& state_ptr = *draw_from;

                // If not drawn overlayed, draw from this state as bottom layer
                // (but only if the state has been started, see note below)
                if (!state_ptr->draw_overlayed() &&
                    state_ptr->has_started())
                {
                        break;
                }
        }

        // Draw every state from this (non-overlayed) state onward
        for ( ; draw_from != std::end(s_current_states); ++draw_from)
        {
                const auto& state_ptr = *draw_from;

                // Do NOT draw states which are not yet started (they may need
                // to set up menus etc in their start function, and expect the
                // chance to do so before drawing is called)

                if (state_ptr->has_started())
                {
                        state_ptr->draw();
                }
        }
}

void on_window_resized()
{
        for (auto& state : s_current_states)
        {
                state->on_window_resized();
        }
}

void update()
{
        if (s_current_states.empty())
        {
                return;
        }

        s_current_states.back()->update();
}

void push(std::unique_ptr<State> state)
{
        TRACE_FUNC_BEGIN;

        // Pause the current state
        if (!s_current_states.empty())
        {
                s_current_states.back()->on_pause();
        }

        s_current_states.push_back(std::move(state));

        s_current_states.back()->on_pushed();

        io::flush_input();

        TRACE_FUNC_END;
}

void pop()
{
        TRACE_FUNC_BEGIN;

        if (s_current_states.empty())
        {
                TRACE_FUNC_END;

                return;
        }

        s_current_states.back()->on_popped();

        s_current_states.pop_back();

        if (!s_current_states.empty())
        {
                s_current_states.back()->on_resume();
        }

        io::flush_input();

        TRACE_FUNC_END;
}

void pop_all()
{
        TRACE_FUNC_BEGIN;

        while (!s_current_states.empty())
        {
                s_current_states.back()->on_popped();

                s_current_states.pop_back();
        }

        TRACE_FUNC_END;
}

bool contains_state(const StateId id)
{
        for (auto& state : s_current_states)
        {
                if (state->id() == id)
                {
                        return true;
                }
        }

        return false;
}

void pop_until(const StateId id)
{
        TRACE_FUNC_BEGIN;

        if (is_empty() || !contains_state(id))
        {
                ASSERT(false);

                return;
        }

        while (s_current_states.back().get()->id() != id)
        {
                pop();
        }

        TRACE_FUNC_END;
}

bool is_current_state(const State& state)
{
        if (is_empty())
        {
                return false;
        }

        return &state == s_current_states.back().get();
}

bool is_empty()
{
        return s_current_states.empty();
}

} // states
