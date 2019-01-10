// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "popup.hpp"

#include "audio.hpp"
#include "browser.hpp"
#include "common_text.hpp"
#include "config.hpp"
#include "io.hpp"
#include "msg_log.hpp"
#include "panel.hpp"
#include "query.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static const int text_w_default = 39;

static int get_x0(const int width)
{
        return panels::center_x(Panel::screen) - (width / 2);
}

static int get_box_y0(const int box_h)
{
        return panels::center_y(Panel::screen) - (box_h / 2) - 1;
}

static int get_box_w(const int text_w)
{
        return text_w + 2;
}

static int get_box_h(const int text_h)
{
        return text_h + 2;
}

static int get_title_y(const int text_h)
{
        const int box_h = get_box_h(text_h);

        const int title_y = get_box_y0(box_h) + 1;

        return title_y;
}

static void draw_box(const int text_w, const int text_h)
{
        const int box_w = get_box_w(text_w);
        const int box_h = get_box_h(text_h);

        const int x0 = get_x0(box_w);

        const int y0 = get_box_y0(box_h);

        const int x1 = x0 + box_w - 1;
        const int y1 = y0 + box_h - 1;

        const R rect(x0, y0, x1, y1);

        io::cover_area(
                Panel::screen,
                rect,
                colors::extra_dark_gray());

        io::draw_box(rect);
}

static void draw_menu_popup(
        const std::vector<std::string>& lines,
        const std::vector<std::string>& choices,
        const size_t current_choice,
        const int text_x0,
        const int text_h,
        const std::string& title)
{
        int text_w = text_w_default;

        // If no message lines, set width to widest menu option or title with
        if (lines.empty())
        {
                text_w = title.size();

                for (const std::string& s : choices)
                {
                        text_w = std::max(text_w, (int)s.size());
                }

                text_w += 2;
        }

        draw_box(text_w, text_h);

        int y = get_title_y(text_h);

        if (!title.empty())
        {
                io::draw_text_center(
                        title,
                        Panel::screen,
                        P(panels::center_x(Panel::screen), y),
                        colors::title(),
                        false, // Do not draw background color
                        colors::black(),
                        true); // Allow pixel-level adjustmet
        }

        const bool show_msg_centered = lines.size() == 1;

        for (const std::string& line : lines)
        {
                y++;

                if (show_msg_centered)
                {
                        io::draw_text_center(
                                line,
                                Panel::screen,
                                P(panels::center_x(Panel::screen), y),
                                colors::white(),
                                false, // Do not draw background color
                                colors::black(),
                                true); // Allow pixel-level adjustmet
                }
                else // Draw the message with left alignment
                {
                        io::draw_text(
                                line,
                                Panel::screen,
                                P(text_x0, y),
                                colors::white(),
                                false); // Do not draw background color
                }

                msg_log::add_line_to_history(line);
        }

        if (!lines.empty() || !title.empty())
        {
                y += 2;
        }

        for (size_t i = 0; i < choices.size(); ++i)
        {
                Color color =
                        (i == current_choice) ?
                        colors::menu_highlight() :
                        colors::menu_dark();

                io::draw_text_center(
                        choices[i],
                        Panel::screen,
                        P(panels::center_x(Panel::screen), y),
                        color,
                        false, // Do not draw background color
                        colors::black(),
                        true); // Allow pixel-level adjustmet

                ++y;
        }

        io::update_screen();
}

// -----------------------------------------------------------------------------
// popup
// -----------------------------------------------------------------------------
namespace popup
{

void msg(const std::string& msg,
         const std::string& title,
         const SfxId sfx,
         const int w_change)
{
        const int text_w = text_w_default + w_change;

        const auto lines = text_format::split(msg, text_w);

        const int text_h =  (int)lines.size() + 3;

        draw_box(text_w, text_h);

        int y = get_title_y(text_h);

        if (sfx != SfxId::END)
        {
                audio::play(sfx);
        }

        if (!title.empty())
        {
                io::draw_text_center(
                        title,
                        Panel::screen,
                        P(panels::center_x(Panel::screen), y),
                        colors::title(),
                        false, // Do not draw background color
                        colors::black(),
                        true); // Allow pixel-level adjustmet
        }

        const bool show_msg_centered = lines.size() == 1;

        for (const std::string& line : lines)
        {
                y++;

                if (show_msg_centered)
                {
                        io::draw_text_center(
                                line,
                                Panel::screen,
                                P(panels::center_x(Panel::screen), y),
                                colors::white(),
                                false, // Do not draw background color
                                colors::black(),
                                true); // Allow pixel-level adjustmet
                }
                else
                {
                        const int text_x0 =
                                get_x0(text_w_default) - ((w_change + 1) / 2);

                        io::draw_text(
                                line,
                                Panel::screen,
                                P(text_x0, y),
                                colors::white(),
                                false); // Do not draw background color
                }

                msg_log::add_line_to_history(line);
        }

        y += 2;

        io::draw_text_center(
                common_text::confirm_hint,
                Panel::screen,
                P(panels::center_x(Panel::screen), y),
                colors::menu_dark(),
                false); // Do not draw background color

        io::update_screen();

        query::wait_for_confirm();
}

int menu(const std::string& msg,
         const std::vector<std::string>& choices,
         const std::string& title,
         const SfxId sfx)
{
        if (config::is_bot_playing())
        {
                return 0;
        }

        const auto lines = text_format::split(msg, text_w_default);

        const int title_h = title.empty() ? 0 : 1;

        const int nr_msg_lines = (int)lines.size();

        const int nr_blank_lines =
                ((nr_msg_lines == 0) && (title_h == 0))
                ? 0
                : 1;

        const int nr_choices = (int)choices.size();

        const int text_h_tot =
                title_h +
                nr_msg_lines +
                nr_blank_lines +
                nr_choices;

        MenuBrowser browser(nr_choices);

        if (sfx != SfxId::END)
        {
                audio::play(sfx);
        }

        draw_menu_popup(
                lines,
                choices,
                browser.y(),
                get_x0(text_w_default),
                text_h_tot,
                title);

        audio::play(SfxId::menu_browse);

        while (true)
        {
                const auto input = io::get();

                const MenuAction action =
                        browser.read(input, MenuInputMode::scrolling);

                switch (action)
                {
                case MenuAction::moved:
                        draw_menu_popup(
                                lines,
                                choices,
                                browser.y(),
                                get_x0(text_w_default),
                                text_h_tot,
                                title);
                        break;

                case MenuAction::esc:
                case MenuAction::space:
                        return nr_choices - 1;

                case MenuAction::selected:
                        return browser.y();

                case MenuAction::none:
                        break;
                }
        }
}

} // popup
