// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "create_character.hpp"

#include "actor_player.hpp"
#include "browser.hpp"
#include "draw_box.hpp"
#include "game.hpp"
#include "global.hpp"
#include "init.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_travel.hpp"
#include "msg_log.hpp"
#include "panel.hpp"
#include "popup.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// New game state
// -----------------------------------------------------------------------------
void NewGameState::on_pushed()
{
        states::push(std::make_unique<GameState>(GameEntryMode::new_game));
        states::push(std::make_unique<EnterNameState>());
        states::push(std::make_unique<PickTraitState>());
        states::push(std::make_unique<PickBgState>());
}

void NewGameState::on_resume()
{
        states::pop();
}

// -----------------------------------------------------------------------------
// Pick background state
// -----------------------------------------------------------------------------
void PickBgState::on_start()
{
        m_bgs = player_bon::pickable_bgs();

        m_browser.reset(
                m_bgs.size(),
                panels::h(Panel::create_char_menu));

        // Set the marker on war veteran, to recommend it as a default choice
        // for new players
        const auto war_vet_pos =
                std::find(std::begin(m_bgs), std::end(m_bgs), Bg::war_vet);

        const auto idx =
                (int)std::distance(std::begin(m_bgs), war_vet_pos);

        m_browser.set_y(idx);
}

void PickBgState::update()
{
        if (config::is_bot_playing()) {
                player_bon::pick_bg(rnd::element(m_bgs));

                ASSERT(player_bon::bg() != Bg::END);

                if (player_bon::bg() == Bg::occultist) {
                        player_bon::pick_occultist_domain(
                                (OccultistDomain)rnd::range(
                                        0,
                                        (int)OccultistDomain::END - 1));
                }

                states::pop();

                return;
        }

        const auto input = io::get();

        const auto action =
                m_browser.read(
                        input,
                        MenuInputMode::scrolling_and_letters);

        switch (action) {
        case MenuAction::selected: {
                const auto bg = m_bgs[m_browser.y()];

                player_bon::pick_bg(bg);

                // Increase clvl to 1
                game::incr_clvl_number();

                states::pop();

                // Occultists also pick a domain
                if (bg == Bg::occultist) {
                        states::push(std::make_unique<PickOccultistState>());
                }
        } break;

        case MenuAction::esc: {
                states::pop_until(StateId::menu);
        } break;

        default:
                break;
        }
}

void PickBgState::draw()
{
        draw_box(panels::area(Panel::screen));

        const int screen_center_x = panels::center_x(Panel::screen);

        io::draw_text_center(
                " What is your background? ",
                Panel::screen,
                P(screen_center_x, 0),
                colors::title(),
                io::DrawBg::yes,
                colors::black(),
                true); // Allow pixel-level adjustment

        int y = 0;

        const auto bg_marked = m_bgs[m_browser.y()];

        // Backgrounds
        for (const auto bg : m_bgs) {
                const auto key_str =
                        m_browser.menu_keys()[y] +
                        std::string {") "};

                const std::string bg_name = player_bon::bg_title(bg);
                const bool is_marked = bg == bg_marked;

                const auto& draw_color =
                        is_marked
                        ? colors::menu_highlight()
                        : colors::menu_dark();

                io::draw_text(
                        key_str + bg_name,
                        Panel::create_char_menu,
                        P(0, y),
                        draw_color);

                ++y;
        }

        // Description
        y = 0;

        const auto descr = player_bon::bg_descr(bg_marked);

        ASSERT(!descr.empty());

        for (const auto& descr_entry : descr) {
                if (descr_entry.str.empty()) {
                        ++y;

                        continue;
                }

                const auto formatted_lines = text_format::split(
                        descr_entry.str,
                        panels::w(Panel::create_char_descr));

                for (const std::string& line : formatted_lines) {
                        io::draw_text(
                                line,
                                Panel::create_char_descr,
                                P(0, y),
                                descr_entry.color);

                        ++y;
                }
        }
}

// -----------------------------------------------------------------------------
// Pick occultist state
// -----------------------------------------------------------------------------
void PickOccultistState::on_start()
{
        m_domains = player_bon::pickable_occultist_domains();

        m_browser.reset(
                m_domains.size(),
                panels::h(Panel::create_char_menu));
}

void PickOccultistState::update()
{
        if (config::is_bot_playing()) {
                player_bon::pick_occultist_domain(rnd::element(m_domains));

                states::pop();

                return;
        }

        const auto input = io::get();

        const auto action =
                m_browser.read(
                        input,
                        MenuInputMode::scrolling_and_letters);

        switch (action) {
        case MenuAction::selected: {
                const auto domain = m_domains[m_browser.y()];

                player_bon::pick_occultist_domain(domain);

                states::pop();
        } break;

        case MenuAction::esc: {
                states::pop_until(StateId::menu);
        } break;

        default:
                break;
        }
}

void PickOccultistState::draw()
{
        draw_box(panels::area(Panel::screen));

        const int screen_center_x = panels::center_x(Panel::screen);

        io::draw_text_center(
                " What is your spell domain? ",
                Panel::screen,
                P(screen_center_x, 0),
                colors::title(),
                io::DrawBg::yes,
                colors::black(),
                true); // Allow pixel-level adjustment

        int y = 0;

        const auto domain_marked = m_domains[m_browser.y()];

        // Domains
        for (const auto domain : m_domains) {
                const auto key_str =
                        m_browser.menu_keys()[y] +
                        std::string {") "};

                const std::string domain_name =
                        player_bon::spell_domain_title(domain);

                const bool is_marked = domain == domain_marked;

                const auto& draw_color =
                        is_marked
                        ? colors::menu_highlight()
                        : colors::menu_dark();

                io::draw_text(
                        key_str + domain_name,
                        Panel::create_char_menu,
                        P(0, y),
                        draw_color);

                ++y;
        }

        // Description
        y = 0;

        const auto descr = player_bon::occultist_domain_descr(domain_marked);

        ASSERT(!descr.empty());

        if (descr.empty()) {
                return;
        }

        const auto formatted_lines = text_format::split(
                descr,
                panels::w(Panel::create_char_descr));

        for (const std::string& line : formatted_lines) {
                io::draw_text(
                        line,
                        Panel::create_char_descr,
                        P(0, y),
                        colors::text());

                ++y;
        }
}

// -----------------------------------------------------------------------------
// Pick trait state
// -----------------------------------------------------------------------------
void PickTraitState::on_start()
{
        const auto player_bg = player_bon::bg();

        player_bon::unpicked_traits_for_bg(
                player_bg,
                m_traits_avail,
                m_traits_unavail);

        const int choices_h = panels::h(Panel::create_char_menu);

        m_browser_traits_avail.reset(m_traits_avail.size(), choices_h);

        m_browser_traits_unavail.reset(m_traits_unavail.size(), choices_h);
}

void PickTraitState::update()
{
        if (config::is_bot_playing()) {
                states::pop();

                return;
        }

        const auto input = io::get();

        // Switch trait screen mode?
        if (input.key == SDLK_TAB) {
                m_screen_mode =
                        (m_screen_mode == TraitScreenMode::pick_new)
                        ? TraitScreenMode::view_unavail
                        : TraitScreenMode::pick_new;

                return;
        }

        MenuBrowser& browser =
                (m_screen_mode == TraitScreenMode::pick_new)
                ? m_browser_traits_avail
                : m_browser_traits_unavail;

        const auto action =
                browser.read(
                        input,
                        MenuInputMode::scrolling_and_letters);

        switch (action) {
        case MenuAction::selected: {
                if (m_screen_mode == TraitScreenMode::pick_new) {
                        const Trait trait = m_traits_avail[browser.y()];

                        const std::string name =
                                player_bon::trait_title(trait);

                        bool should_pick_trait = true;

                        const bool is_character_creation =
                                states::contains_state(StateId::pick_name);

                        if (!is_character_creation) {
                                states::draw();

                                const auto result =
                                        popup::menu(
                                                "",
                                                {"Yes", "No"},
                                                "Gain trait \"" + name + "\"?");

                                should_pick_trait = (result == 0);
                        }

                        if (should_pick_trait) {
                                player_bon::pick_trait(trait);

                                if (!is_character_creation) {
                                        game::add_history_event(
                                                "Gained trait \"" +
                                                name +
                                                "\"");
                                }

                                states::pop();
                        }
                }
        } break;

        case MenuAction::esc: {
                if (states::contains_state(StateId::pick_name)) {
                        states::pop_until(StateId::menu);
                }
        }

        default:
                break;
        }
}

void PickTraitState::draw()
{
        draw_box(panels::area(Panel::screen));

        std::string title;

        if (m_screen_mode == TraitScreenMode::pick_new) {
                title =
                        states::contains_state(StateId::pick_name)
                        ? "Which extra trait do you start with?"
                        : "Which trait do you gain?";

                title += " [TAB] to view unavailable traits";
        } else {
                // Viewing unavailable traits
                title = "Currently unavailable traits";

                title += " [TAB] to view available traits";
        }

        const int screen_center_x = panels::center_x(Panel::screen);

        io::draw_text_center(
                " " + title + " ",
                Panel::screen,
                P(screen_center_x, 0),
                colors::title(),
                io::DrawBg::yes,
                colors::black(),
                true);

        MenuBrowser* browser;

        std::vector<Trait>* traits;

        if (m_screen_mode == TraitScreenMode::pick_new) {
                browser = &m_browser_traits_avail;

                traits = &m_traits_avail;
        } else {
                // Viewing unavailable traits
                browser = &m_browser_traits_unavail;

                traits = &m_traits_unavail;
        }

        const int browser_y = browser->y();

        const Trait trait_marked = traits->at(browser_y);

        const auto player_bg = player_bon::bg();

        int y = 0;

        const Range idx_range_shown = browser->range_shown();

        // Traits
        for (int i = idx_range_shown.min; i <= idx_range_shown.max; ++i) {
                const auto key_str =
                        browser->menu_keys()[y] +
                        std::string {") "};

                const Trait trait = traits->at(i);

                std::string trait_name = player_bon::trait_title(trait);

                const bool is_idx_marked = browser_y == i;

                Color color = colors::light_magenta();

                if (m_screen_mode == TraitScreenMode::pick_new) {
                        if (is_idx_marked) {
                                color = colors::menu_highlight();
                        } else {
                                // Not marked
                                color = colors::menu_dark();
                        }
                } else {
                        // Viewing unavailable traits
                        if (is_idx_marked) {
                                color = colors::light_red();
                        } else {
                                // Not marked
                                color = colors::red();
                        }
                }

                io::draw_text(
                        key_str + trait_name,
                        Panel::create_char_menu,
                        P(0, y),
                        color);

                ++y;
        }

        // Draw "more" labels
        // if (!browser->is_on_top_page())
        // {
        //         io::draw_text(
        //                 "(More - Page Up)",
        //                 Panel::screen,
        //                 P(opt_x0_, top_more_y_),
        //                 colors::light_white());
        // }

        // if (!browser->is_on_btm_page())
        // {
        //         io::draw_text(
        //                 "(More - Page Down)",
        //                 Panel::screen,
        //                 P(opt_x0_, btm_more_y_),
        //                 colors::light_white());
        // }

        // Description
        y = 0;

        std::string descr = player_bon::trait_descr(trait_marked);

        const auto formatted_descr =
                text_format::split(
                        descr,
                        panels::w(Panel::create_char_descr));

        for (const std::string& str : formatted_descr) {
                io::draw_text(
                        str,
                        Panel::create_char_descr,
                        P(0, y),
                        colors::text());
                ++y;
        }

        // Prerequisites
        std::vector<Trait> trait_marked_prereqs;

        Bg trait_marked_bg_prereq = Bg::END;

        player_bon::trait_prereqs(
                trait_marked,
                player_bg,
                trait_marked_prereqs,
                trait_marked_bg_prereq);

        const int y0_prereqs = 10;

        y = y0_prereqs;

        if (!trait_marked_prereqs.empty() ||
            trait_marked_bg_prereq != Bg::END) {
                int x = 0;

                const std::string label = "Prerequisite(s):";

                io::draw_text(
                        label,
                        Panel::create_char_descr,
                        P(x, y),
                        colors::text());

                x += label.length() + 1;

                std::vector<ColoredString> prereq_titles;

                const Color& clr_prereq_ok = colors::green();
                const Color& clr_prereq_not_ok = colors::red();

                if (trait_marked_bg_prereq != Bg::END) {
                        const auto& color =
                                (player_bon::bg() == trait_marked_bg_prereq)
                                ? clr_prereq_ok
                                : clr_prereq_not_ok;

                        const std::string bg_title =
                                player_bon::bg_title(trait_marked_bg_prereq);

                        prereq_titles.emplace_back(bg_title, color);
                }

                for (const auto prereq_trait : trait_marked_prereqs) {
                        const bool is_picked =
                                player_bon::has_trait(prereq_trait);

                        const auto& color =
                                is_picked
                                ? clr_prereq_ok
                                : clr_prereq_not_ok;

                        const std::string trait_title =
                                player_bon::trait_title(prereq_trait);

                        prereq_titles.emplace_back(trait_title, color);
                }

                const size_t nr_prereq_titles = prereq_titles.size();
                for (size_t prereq_idx = 0;
                     prereq_idx < nr_prereq_titles;
                     ++prereq_idx) {
                        const auto& prereq_title = prereq_titles[prereq_idx];

                        io::draw_text(
                                prereq_title.str,
                                Panel::create_char_descr,
                                P(x, y),
                                prereq_title.color);

                        x += prereq_title.str.length();

                        if (prereq_idx < (nr_prereq_titles - 1)) {
                                io::draw_text(
                                        ",",
                                        Panel::create_char_descr,
                                        P(x, y),
                                        colors::text());

                                ++x;
                        }

                        ++x;
                }
        }
}

// -----------------------------------------------------------------------------
// Enter name state
// -----------------------------------------------------------------------------
void EnterNameState::on_start()
{
        const std::string default_name = config::default_player_name();

        m_current_str = default_name;
}

void EnterNameState::update()
{
        if (config::is_bot_playing()) {
                auto& d = *map::g_player->m_data;

                d.name_a = d.name_the = "Bot";

                states::pop();

                return;
        }

        const auto input = io::get();

        if (input.key == SDLK_ESCAPE) {
                states::pop_until(StateId::menu);
                return;
        }

        if (input.key == SDLK_RETURN) {
                if (m_current_str.empty()) {
                        m_current_str = "Player";
                } else {
                        // Player has entered a string
                        config::set_default_player_name(m_current_str);
                }

                auto& d = *map::g_player->m_data;

                d.name_a = d.name_the = m_current_str;

                states::pop();

                return;
        }

        if (m_current_str.size() < g_player_name_max_len) {
                const bool is_space = input.key == SDLK_SPACE;

                if (is_space) {
                        m_current_str.push_back(' ');

                        return;
                }

                const bool is_valid_non_space_char =
                        (input.key >= 'a' && input.key <= 'z') ||
                        (input.key >= 'A' && input.key <= 'Z') ||
                        (input.key >= '0' && input.key <= '9');

                if (is_valid_non_space_char) {
                        m_current_str.push_back(input.key);

                        return;
                }
        }

        if (!m_current_str.empty()) {
                if (input.key == SDLK_BACKSPACE) {
                        m_current_str.erase(m_current_str.end() - 1);
                }
        }
}

void EnterNameState::draw()
{
        draw_box(panels::area(Panel::screen));

        const int screen_center_x = panels::center_x(Panel::screen);

        io::draw_text_center(
                " What is your name? ",
                Panel::screen,
                P(screen_center_x, 0),
                colors::title(),
                io::DrawBg::yes);

        const int y_name = 3;

        const std::string name_str =
                (m_current_str.size() < g_player_name_max_len)
                ? m_current_str + "_"
                : m_current_str;

        const size_t name_x0 = screen_center_x - (g_player_name_max_len / 2);
        const size_t name_x1 = name_x0 + g_player_name_max_len - 1;

        io::draw_text(
                name_str,
                Panel::screen,
                P(name_x0, y_name),
                colors::menu_highlight());

        R box_rect(
                P((int)name_x0 - 1, y_name - 1),
                P((int)name_x1 + 1, y_name + 1));

        draw_box(box_rect);
}
