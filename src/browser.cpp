#include "browser.hpp"

#include "array2.hpp"
#include "audio.hpp"
#include "config.hpp"
#include "io.hpp"
#include "misc.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static int nr_menu_keys_avail(const std::vector<char>& menu_keys)
{
        return (int)std::distance(std::begin(menu_keys), std::end(menu_keys));
}

// -----------------------------------------------------------------------------
// MenuBrowser
// -----------------------------------------------------------------------------
MenuAction MenuBrowser::read(
        const InputData& input,
        MenuInputMode mode)
{
        if ((input.key == SDLK_UP) || (input.key == SDLK_KP_8))
        {
                move(VerDir::up);

                return MenuAction::moved;
        }
        else if ((input.key == SDLK_DOWN) || (input.key == SDLK_KP_2))
        {
                move(VerDir::down);

                return MenuAction::moved;
        }
        else if ((input.key == SDLK_PAGEUP) || (input.key == '<'))
        {
                move_page(VerDir::up);

                return MenuAction::moved;
        }
        else if ((input.key == SDLK_PAGEDOWN) || (input.key == '>'))
        {
                move_page(VerDir::down);

                return MenuAction::moved;
        }
        else if (input.key == SDLK_RETURN)
        {
                audio::play(SfxId::menu_select);

                return MenuAction::selected;
        }
        else if (input.key == SDLK_SPACE)
        {
                return MenuAction::space;
        }
        else if (input.key == SDLK_ESCAPE)
        {
                return MenuAction::esc;
        }
        else if (mode == MenuInputMode::scrolling_and_letters)
        {
                const char c = input.key;

                const auto find_result =
                        std::find(
                                std::begin(menu_keys_),
                                std::end(menu_keys_),
                                c);

                if (find_result == std::end(menu_keys_))
                {
                        // Not a valid menu key, ever
                        return MenuAction::none;
                }

                const auto relative_idx =
                        (int)std::distance(
                                std::begin(menu_keys_),
                                find_result);

                if (relative_idx >= nr_items_shown())
                {
                        // The key is not in the range of shown items
                        return MenuAction::none;
                }

                // OK, the user did select an item
                const int global_idx = top_idx_shown() + relative_idx;

                set_y(global_idx);

                audio::play(SfxId::menu_select);

                return MenuAction::selected;
        }
        else
        {
                return MenuAction::none;
        }
}

void MenuBrowser::move(const VerDir dir)
{
        const int last_idx = nr_items_ - 1;

        if (dir == VerDir::up)
        {
                y_ = y_ == 0 ? last_idx : (y_ - 1);
        }
        else // Down
        {
                y_ = y_ == last_idx ? 0 : (y_ + 1);
        }

        update_range_shown();

        audio::play(SfxId::menu_browse);
}

void MenuBrowser::move_page(const VerDir dir)
{
        if (dir == VerDir::up)
        {
                if (list_h_ >= 0)
                {
                        y_ -= list_h_;
                }
                else // List height undefined (i.e. showing all)
                {
                        y_ = 0;
                }
        }
        else // Down
        {
                if (list_h_ >= 0)
                {
                        y_ += list_h_;
                }
                else // List height undefined (i.e. showing all)
                {
                        y_ = nr_items_ - 1;
                }
        }

        set_y_nearest_valid();

        update_range_shown();

        audio::play(SfxId::menu_browse);
}

void MenuBrowser::set_y(const int y)
{
        y_ = y;

        set_y_nearest_valid();

        update_range_shown();
}

Range MenuBrowser::range_shown() const
{
        // Shown ranged defined?
        if (list_h_ >= 0)
        {
                return range_shown_;
        }
        else // List height undefined (i.e. showing all)
        {
                // Just return a range of the total number of items
                return Range(0, nr_items_ - 1);
        }
}

void MenuBrowser::update_range_shown()
{
        // Shown ranged defined?
        if (list_h_ >= 0)
        {
                const int top = (y_ / list_h_) * list_h_;
                const int btm = std::min(top + list_h_, nr_items_) - 1;

                range_shown_.set(top, btm);
        }
}

void MenuBrowser::set_y_nearest_valid()
{
        set_constr_in_range(0, y_, nr_items_ - 1);
}

int MenuBrowser::nr_items_shown() const
{
        if (list_h_ >= 0)
        {
                // The list height has been defined
                return range_shown_.len();
        }
        else
        {
                // List height undefined (i.e. showing all) - just return total
                // number of items
                return nr_items_;
        }
}

int MenuBrowser::top_idx_shown() const
{
        // Shown ranged defined?
        if (list_h_ >= 0)
        {
                // List height undefined (i.e. showing all)
                return range_shown_.min;
        }
        else // Not showing all items
        {
                return 0;
        }
}

int MenuBrowser::btm_idx_shown() const
{
        // Shown ranged defined?
        if (list_h_ >= 0)
        {
                return range_shown_.max;
        }
        else // List height undefined (i.e. showing all)
        {
                return nr_items_ - 1;
        }
}

bool MenuBrowser::is_on_top_page() const
{
        // Shown ranged defined?
        if (list_h_ >= 0)
        {
                return range_shown_.min == 0;
        }
        else // List height undefined (i.e. showing all)
        {
                return true;
        }
}

bool MenuBrowser::is_on_btm_page() const
{
        // Shown ranged defined?
        if (list_h_ >= 0)
        {
                return range_shown_.max == nr_items_ - 1;
        }
        else // List height undefined (i.e. showing all)
        {
                return true;
        }
}

void MenuBrowser::reset(const int nr_items, const int list_h)
{
        nr_items_ = nr_items;

        // The size of the list viewable on screen is capped to the global
        // number of menu selection keys available (note that the client asks
        // the browser how many items should actually be drawn, so this capping
        // should be reflected for all clients).
        list_h_ = std::min(list_h, nr_menu_keys_avail(menu_keys_));

        set_y_nearest_valid();

        update_range_shown();
}
