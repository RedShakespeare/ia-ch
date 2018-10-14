#ifndef ITEM_SCROLL_HPP
#define ITEM_SCROLL_HPP

#include "item.hpp"

class Actor;
class Spell;

class Scroll: public Item
{
public:
        Scroll(ItemData* const item_data);

        ~Scroll() {}

        void save() const override;

        void load() override;

        Color interface_color() const override
        {
                return colors::magenta();
        }

        std::string name_inf_str() const override;

        ConsumeItem activate(Actor* const actor) override;

        const std::string real_name() const;

        std::vector<std::string> descr() const override;

        void on_player_reached_new_dlvl() override final;

        void on_actor_turn_in_inv(const InvType inv_type) override;

        void identify(const Verbosity verbosity) override;

        Spell* make_spell() const;

private:
        int domain_feeling_dlvl_countdown_;
        int domain_feeling_turn_countdown_;
};

namespace scroll_handling
{

void init();

void save();
void load();

} // scroll_handling

#endif // ITEM_SCROLL_HPP
