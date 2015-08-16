#include "create_character.hpp"

#include "render.hpp"
#include "actor_player.hpp"
#include "input.hpp"
#include "menu_input_handling.hpp"
#include "text_format.hpp"
#include "utils.hpp"
#include "map.hpp"

namespace create_character
{

const int OPT_X0    = 1;
const int OPT_Y0    = 2;
const int OPT_X1    = 22;
const int OPT_Y1    = SCREEN_H - 2;
const int OPT_H     = OPT_Y1 - OPT_Y0 + 1;

namespace
{

const int DESCR_X0  = OPT_X1 + 2;
const int DESCR_Y0  = OPT_Y0;
const int DESCR_X1  = SCREEN_W - 1;
const int DESCR_Y1  = OPT_Y1;

const int DESCR_W   = DESCR_X1 - DESCR_Y1 + 1;

void draw_opt_box()
{
    const Rect r(OPT_X0 - 1, OPT_Y0 - 1, OPT_X1 + 1, OPT_Y1 + 1);

    render::draw_box(r);
}

namespace enter_name
{

void draw(const std::string& cur_string)
{
    render::clear_screen();

    render::draw_box(Rect(Pos(0, 0), Pos(SCREEN_W - 1, SCREEN_H - 1)));

    render::draw_text_centered("What is your name?", Panel::screen,
                               Pos(MAP_W_HALF, 0), clr_white);
    const int Y_NAME = 2;

    const std::string NAME_STR = cur_string.size() < PLAYER_NAME_MAX_LEN ?
                                 cur_string + "_" : cur_string;

    const size_t NAME_X0 = MAP_W_HALF - (PLAYER_NAME_MAX_LEN / 2);
    const size_t NAME_X1 = NAME_X0 + PLAYER_NAME_MAX_LEN - 1;

    render::draw_text(NAME_STR, Panel::screen, Pos(NAME_X0, Y_NAME), clr_menu_highlight);

    Rect box_rect(Pos(NAME_X0 - 1, Y_NAME - 1), Pos(NAME_X1 + 1, Y_NAME + 1));

    render::draw_box(box_rect);

    render::update_screen();
}

void read_keys(std::string& cur_string, bool& is_done)
{
    const Key_data& d = input::input(false);

    if (d.sdl_key == SDLK_RETURN)
    {
        is_done = true;
        cur_string = cur_string.empty() ? "Player" : cur_string;
        return;
    }

    if (cur_string.size() < PLAYER_NAME_MAX_LEN)
    {
        if (
            d.sdl_key == SDLK_SPACE ||
            (d.key >= int('a') && d.key <= int('z')) ||
            (d.key >= int('A') && d.key <= int('Z')) ||
            (d.key >= int('0') && d.key <= int('9')))
        {
            if (d.sdl_key == SDLK_SPACE)
            {
                cur_string.push_back(' ');
            }
            else
            {
                cur_string.push_back(char(d.key));
            }

            draw(cur_string);
            return;
        }
    }

    if (cur_string.size() > 0)
    {
        if (d.sdl_key == SDLK_BACKSPACE)
        {
            cur_string.erase(cur_string.end() - 1);
            draw(cur_string);
        }
    }
}

void run()
{
    std::string name = "";
    draw(name);
    bool is_done = false;

    while (!is_done)
    {
        if (config::is_bot_playing())
        {
            name = "AZATHOTH";
            is_done = true;
        }
        else
        {
            read_keys(name, is_done);
        }
    }

    Actor_data_t& def = map::player->data();
    def.name_a      = def.name_the = name;
}

} //Enter_name

void draw_pick_bg(const std::vector<Bg>& bgs, const Menu_browser& browser)
{
    render::clear_screen();

    draw_opt_box();

    render::draw_text_centered("Choose your background", Panel::screen,
                               Pos(MAP_W_HALF, 0), clr_white, clr_black, true);

    int y = OPT_Y0;

    const Bg bg_marked = bgs[browser.y()];

    //------------------------------------------------------------- BACKGROUNDS
    for (const Bg bg : bgs)
    {
        const std::string   bg_name     = player_bon::bg_title(bg);
        const bool          IS_MARKED   = bg == bg_marked;
        const Clr&          drw_clr     = IS_MARKED ? clr_menu_highlight : clr_menu_drk;

        render::draw_text(bg_name, Panel::screen, Pos(OPT_X0, y), drw_clr);
        ++y;
    }

    //------------------------------------------------------------- DESCRIPTION
    y = DESCR_Y0;

    std::vector<std::string> raw_descr_lines;

    player_bon::bg_descr(bg_marked, raw_descr_lines);

    for (const std::string& raw_line : raw_descr_lines)
    {
        std::vector<std::string> formatted_lines;

        text_format::line_to_lines(raw_line, DESCR_W, formatted_lines);

        for (const std::string& line : formatted_lines)
        {
            render::draw_text(line, Panel::screen, Pos(DESCR_X0, y), clr_white);
            y++;
        }
    }

    render::update_screen();
}

void pick_bg()
{
    if (config::is_bot_playing())
    {
        player_bon::pick_bg(Bg(rnd::range(0, int(Bg::END) - 1)));
    }
    else //Human playing
    {
        std::vector<Bg> bgs;
        player_bon::pickable_bgs(bgs);

        Menu_browser browser(bgs.size(), 0);

        draw_pick_bg(bgs, browser);

        while (true)
        {
            const Menu_action action = menu_input_handling::action(browser);

            switch (action)
            {
            case Menu_action::browsed:
                draw_pick_bg(bgs, browser);
                break;

            case Menu_action::selected:
                player_bon::pick_bg(bgs[browser.y()]);
                return;

            case Menu_action::esc:
            case Menu_action::space:
            case Menu_action::selected_shift:
                break;
            }
        }
    }
}

void draw_pick_trait(const std::vector<Trait>& traits,
                     const Menu_browser& browser,
                     const bool IS_CHARACTER_CREATION)
{
    render::clear_screen();

    draw_opt_box();

    std::string title = IS_CHARACTER_CREATION ?
                        "Which additional trait do you start with?" :
                        "You have reached a new level! Which trait do you gain?";

    render::draw_text_centered(title, Panel::screen, Pos(MAP_W_HALF, 0),
                               clr_white, clr_black, true);

    const Trait trait_marked = traits[browser.y()];

    //------------------------------------------------------------- TRAITS
    int y = OPT_Y0;

    for (const Trait trait : traits)
    {
        std::string     trait_name  = player_bon::trait_title(trait);
        const bool      IS_MARKED   = trait == trait_marked;
        const Clr&      drw_clr     = IS_MARKED ? clr_menu_highlight : clr_menu_drk;

        render::draw_text(trait_name, Panel::screen, Pos(OPT_X0, y), drw_clr);

        ++y;
    }

    //------------------------------------------------------------- DESCRIPTION
    y = DESCR_Y0;

    std::string descr = player_bon::trait_descr(trait_marked);

    std::vector<std::string> descr_lines;

    text_format::line_to_lines("Effect(s): " + descr, DESCR_W, descr_lines);

    for (const std::string& str : descr_lines)
    {
        render::draw_text(str, Panel::screen, Pos(DESCR_X0, y), clr_white);
        ++y;
    }

    //------------------------------------------------------------- PREREQUISITES
    const int Y0_PREREQS = 17;

    y = Y0_PREREQS;

    std::vector<Trait> trait_prereqs;

    Bg bg_prereq = Bg::END;

    player_bon::trait_prereqs(trait_marked, trait_prereqs, bg_prereq);

    if (!trait_prereqs.empty() || bg_prereq != Bg::END)
    {
        render::draw_text("This trait had the following prerequisite(s):",
                          Panel::screen, Pos(DESCR_X0, y), clr_white);
        ++y;

        std::string prereq_str = "";

        if (bg_prereq != Bg::END)
        {
            prereq_str = player_bon::bg_title(bg_prereq);
        }

        for (Trait prereq_trait : trait_prereqs)
        {
            std::string prereq_title = player_bon::trait_title(prereq_trait);

            prereq_str += (prereq_str.empty() ? "" : ", ") + prereq_title;
        }

        std::vector<std::string> prereq_lines;
        text_format::line_to_lines(prereq_str, DESCR_W, prereq_lines);

        for (const std::string& str : prereq_lines)
        {
            render::draw_text(str, Panel::screen, Pos(DESCR_X0, y), clr_white);
            ++y;
        }
    }

    //------------------------------------------------------------- PREVIOUS
    y = Y0_PREREQS + 4;

    std::string picked_str = player_bon::all_picked_traits_titles_line();

    if (picked_str != "")
    {
        picked_str = "Trait(s) gained: " + picked_str;

        std::vector<std::string> picked_lines;
        text_format::line_to_lines(picked_str, DESCR_W, picked_lines);

        for (const std::string& str : picked_lines)
        {
            render::draw_text(str, Panel::screen, Pos(DESCR_X0, y), clr_white);
            ++y;
        }
    }

    render::update_screen();
}

} //namespace

void create_character()
{
    pick_bg();
    pick_new_trait(true);
    enter_name::run();
}

void pick_new_trait(const bool IS_CHARACTER_CREATION)
{
    if (config::is_bot_playing())
    {
        return;
    }

    std::vector<Trait> pickable_traits;
    player_bon::pickable_traits(pickable_traits);

    if (!pickable_traits.empty())
    {
        Menu_browser browser(pickable_traits.size(), 0);

        draw_pick_trait(pickable_traits, browser, IS_CHARACTER_CREATION);

        while (true)
        {
            const Menu_action action = menu_input_handling::action(browser);

            switch (action)
            {
            case Menu_action::browsed:
                draw_pick_trait(pickable_traits, browser, IS_CHARACTER_CREATION);
                break;

            case Menu_action::selected:
                player_bon::pick_trait(pickable_traits[browser.y()]);

                if (!IS_CHARACTER_CREATION)
                {
                    render::draw_map_and_interface();
                }

                return;

            case Menu_action::esc:
            case Menu_action::space:
            case Menu_action::selected_shift:
                break;
            }
        }
    }
}

} //Create_character
