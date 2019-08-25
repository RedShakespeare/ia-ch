// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ITEM_SCROLL_HPP
#define ITEM_SCROLL_HPP

#include "item.hpp"


namespace actor
{
class Actor;
}

class Spell;


namespace scroll
{

const int g_low_spawn_chance = 15;
const int g_high_spawn_chance = 35;


void init();

void save();
void load();


class Scroll: public item::Item
{
public:
        Scroll(item::ItemData* const item_data);

        ~Scroll() {}

        void save_hook() const override;

        void load_hook() override;

        Color interface_color() const override
        {
                return colors::magenta();
        }

        std::string name_inf_str() const override;

        ConsumeItem activate(actor::Actor* const actor) override;

        const std::string real_name() const;

        std::vector<std::string> descr_hook() const override;

        void on_player_reached_new_dlvl_hook() override final;

        void on_actor_turn_in_inv_hook(const InvType inv_type) override;

        void identify(const Verbose verbose) override;

        Spell* make_spell() const;

private:
        int m_domain_feeling_dlvl_countdown;
        int m_domain_feeling_turn_countdown;
};

} // scroll

#endif // ITEM_SCROLL_HPP
