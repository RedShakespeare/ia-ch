#ifndef BROWSER_HPP
#define BROWSER_HPP

#include <vector>

#include "global.hpp"
#include "random.hpp"

struct InputData;

enum class MenuAction
{
        none,
        moved,
        selected,
        space,
        esc
};

enum class MenuInputMode
{
        scrolling_and_letters,
        scrolling
};

const std::vector<char> std_menu_keys =
{
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',

        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
};

// TODO: There's probably some public methods here that could be private/removed
class MenuBrowser
{
public:
        MenuBrowser(const int nr_items, const int list_h = - 1)
        {
                reset(nr_items, list_h);
        }

        MenuBrowser() {}

        MenuBrowser& operator=(const MenuBrowser&) = default;

        MenuAction read(const InputData& input, MenuInputMode mode);

        void move(const VerDir dir);

        void move_page(const VerDir dir);

        int y() const
        {
                return y_;
        }

        void set_y(const int y);

        Range range_shown() const;

        int nr_items_shown() const;

        int top_idx_shown() const;

        int btm_idx_shown() const;

        bool is_on_top_page() const;

        bool is_on_btm_page() const;

        int nr_items_tot() const
        {
                return nr_items_;
        }

        bool is_at_idx(const int idx) const
        {
                return y_ == idx;
        }

        void reset(const int nr_items, const int list_h = -1);

        const std::vector<char>& menu_keys() const
        {
                return menu_keys_;
        }

        void set_custom_menu_keys(const std::vector<char>& keys)
        {
                menu_keys_ = keys;
        }

private:
        void set_y_nearest_valid();

        void update_range_shown();

        std::vector<char> menu_keys_ {std_menu_keys};
        int nr_items_ {0};
        int y_ {0};
        int list_h_ {-1};
        Range range_shown_ {-1, -1};
};

#endif // BROWSER_HPP
