// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef LOOK_HPP
#define LOOK_HPP

#include <string>

#include "colors.hpp"
#include "info_screen_state.hpp"


namespace actor
{
class Actor;
}


struct P;


class ViewActorDescr: public InfoScreenState
{
public:
        ViewActorDescr(actor::Actor& actor) :
                InfoScreenState(),
                m_actor(actor) {}

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

        std::vector<ColoredString> m_lines {};

        int m_top_idx {0};

        actor::Actor& m_actor;
};


namespace look
{

void print_location_info_msgs(const P& pos);

void print_living_actor_info_msg(const P& pos);

} // look

#endif // LOOK_HPP
