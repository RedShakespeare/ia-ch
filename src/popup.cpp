// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "popup.hpp"

#include "audio.hpp"
#include "browser.hpp"
#include "common_text.hpp"
#include "config.hpp"
#include "draw_box.hpp"
#include "io.hpp"
#include "msg_log.hpp"
#include "panel.hpp"
#include "query.hpp"
#include "rect.hpp"
#include "sdl_base.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
enum class HighlightMenuKeys {
        no,
        yes
};

static const int s_text_w_default = 39;

static const uint32_t s_msg_line_delay = 50;

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

static void draw_popup_box(const int text_w, const int text_h)
{
        const int box_w = get_box_w(text_w + 2);
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

        draw_box(rect, colors::gray());
}

static void draw_menu_popup(
        const std::vector<std::string>& lines,
        const std::vector<std::string>& choices,
        const size_t current_choice,
        int text_w,
        const int text_x0,
        const int text_h,
        const std::string& title,
        const HighlightMenuKeys highlight_keys)
{
        // If no message lines, set width to widest menu option or title with
        if (lines.empty()) {
                text_w = title.size();

                for (const std::string& s : choices) {
                        text_w = std::max(text_w, (int)s.size());
                }

                text_w += 2;
        }

        draw_popup_box(text_w, text_h);

        int y = get_title_y(text_h);

        if (!title.empty()) {
                io::draw_text_center(
                        title,
                        Panel::screen,
                        P(panels::center_x(Panel::screen), y),
                        colors::title(),
                        io::DrawBg::no,
                        colors::black(),
                        true); // Allow pixel-level adjustmet

                ++y;
        }

        const bool show_msg_centered = lines.size() == 1;

        for (const std::string& line : lines) {
                if (show_msg_centered) {
                        io::draw_text_center(
                                line,
                                Panel::screen,
                                P(panels::center_x(Panel::screen), y),
                                colors::text(),
                                io::DrawBg::no,
                                colors::black(),
                                true); // Allow pixel-level adjustmet
                } else {
                        // Draw the message with left alignment
                        io::draw_text(
                                line,
                                Panel::screen,
                                P(text_x0, y),
                                colors::text(),
                                io::DrawBg::no);
                }

                msg_log::add_line_to_history(line);

                ++y;
        }

        if (!lines.empty() || !title.empty()) {
                ++y;
        }

        int longest_choice_len = 0;

        std::for_each(
                std::begin(choices),
                std::end(choices),
                [&longest_choice_len](const auto& choice_str) {
                        longest_choice_len =
                                std::max(
                                        longest_choice_len,
                                        (int)choice_str.length());
                });

        const int choice_x_pos =
                panels::center_x(Panel::screen) -
                (longest_choice_len / 2);

        for (size_t i = 0; i < choices.size(); ++i) {
                const auto choice_str = choices[i];

                int choice_suffix_start = 0;
                int draw_x_pos = choice_x_pos;

                if (highlight_keys == HighlightMenuKeys::yes) {
                        auto key_str = choice_str.substr(0, 3);

                        const auto key_color =
                                (i == current_choice)
                                ? colors::menu_key_highlight()
                                : colors::menu_key_dark();

                        io::draw_text(
                                key_str,
                                Panel::screen,
                                {draw_x_pos, y},
                                key_color);

                        choice_suffix_start = key_str.length();
                        draw_x_pos = choice_x_pos + key_str.length();
                }

                const auto choice_suffix =
                        choice_str.substr(
                                choice_suffix_start,
                                std::string::npos);

                const auto color =
                        (i == current_choice)
                        ? colors::menu_highlight()
                        : colors::menu_dark();

                io::draw_text(
                        choice_suffix,
                        Panel::screen,
                        {draw_x_pos, y},
                        color);

                ++y;
        }

        io::update_screen();
}

// -----------------------------------------------------------------------------
// popup
// -----------------------------------------------------------------------------
namespace popup {

// TODO: Instead of this sh***y "w_change", the popup should calculate a good
// size itself (not breaking lines before the last word of a sentence, etc).
void msg(
        const std::string& msg,
        const std::string& title,
        const audio::SfxId sfx,
        const int w_change)
{
        states::draw();

        msg_log::more_prompt();

        const int text_w = s_text_w_default + w_change;

        const auto lines = text_format::split(msg, text_w);

        const int text_h = (int)lines.size() + 3;

        draw_popup_box(text_w, text_h);

        int y = get_title_y(text_h);

        if (sfx != audio::SfxId::END) {
                audio::play(sfx);
        }

        if (!title.empty()) {
                io::draw_text_center(
                        title,
                        Panel::screen,
                        P(panels::center_x(Panel::screen), y),
                        colors::title(),
                        io::DrawBg::no,
                        colors::black(),
                        true); // Allow pixel-level adjustmet
        }

        const bool show_msg_centered = lines.size() == 1;

        for (const std::string& line : lines) {
                ++y;

                if (show_msg_centered) {
                        io::draw_text_center(
                                line,
                                Panel::screen,
                                P(panels::center_x(Panel::screen), y),
                                colors::text(),
                                io::DrawBg::no,
                                colors::black(),
                                true); // Allow pixel-level adjustmet
                } else {
                        const int text_x0 =
                                get_x0(s_text_w_default) -
                                ((w_change + 1) / 2);

                        io::draw_text(
                                line,
                                Panel::screen,
                                P(text_x0, y),
                                colors::text(),
                                io::DrawBg::no);
                }

                io::update_screen();

                sdl_base::sleep(s_msg_line_delay);

                msg_log::add_line_to_history(line);
        }

        y += 2;

        io::draw_text_center(
                common_text::g_confirm_hint,
                Panel::screen,
                P(panels::center_x(Panel::screen), y),
                colors::menu_dark(),
                io::DrawBg::no);

        io::update_screen();

        io::clear_events();

        query::wait_for_confirm();
}

// TODO: Instead of this sh***y "w_change", the popup should calculate a good
// size itself (not breaking lines before the last word of a sentence, etc).
int menu(
        const std::string& msg,
        const std::vector<std::string>& choices,
        const std::string& title,
        const int w_change,
        const audio::SfxId sfx,
        const std::vector<char>& menu_keys)
{
        states::draw();

        msg_log::more_prompt();

        io::clear_events();

        if (config::is_bot_playing()) {
                return 0;
        }

        const int text_w = s_text_w_default + w_change;

        const auto lines = text_format::split(msg, text_w);

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
        auto highlight_keys = HighlightMenuKeys::no;
        auto input_mode = MenuInputMode::scrolling;

        if (!menu_keys.empty()) {
                browser.set_custom_menu_keys(menu_keys);
                highlight_keys = HighlightMenuKeys::yes;
                input_mode = MenuInputMode::scrolling_and_letters;
        }

        if (sfx != audio::SfxId::END) {
                audio::play(sfx);
        }

        draw_menu_popup(
                lines,
                choices,
                browser.y(),
                text_w,
                get_x0(text_w),
                text_h_tot,
                title,
                highlight_keys);

        audio::play(audio::SfxId::menu_browse);

        while (true) {
                const auto input = io::get();
                const auto action = browser.read(input, input_mode);

                switch (action) {
                case MenuAction::moved:
                        draw_menu_popup(
                                lines,
                                choices,
                                browser.y(),
                                text_w,
                                get_x0(text_w),
                                text_h_tot,
                                title,
                                highlight_keys);
                        break;

                case MenuAction::esc:
                case MenuAction::space:
                        return nr_choices - 1;

                case MenuAction::selected:
                        return browser.y();

                case MenuAction::left:
                case MenuAction::right:
                case MenuAction::none:
                        break;
                }
        }
}

} // namespace popup
