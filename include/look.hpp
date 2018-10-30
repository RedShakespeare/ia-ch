#ifndef LOOK_HPP
#define LOOK_HPP

#include <string>

#include "colors.hpp"
#include "info_screen_state.hpp"

struct P;

class Actor;

class ViewActorDescr: public InfoScreenState
{
public:
        ViewActorDescr(Actor& actor) :
                InfoScreenState(),
                actor_(actor) {}

        void on_start() override;

        void draw() override;

        void update() override;

        StateId id() override;

private:
        std::string title() const override;

        InfoScreenType type() const override
        {
                return InfoScreenType::scrolling;
        }

        std::string auto_description_str() const;

        std::vector<ColoredString> lines_ {};

        int top_idx_ {0};

        Actor& actor_;
};

namespace look
{

void print_location_info_msgs(const P& pos);

void print_living_actor_info_msg(const P& pos);

} // look

#endif // LOOK_HPP
