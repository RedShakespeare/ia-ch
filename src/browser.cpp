// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

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
MenuAction MenuBrowser::read(const InputData& input, MenuInputMode mode)
{
        // NOTE: j k l are reserved for browsing with vi keys (not included in
        // the standard menu key letters)

        if ((input.key == SDLK_UP) ||
            (input.key == SDLK_KP_8) ||
            (input.key == 'k'))
        {
                move(VerDir::up);

                return MenuAction::moved;
        }
        else if ((input.key == SDLK_DOWN) ||
                 (input.key == SDLK_KP_2) ||
                 (input.key == 'j'))
        {
                move(VerDir::down);

                return MenuAction::moved;
        }
        else if ((input.key == SDLK_PAGEUP) ||
                 (input.key == '<'))
        {
                move_page(VerDir::up);

                return MenuAction::moved;
        }
        else if ((input.key == SDLK_PAGEDOWN) ||
                 (input.key == '>'))
        {
                move_page(VerDir::down);

                return MenuAction::moved;
        }
        else if ((input.key == SDLK_RETURN) ||
                 (input.key == 'l'))
        {
                if (m_play_selection_audio)
                {
                        audio::play(SfxId::menu_select);
                }

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
                const char c = (char)input.key;

                const auto find_result =
                        std::find(
                                std::begin(m_menu_keys),
                                std::end(m_menu_keys),
                                c);

                if (find_result == std::end(m_menu_keys))
                {
                        // Not a valid menu key, ever
                        return MenuAction::none;
                }

#ifndef NDEBUG
                // Should never be used as letters (reserved for browsing)
                if ((c == 'j') || (c == 'k') || (c == 'l'))
                {
                        PANIC;
                }
#endif // NDEBUG

                const auto relative_idx =
                        (int)std::distance(
                                std::begin(m_menu_keys),
                                find_result);

                if (relative_idx >= nr_items_shown())
                {
                        // The key is not in the range of shown items
                        return MenuAction::none;
                }

                // OK, the user did select an item
                const int global_idx = top_idx_shown() + relative_idx;

                set_y(global_idx);

                if (m_play_selection_audio)
                {
                        audio::play(SfxId::menu_select);
                }

                return MenuAction::selected;
        }
        else
        {
                return MenuAction::none;
        }
}

void MenuBrowser::move(const VerDir dir)
{
        const int last_idx = m_nr_items - 1;

        if (dir == VerDir::up)
        {
                m_y = m_y == 0 ? last_idx : (m_y - 1);
        }
        else // Down
        {
                m_y = m_y == last_idx ? 0 : (m_y + 1);
        }

        update_range_shown();

        audio::play(SfxId::menu_browse);
}

void MenuBrowser::move_page(const VerDir dir)
{
        if (dir == VerDir::up)
        {
                if (m_list_h >= 0)
                {
                        m_y -= m_list_h;
                }
                else // List height undefined (i.e. showing all)
                {
                        m_y = 0;
                }
        }
        else // Down
        {
                if (m_list_h >= 0)
                {
                        m_y += m_list_h;
                }
                else // List height undefined (i.e. showing all)
                {
                        m_y = m_nr_items - 1;
                }
        }

        set_y_nearest_valid();

        update_range_shown();

        audio::play(SfxId::menu_browse);
}

void MenuBrowser::set_y(const int y)
{
        m_y = y;

        set_y_nearest_valid();

        update_range_shown();
}

Range MenuBrowser::range_shown() const
{
        // Shown ranged defined?
        if (m_list_h >= 0)
        {
                return m_range_shown;
        }
        else // List height undefined (i.e. showing all)
        {
                // Just return a range of the total number of items
                return Range(0, m_nr_items - 1);
        }
}

void MenuBrowser::update_range_shown()
{
        // Shown ranged defined?
        if (m_list_h >= 0)
        {
                const int top = (m_y / m_list_h) * m_list_h;
                const int btm = std::min(top + m_list_h, m_nr_items) - 1;

                m_range_shown.set(top, btm);
        }
}

void MenuBrowser::set_y_nearest_valid()
{
        set_constr_in_range(0, m_y, m_nr_items - 1);
}

int MenuBrowser::nr_items_shown() const
{
        if (m_list_h >= 0)
        {
                // The list height has been defined
                return m_range_shown.len();
        }
        else
        {
                // List height undefined (i.e. showing all) - just return total
                // number of items
                return m_nr_items;
        }
}

int MenuBrowser::top_idx_shown() const
{
        // Shown ranged defined?
        if (m_list_h >= 0)
        {
                // List height undefined (i.e. showing all)
                return m_range_shown.min;
        }
        else // Not showing all items
        {
                return 0;
        }
}

int MenuBrowser::btm_idx_shown() const
{
        // Shown ranged defined?
        if (m_list_h >= 0)
        {
                return m_range_shown.max;
        }
        else // List height undefined (i.e. showing all)
        {
                return m_nr_items - 1;
        }
}

bool MenuBrowser::is_on_top_page() const
{
        // Shown ranged defined?
        if (m_list_h >= 0)
        {
                return m_range_shown.min == 0;
        }
        else // List height undefined (i.e. showing all)
        {
                return true;
        }
}

bool MenuBrowser::is_on_btm_page() const
{
        // Shown ranged defined?
        if (m_list_h >= 0)
        {
                return m_range_shown.max == m_nr_items - 1;
        }
        else // List height undefined (i.e. showing all)
        {
                return true;
        }
}

void MenuBrowser::reset(const int nr_items, const int list_h)
{
        m_nr_items = nr_items;

        // The size of the list viewable on screen is capped to the global
        // number of menu selection keys available (note that the client asks
        // the browser how many items should actually be drawn, so this capping
        // should be reflected for all clients).
        m_list_h = std::min(list_h, nr_menu_keys_avail(m_menu_keys));

        set_y_nearest_valid();

        update_range_shown();
}
