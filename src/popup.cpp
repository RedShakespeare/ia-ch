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
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
enum class HighlightMenuKeys {
        no,
        yes
};

static const int s_text_w_default = 39;

static int get_x0(const int width)
{
        return panels::center_x(Panel::screen) - (width / 2);
}

static int get_box_y0(const int box_h)
{
        return panels::center_y(Panel::screen) - (box_h / 2) - 1;
}

static int get_title_y(const int text_h)
{
        const int box_h = text_h + 2;

        const int title_y = get_box_y0(box_h) + 1;

        return title_y;
}

static void draw_popup_box(const int text_w, const int text_h)
{
        const int box_w = text_w + 4;
        const int box_h = text_h + 2;

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

// -----------------------------------------------------------------------------
// popup
// -----------------------------------------------------------------------------
namespace popup {

// -----------------------------------------------------------------------------
// Popup
// -----------------------------------------------------------------------------
Popup::Popup(const AddToMsgHistory add_to_msg_history) :
        m_popup_state(std::make_unique<PopupState>(add_to_msg_history))
{
}

Popup::~Popup()
{
        // Sanity check: The popup has actually been run
        ASSERT(!m_popup_state);
}

void Popup::run()
{
        states::run_until_state_done(std::move(m_popup_state));
}

Popup& Popup::set_title(const std::string& title)
{
        m_popup_state->m_title = title;

        return *this;
}

Popup& Popup::set_msg(const std::string& msg)
{
        m_popup_state->m_msg = msg;

        return *this;
}

Popup& Popup::set_menu(
        const std::vector<std::string>& choices,
        const std::vector<char>& menu_keys,
        int* menu_choice_result)
{
        m_popup_state->m_menu_choices = choices;
        m_popup_state->m_menu_keys = menu_keys;
        m_popup_state->m_menu_choice_result = menu_choice_result;

        return *this;
}

Popup& Popup::set_sfx(audio::SfxId sfx)
{
        m_popup_state->m_sfx = sfx;

        return *this;
}

// -----------------------------------------------------------------------------
// PopupState
// -----------------------------------------------------------------------------
PopupState::PopupState(const AddToMsgHistory add_to_msg_history) :
        m_add_to_msg_history(add_to_msg_history)
{
}

void PopupState::on_start()
{
        ASSERT(m_menu_choices.size() == m_menu_keys.size());

        if (m_sfx != audio::SfxId::END) {
                audio::play(m_sfx);
        }

        if (m_add_to_msg_history == AddToMsgHistory::yes) {
                if (!m_title.empty()) {
                        msg_log::add_line_to_history(m_title);
                }

                if (!m_msg.empty()) {
                        const auto lines =
                                text_format::split(
                                        m_msg,
                                        io::g_min_nr_gui_cells_x - 2);

                        for (const auto& line : lines) {
                                msg_log::add_line_to_history(line);
                        }
                }
        }

        if (!m_menu_choices.empty()) {
                m_browser.reset(m_menu_choices.size());

                m_browser.set_custom_menu_keys(m_menu_keys);
        }
}

void PopupState::draw()
{
        if (m_menu_choices.empty()) {
                draw_msg_popup();
        } else {
                draw_menu_popup();
        }
}

void PopupState::draw_msg_popup() const
{
        const auto text_w = s_text_w_default;
        const auto lines = text_format::split(m_msg, text_w);
        const auto text_h = (int)lines.size() + 3;

        draw_popup_box(text_w, text_h);

        auto y = get_title_y(text_h);

        if (!m_title.empty()) {
                io::draw_text_center(
                        m_title,
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
                        const int text_x0 = get_x0(s_text_w_default);

                        io::draw_text(
                                line,
                                Panel::screen,
                                P(text_x0, y),
                                colors::text(),
                                io::DrawBg::no);
                }
        }

        y += 2;

        io::draw_text_center(
                common_text::g_confirm_hint,
                Panel::screen,
                P(panels::center_x(Panel::screen), y),
                colors::menu_dark(),
                io::DrawBg::no);
}

void PopupState::draw_menu_popup() const
{
        int text_w = s_text_w_default;
        const auto lines = text_format::split(m_msg, text_w);
        const auto title_h = m_title.empty() ? 0 : 1;
        const auto nr_msg_lines = (int)lines.size();

        const auto nr_blank_lines =
                ((nr_msg_lines == 0) && (title_h == 0))
                ? 0
                : 1;

        const auto nr_choices = (int)m_menu_choices.size();

        const auto text_h_tot =
                title_h +
                nr_msg_lines +
                nr_blank_lines +
                nr_choices;

        // If no message lines, set width to title with or widest menu option
        if (lines.empty()) {
                text_w = m_title.size();

                for (const std::string& s : m_menu_choices) {
                        text_w = std::max(text_w, (int)s.size());
                }

                text_w += 2;
        }

        draw_popup_box(text_w, text_h_tot);

        int y = get_title_y(text_h_tot);

        if (!m_title.empty()) {
                io::draw_text_center(
                        m_title,
                        Panel::screen,
                        {panels::center_x(Panel::screen), y},
                        colors::title(),
                        io::DrawBg::no,
                        colors::black(),
                        true); // Allow pixel-level adjustmet

                ++y;
        }

        const bool show_msg_centered = (lines.size() == 1);

        auto text_x0 = get_x0(text_w);

        for (const std::string& line : lines) {
                if (show_msg_centered) {
                        io::draw_text_center(
                                line,
                                Panel::screen,
                                {panels::center_x(Panel::screen), y},
                                colors::text(),
                                io::DrawBg::no,
                                colors::black(),
                                true); // Allow pixel-level adjustmet
                } else {
                        // Draw the message with left alignment
                        io::draw_text(
                                line,
                                Panel::screen,
                                {text_x0, y},
                                colors::text(),
                                io::DrawBg::no);
                }

                ++y;
        }

        if (!lines.empty() || !m_title.empty()) {
                ++y;
        }

        int longest_choice_len = 0;

        std::for_each(
                std::begin(m_menu_choices),
                std::end(m_menu_choices),
                [&longest_choice_len](const auto& choice_str) {
                        longest_choice_len =
                                std::max(
                                        longest_choice_len,
                                        (int)choice_str.length());
                });

        const int choice_x_pos =
                panels::center_x(Panel::screen) -
                (longest_choice_len / 2);

        for (size_t i = 0; i < m_menu_choices.size(); ++i) {
                const auto choice_str = m_menu_choices[i];
                const auto key_str = choice_str.substr(0, 3);

                int draw_x_pos = choice_x_pos;

                const auto key_color =
                        m_browser.is_at_idx(i)
                        ? colors::menu_key_highlight()
                        : colors::menu_key_dark();

                io::draw_text(
                        key_str,
                        Panel::screen,
                        {draw_x_pos, y},
                        key_color,
                        io::DrawBg::no);

                const auto choice_suffix_start = key_str.length();

                draw_x_pos = choice_x_pos + key_str.length();

                const auto choice_suffix =
                        choice_str.substr(
                                choice_suffix_start,
                                std::string::npos);

                const auto color =
                        m_browser.is_at_idx(i)
                        ? colors::menu_highlight()
                        : colors::menu_dark();

                io::draw_text(
                        choice_suffix,
                        Panel::screen,
                        {draw_x_pos, y},
                        color,
                        io::DrawBg::no);

                ++y;
        }

        io::update_screen();
}

void PopupState::on_window_resized()
{
}

void PopupState::update()
{
        if (config::is_bot_playing()) {
                *m_menu_choice_result = 0;

                states::pop();

                return;
        }

        const auto input = io::get();

        if (m_menu_choices.empty()) {
                switch (input.key) {
                case SDLK_SPACE:
                case SDLK_ESCAPE:
                case SDLK_RETURN:
                        states::pop();
                        break;

                default:
                        break;
                }
        } else {
                const auto action =
                        m_browser.read(
                                input,
                                MenuInputMode::scrolling_and_letters);

                switch (action) {
                case MenuAction::moved:
                        break;

                case MenuAction::esc:
                case MenuAction::space:
                        *m_menu_choice_result = (int)m_menu_choices.size() - 1;
                        states::pop();
                        break;

                case MenuAction::selected:
                        *m_menu_choice_result = m_browser.y();

                        TRACE
                                << "*m_menu_choice_result: "
                                << *m_menu_choice_result
                                << std::endl;

                        states::pop();
                        break;

                case MenuAction::left:
                case MenuAction::right:
                case MenuAction::none:
                        break;
                }
        }
}

StateId PopupState::id() const
{
        return StateId::popup;
}

} // namespace popup
