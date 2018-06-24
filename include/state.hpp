#ifndef STATE_HPP
#define STATE_HPP

#include <vector>
#include <memory>

enum class StateId
{
        browse_highscore_entry,
        browse_spells,
        config,
        descript,
        game,
        highscore,
        inventory,
        manual,
        manual_page,
        marker,
        menu,
        message,
        new_game,
        new_level,
        pick_background,
        pick_name,
        pick_trait,
        postmortem_info,
        postmortem_menu,
        view_actor,
        view_minimap
};

class State
{
public:
        State() :
                has_started_(false) {}

        virtual ~State() {}

        // Executed immediately when the state is pushed.
        virtual void on_pushed() {}

        // Executed the first time that the state becomes the current state.
        // Sometimes multiple states may be pushed in a sequence, and one of the
        // later states may want to perform actions only when it actually
        // becomes the current state.
        virtual void on_start() {}

        // Executed immediately when the state is popped.
        // This should only be used for cleanup, do not push or pop other states
        // from this call (this is not supported).
        virtual void on_popped() {}

        virtual void draw() {}

        // If true, this state is drawn overlayed on the state(s) below. This
        // can be used for example to draw a marker state on top of the map.
        virtual bool draw_overlayed() const
        {
                return false;
        }

        virtual void on_window_resized() {}

        // Read input, process game logic etc.
        virtual void update() {}

        // All states above have been popped
        virtual void on_resume() {}

        // Another state is pushed on top
        virtual void on_pause() {}

        bool has_started() const
        {
                return has_started_;
        }

        void set_started()
        {
                has_started_ = true;
        }

        bool has_started()
        {
                return has_started_;
        }

        virtual StateId id() = 0;

private:
        bool has_started_;
};

namespace states
{

void init();

void cleanup();

void start();

void draw();

void on_window_resized();

void update();

void push(std::unique_ptr<State> state);

void pop();

void pop_all();

bool is_empty();

bool is_current_state(const State& state);

void pop_until(const StateId id);

bool contains_state(const StateId id);

} // states

#endif // STATE_HPP
