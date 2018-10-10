#include "create_character.hpp"

#include "actor_player.hpp"
#include "browser.hpp"
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
        bgs_ = player_bon::pickable_bgs();

        browser_.reset(
                bgs_.size(),
                panels::h(Panel::create_char_menu));
}

void PickBgState::update()
{
        if (config::is_bot_playing())
        {
                player_bon::pick_bg(rnd::element(bgs_));

                states::pop();

                return;
        }

        const auto input = io::get(false);

        const auto action =
                browser_.read(
                        input,
                        MenuInputMode::scrolling_and_letters);

        switch (action)
        {
        case MenuAction::selected:
        {
                const auto bg = bgs_[browser_.y()];

                player_bon::pick_bg(bg);

                states::pop();

                // Occultists also pick a domain
                if (bg == Bg::occultist)
                {
                        states::push(std::make_unique<PickOccultistState>());
                }
        }
        break;

        case MenuAction::esc:
        {
                states::pop_until(StateId::menu);
        }
        break;

        default:
                break;
        }
}

void PickBgState::draw()
{
        const int screen_center_x = panels::center_x(Panel::screen);

        io::draw_text_center(
                "What is your background?",
                Panel::screen,
                P(screen_center_x, 0),
                colors::title(),
                false, // Do not draw background color
                colors::black(),
                true); // Allow pixel-level adjustment

        int y = 0;

        const auto bg_marked = bgs_[browser_.y()];

        // Backgrounds
        std::string key_str = "a) ";

        for (const auto bg : bgs_)
        {
                const std::string bg_name = player_bon::bg_title(bg);
                const bool is_marked = bg == bg_marked;

                const auto& draw_color =
                        is_marked ?
                        colors::menu_highlight() :
                        colors::menu_dark();

                io::draw_text(
                        key_str + bg_name,
                        Panel::create_char_menu,
                        P(0, y),
                        draw_color);

                ++y;

                ++key_str[0];
        }

        // Description
        y = 0;

        const auto descr = player_bon::bg_descr(bg_marked);

        ASSERT(!descr.empty());

        for (const auto& descr_entry : descr)
        {
                if (descr_entry.str.empty())
                {
                        ++y;

                        continue;
                }

                const auto formatted_lines = text_format::split(
                        descr_entry.str,
                        panels::w(Panel::create_char_descr));

                for (const std::string& line : formatted_lines)
                {
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
        domains_ = player_bon::pickable_occultist_domains();

        browser_.reset(
                domains_.size(),
                panels::h(Panel::create_char_menu));
}

void PickOccultistState::update()
{
        if (config::is_bot_playing())
        {
                player_bon::pick_occultist_domain(rnd::element(domains_));

                states::pop();

                return;
        }

        const auto input = io::get(false);

        const auto action =
                browser_.read(
                        input,
                        MenuInputMode::scrolling_and_letters);

        switch (action)
        {
        case MenuAction::selected:
        {
                const auto domain = domains_[browser_.y()];

                player_bon::pick_occultist_domain(domain);

                states::pop();
        }
        break;

        case MenuAction::esc:
        {
                states::pop_until(StateId::menu);
        }
        break;

        default:
                break;
        }
}

void PickOccultistState::draw()
{
        const int screen_center_x = panels::center_x(Panel::screen);

        io::draw_text_center(
                "What is your spell domain?",
                Panel::screen,
                P(screen_center_x, 0),
                colors::title(),
                false, // Do not draw background color
                colors::black(),
                true); // Allow pixel-level adjustment

        int y = 0;

        const auto domain_marked = domains_[browser_.y()];

        // Domains
        std::string key_str = "a) ";

        for (const auto domain : domains_)
        {
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

                ++key_str[0];
        }

        // Description
        y = 0;

        const auto descr = player_bon::occultist_domain_descr(domain_marked);

        ASSERT(!descr.empty());

        if (descr.empty())
        {
                return;
        }

        const auto formatted_lines = text_format::split(
                descr,
                panels::w(Panel::create_char_descr));

        for (const std::string& line : formatted_lines)
        {
                io::draw_text(
                        line,
                        Panel::create_char_descr,
                        P(0, y),
                        colors::white());

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
                traits_avail_,
                traits_unavail_);

        const int choices_h = panels::h(Panel::create_char_menu);

        browser_traits_avail_.reset(traits_avail_.size(), choices_h);

        browser_traits_unavail_.reset(traits_unavail_.size(), choices_h);
}

void PickTraitState::update()
{
        if (config::is_bot_playing())
        {
                states::pop();

                return;
        }

        const auto input = io::get(false);

        // Switch trait screen mode?
        if (input.key == SDLK_TAB)
        {
                screen_mode_ =
                        (screen_mode_ == TraitScreenMode::pick_new)
                        ? TraitScreenMode::view_unavail
                        : TraitScreenMode::pick_new;

                return;
        }

        MenuBrowser& browser =
                (screen_mode_ == TraitScreenMode::pick_new)
                ? browser_traits_avail_
                : browser_traits_unavail_;

        const auto action =
                browser.read(
                        input,
                        MenuInputMode::scrolling_and_letters);

        switch (action)
        {
        case MenuAction::selected:
        {
                if (screen_mode_ == TraitScreenMode::pick_new)
                {
                        const Trait trait = traits_avail_[browser.y()];

                        player_bon::pick_trait(trait);

                        states::pop();
                }
        }
        break;

        case MenuAction::esc:
        {
                if (states::contains_state(StateId::pick_name))
                {
                        states::pop_until(StateId::menu);
                }
        }

        default:
                break;
        }
}

void PickTraitState::draw()
{
        std::string title;

        if (screen_mode_ == TraitScreenMode::pick_new)
        {
                title =
                        states::contains_state(StateId::pick_name)
                        ? "Which extra trait do you start with?"
                        : "Which trait do you gain?";

                title += " [TAB] to view unavailable traits";
        }
        else // Viewing unavailable traits
        {
                title = "Currently unavailable traits";

                title += " [TAB] to view available traits";
        }

        const int screen_center_x = panels::center_x(Panel::screen);

        io::draw_text_center(
                title,
                Panel::screen,
                P(screen_center_x, 0),
                colors::title(),
                false, // Do not draw background color
                colors::black(),
                true);

        MenuBrowser* browser;

        std::vector<Trait>* traits;

        if (screen_mode_ == TraitScreenMode::pick_new)
        {
                browser = &browser_traits_avail_;

                traits = &traits_avail_;
        }
        else // Viewing unavailable traits
        {
                browser = &browser_traits_unavail_;

                traits = &traits_unavail_;
        }

        const int browser_y = browser->y();

        const Trait trait_marked = traits->at(browser_y);

        const auto player_bg = player_bon::bg();

        // Traits
        int y = 0;

        const Range idx_range_shown = browser->range_shown();

        std::string key_str = "a) ";

        for (int i = idx_range_shown.min; i <= idx_range_shown.max; ++i)
        {
                const Trait trait = traits->at(i);

                std::string trait_name = player_bon::trait_title(trait);

                const bool is_idx_marked = browser_y == i;

                Color color = colors::light_magenta();

                if (screen_mode_ == TraitScreenMode::pick_new)
                {
                        if (is_idx_marked)
                        {
                                color = colors::menu_highlight();
                        }
                        else // Not marked
                        {
                                color = colors::menu_dark();
                        }
                }
                else // Viewing unavailable traits
                {
                        if (is_idx_marked)
                        {
                                color = colors::light_red();
                        }
                        else // Not marked
                        {
                                color = colors::red();
                        }
                }

                io::draw_text(
                        key_str + trait_name,
                        Panel::create_char_menu,
                        P(0, y),
                        color);

                ++y;
                ++key_str[0];
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

        for (const std::string& str : formatted_descr)
        {
                io::draw_text(
                        str,
                        Panel::create_char_descr,
                        P(0, y),
                        colors::white());
                ++y;
        }

        // Prerequisites
        std::vector<Trait> trait_marked_prereqs;

        Bg trait_marked_bg_prereq = Bg::END;

        int trait_marked_clvl_prereq = -1;

        player_bon::trait_prereqs(
                trait_marked,
                player_bg,
                trait_marked_prereqs,
                trait_marked_bg_prereq,
                trait_marked_clvl_prereq);

        const int y0_prereqs = 10;

        y = y0_prereqs;

        if (!trait_marked_prereqs.empty() ||
            trait_marked_bg_prereq != Bg::END)
        {
                const std::string label = "Prerequisite(s):";

                io::draw_text(
                        label,
                        Panel::create_char_descr,
                        P(0, y),
                        colors::white());

                std::vector<ColoredString> prereq_titles;

                std::string prereq_str = "";

                const Color& clr_prereq_ok = colors::green();

                const Color& clr_prereq_not_ok = colors::red();

                if (trait_marked_bg_prereq != Bg::END)
                {
                        const auto& color =
                                (player_bon::bg() == trait_marked_bg_prereq) ?
                                clr_prereq_ok :
                                clr_prereq_not_ok;

                        const std::string bg_title =
                                player_bon::bg_title(trait_marked_bg_prereq);

                        prereq_titles.push_back(ColoredString(bg_title, color));
                }

                for (Trait prereq_trait : trait_marked_prereqs)
                {
                        const bool is_picked =
                                player_bon::traits[(size_t)prereq_trait];

                        const auto& color =
                                is_picked ?
                                clr_prereq_ok :
                                clr_prereq_not_ok;

                        const std::string trait_title =
                                player_bon::trait_title(prereq_trait);

                        prereq_titles.push_back(
                                ColoredString(trait_title, color));
                }

                if (trait_marked_clvl_prereq != -1)
                {
                        const Color& color =
                                (game::clvl() >= trait_marked_clvl_prereq) ?
                                clr_prereq_ok :
                                clr_prereq_not_ok;

                        const std::string clvl_title =
                                "Character level " +
                                std::to_string(trait_marked_clvl_prereq);

                        prereq_titles.push_back(
                                ColoredString(clvl_title, color));
                }

                const int prereq_list_x = label.size() + 1;

                for (const ColoredString& title : prereq_titles)
                {
                        io::draw_text(
                                title.str,
                                Panel::create_char_descr,
                                P(prereq_list_x, y),
                                title.color);

                        ++y;
                }
        }
}

// -----------------------------------------------------------------------------
// Enter name state
// -----------------------------------------------------------------------------
void EnterNameState::on_start()
{
        const std::string default_name = config::default_player_name();

        current_str_ = default_name;
}

void EnterNameState::update()
{
        if (config::is_bot_playing())
        {
                ActorData& d = *map::player->data;

                d.name_a = d.name_the = "Bot";

                states::pop();

                return;
        }

        const auto input = io::get(false);

        if (input.key == SDLK_ESCAPE)
        {
                states::pop_until(StateId::menu);
                return;
        }

        if (input.key == SDLK_RETURN)
        {
                if (current_str_.empty())
                {
                        current_str_ = "Player";
                }
                else // Player has entered a string
                {
                        config::set_default_player_name(current_str_);
                }

                ActorData& d = *map::player->data;

                d.name_a = d.name_the = current_str_;

                states::pop();

                return;
        }

        if (current_str_.size() < player_name_max_len)
        {
                if (input.key == SDLK_SPACE)
                {
                        current_str_.push_back(' ');

                        return;
                }
                else if ((input.key >= 'a' && input.key <= 'z') ||
                         (input.key >= 'A' && input.key <= 'Z') ||
                         (input.key >= '0' && input.key <= '9'))
                {
                        current_str_.push_back(input.key);

                        return;
                }
        }

        if (!current_str_.empty())
        {
                if (input.key == SDLK_BACKSPACE)
                {
                        current_str_.erase(current_str_.end() - 1);
                }
        }
}

void EnterNameState::draw()
{
        const int screen_center_x = panels::center_x(Panel::screen);

        io::draw_text_center(
                "What is your name?",
                Panel::screen,
                P(screen_center_x, 0),
                colors::title());

        const int y_name = 3;

        const std::string name_str =
                (current_str_.size() < player_name_max_len) ?
                current_str_ + "_" :
                current_str_;

        const size_t name_x0 = screen_center_x - (player_name_max_len / 2);
        const size_t name_x1 = name_x0 + player_name_max_len - 1;

        io::draw_text(
                name_str,
                Panel::screen,
                P(name_x0, y_name),
                colors::menu_highlight());

        R box_rect(P(name_x0 - 1, y_name - 1),
                   P(name_x1 + 1, y_name + 1));

        io::draw_box(box_rect);
}
