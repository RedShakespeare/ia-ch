// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "main_menu.hpp"

#include <string>

#include "actor.hpp"
#include "actor_player.hpp"
#include "audio.hpp"
#include "colors.hpp"
#include "create_character.hpp"
#include "draw_box.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "highscore.hpp"
#include "init.hpp"
#include "io.hpp"
#include "manual.hpp"
#include "map.hpp"
#include "map_travel.hpp"
#include "misc.hpp"
#include "panel.hpp"
#include "popup.hpp"
#include "saving.hpp"
#include "text_format.hpp"
#include "version.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::string s_git_sha1_str;

static std::string s_current_quote;

// TODO: This should be loaded from a text file
static const std::vector<std::string> s_quotes =
        {
                {"Happy is the tomb where no wizard hath lain and happy the town at night "
                 "whose wizards are all ashes."},

                {"Our means of receiving impressions are absurdly few, and our notions of "
                 "surrounding objects infinitely narrow. We see things only as we are "
                 "constructed to see them, and can gain no idea of their absolute nature."},

                {"I am writing this under an appreciable mental strain, since by tonight I "
                 "shall be no more..."},

                {"The end is near. I hear a noise at the door, as of some immense slippery "
                 "body lumbering against it. It shall not find me..."},

                {"Sometimes I believe that this less material life is our truer life, and "
                 "that our vain presence on the terraqueous globe is itself the secondary "
                 "or merely virtual phenomenon."},

                {"Science, already oppressive with its shocking revelations, will perhaps "
                 "be the ultimate exterminator of our human species, if separate species "
                 "we be, for its reserve of unguessed horrors could never be borne by "
                 "mortal brains if loosed upon the world...."},

                {"Madness rides the star-wind... claws and teeth sharpened on centuries of "
                 "corpses... dripping death astride a bacchanale of bats from nigh-black "
                 "ruins of buried temples of Belial..."},

                {"Memories and possibilities are ever more hideous than realities."},

                {"Yog-Sothoth knows the gate. Yog-Sothoth is the gate. Yog-Sothoth is the "
                 "key and guardian of the gate. Past, present, future, all are one in "
                 "Yog-Sothoth. He knows where the Old Ones broke through of old, and where "
                 "They shall break through again."},

                {"Slowly but inexorably crawling upon my consciousness and rising above "
                 "every other impression, came a dizzying fear of the unknown; not death, "
                 "but some nameless, unheard-of thing inexpressibly more ghastly and "
                 "abhorrent."},

                {"I felt that some horrible scene or object lurked beyond the silk-hung "
                 "walls, and shrank from glancing through the arched, latticed windows "
                 "that opened so bewilderingly on every hand."},

                {"There now ensued a series of incidents which transported me to the "
                 "opposite extremes of ecstasy and horror; incidents which I tremble to "
                 "recall and dare not seek to interpret..."},

                {"From the new-flooded lands it flowed again, uncovering death and decay; "
                 "and from its ancient and immemorial bed it trickled loathsomely, "
                 "uncovering nighted secrets of the years when Time was young and the gods "
                 "unborn."},

                {"The moon is dark, and the gods dance in the night; there is terror in "
                 "the sky, for upon the moon hath sunk an eclipse foretold in no books of "
                 "men or of earth's gods..."},

                {"May the merciful gods, if indeed there be such, guard those hours when "
                 "no power of the will can keep me from the chasm of sleep. With him who "
                 "has come back out of the nethermost chambers of night, haggard and "
                 "knowing, peace rests nevermore."},

                {"What I learned and saw in those hours of impious exploration can never "
                 "be told, for want of symbols or suggestions in any language."},

                {"The most merciful thing in the world, I think, is the inability of the "
                 "human mind to correlate all its contents."},

                {"In his house at R'lyeh dead Cthulhu waits dreaming."},

                {"Ph'nglui mglw'nafh Cthulhu R'lyeh wgah'nagl fhtagn"},

                {"They worshipped, so they said, the Great Old Ones who lived ages before "
                 "there were any men, and who came to the young world out of the sky..."},

                {"That is not dead which can eternal lie, and with strange aeons even "
                 "death may die."},

                {"I have looked upon all that the universe has to hold of horror, and even "
                 "the skies of spring and the flowers of summer must ever afterward be "
                 "poison to me. But I do not think my life will be long. I know too much, "
                 "and the cult still lives."},

                {"Something terrible came to the hills and valleys on that meteor, and "
                 "something terrible, though I know not in what proportion, still remains."},

                {"Man's respect for the imponderables varies according to his mental "
                 "constitution and environment. Through certain modes of thought and "
                 "training it can be elevated tremendously, yet there is always a limit."},

                {"The oldest and strongest emotion of mankind is fear, and the oldest and "
                 "strongest kind of fear is fear of the unknown."},

                {"I have seen the dark universe yawning, where the black planets roll "
                 "without aim, where they roll in their horror unheeded, without "
                 "knowledge, or lustre, or name."},

                {"Searchers after horror haunt strange, far places."},

                {"The sciences have hitherto harmed us little; but some day the piecing "
                 "together of dissociated knowledge will open up such terrifying vistas of "
                 "reality, that we shall either go mad from the revelation or flee from "
                 "the deadly light into the peace and safety of a new dark age."},

                {"There are horrors beyond life's edge that we do not suspect, and once in "
                 "a while man's evil prying calls them just within our range."},

                {"We live on a placid island of ignorance in the midst of black seas of "
                 "infinity, and it was not meant that we should voyage far."},

                {"There are black zones of shadow close to our daily paths, and now and "
                 "then some evil soul breaks a passage through. When that happens, the man "
                 "who knows must strike before reckoning the consequences."},

                {"Non-Euclidean calculus and quantum physics are enough to stretch any "
                 "brain; and when one mixes them with folklore, and tries to trace a "
                 "strange background of multi-dimensional reality behind the ghoulish "
                 "hints of Gothic tales and the wild whispers of the chimney-corner, one "
                 "can hardly expect to be wholly free from mental tension."},

                {"I could not help feeling that they were evil things-- mountains of "
                 "madness whose farther slopes looked out over some accursed ultimate "
                 "abyss."},

                {"That seething, half luminous cloud background held ineffable suggestions "
                 "of a vague, ethereal beyondness far more than terrestrially spatial; "
                 "and gave appalling reminders of the utter remoteness, separateness, "
                 "desolation, and aeon-long death of this untrodden and unfathomed austral "
                 "world."},

                {"With five feeble senses we pretend to comprehend the boundlessly complex "
                 "cosmos, yet other beings might not only see very differently, but might "
                 "see and study whole worlds of matter, energy, and life which lie close "
                 "at hand yet can never be detected with the senses we have."},

                {"It is absolutely necessary, for the peace and safety of mankind, that "
                 "some of earth's dark, dead corners and unplumbed depths be left alone; "
                 "lest sleeping abnormalities wake to resurgent life, and blasphemously "
                 "surviving nightmares squirm and splash out of their black lairs to newer "
                 "and wider conquests."},

                {"I felt myself on the edge of the world; peering over the rim into a "
                 "fathomless chaos of eternal night."},

                {"And where Nyarlathotep went, rest vanished, for the small hours were "
                 "rent with the screams of nightmare."},

                {"It was just a color out of space - a frightful messenger from unformed "
                 "realms of infinity beyond all Nature as we know it; from realms whose "
                 "mere existence stuns the brain and numbs us with the black extra-cosmic "
                 "gulfs it throws open before our frenzied eyes."},

                {"It lumbered slobberingly into sight and gropingly squeezed its "
                 "gelatinous green immensity through the black doorway into the tainted "
                 "outside air of that poison city of madness."},

                {"The Thing cannot be described - there is no language for such abysms of "
                 "shrieking and immemorial lunacy, such eldritch contradictions of all "
                 "matter, force, and cosmic order."},

                {"I could tell I was at the gateway of a region half-bewitched through "
                 "the piling-up of unbroken time-accumulations; a region where old, "
                 "strange things have had a chance to grow and linger because they have "
                 "never been stirred up."}};

static bool query_overwrite_savefile()
{
        int choice;

        popup::Popup(popup::AddToMsgHistory::no)
                .set_title("A saved game exists")
                .set_msg("Start a new game?")
                .set_menu(
                        {"(Y)es", "(N)o"},
                        {'y', 'n'},
                        &choice)
                .run();

        return (choice == 0);
}

// -----------------------------------------------------------------------------
// Main menu state
// -----------------------------------------------------------------------------
MainMenuState::MainMenuState() :
        m_browser(MenuBrowser(6))
{
        m_browser.set_custom_menu_keys({'n', 'r', 't', 'o', 'g', 'e'});
}

MainMenuState::~MainMenuState() = default;

StateId MainMenuState::id() const
{
        return StateId::main_menu;
}

void MainMenuState::draw()
{
        io::clear_screen();

        if (config::is_tiles_mode()) {
                draw_box(panels::area(Panel::screen));
        }

        if (config::is_tiles_mode()) {
                io::draw_logo();
        } else {
                // Text mode
                io::draw_text_center(
                        "I n f r a   A r c a n a",
                        Panel::screen,
                        {panels::center_x(Panel::screen),
                         (panels::h(Panel::screen) * 3) / 12},
                        colors::gold());
        }

        if (config::is_gj_mode()) {
                io::draw_text(
                        "### GJ MODE ENABLED ###",
                        Panel::screen,
                        {1, 1},
                        colors::yellow());
        }
#ifndef NDEBUG
        else {
                io::draw_text(
                        "### DEBUG ###",
                        Panel::screen,
                        {1, 1},
                        colors::yellow());

                io::draw_text(
                        "###  MODE ###",
                        Panel::screen,
                        {1, 2},
                        colors::yellow());
        }
#endif // NDEBUG

        const std::vector<std::string> labels = {
                "(N)ew journey",
                "(R)esurrect",
                "(T)ome of Wisdom",
                "(O)ptions",
                "(G)raveyard",
                "(E)scape to reality"};

        const P screen_dims = panels::dims(Panel::screen);

        // NOTE: The main menu logo is 311 pixels high, we place the main meny
        // at least at this height, plus a little more
        // TODO: Read the logo height programmatically instead of hard coding it
        const P menu_pos(
                (screen_dims.x * 13) / 20,
                std::max(
                        (320 / config::gui_cell_px_h()),
                        ((screen_dims.y * 4) / 10)));

        P pos = menu_pos;

        for (size_t i = 0; i < labels.size(); ++i) {
                const std::string label = labels[i];

                const bool is_marked = m_browser.is_at_idx(i);

                auto str = label.substr(0, 3);

                auto color =
                        is_marked
                        ? colors::menu_key_highlight()
                        : colors::menu_key_dark();

                io::draw_text(str, Panel::screen, pos, color);

                str = label.substr(3, std::string::npos);

                color =
                        is_marked
                        ? colors::menu_highlight()
                        : colors::menu_dark();

                io::draw_text(str, Panel::screen, pos.with_x_offset(3), color);

                // ++pos.x;
                ++pos.y;
        }

        const Color quote_clr = colors::gray_brown().fraction(1.75);

        std::vector<std::string> quote_lines;

        int quote_w = 45;

        // Decrease quote width until we find a width that doesn't leave a
        // "tiny" string on the last line (looks very ugly),
        while (quote_w != 0) {
                quote_lines =
                        text_format::split(
                                "\"" + s_current_quote + "\"",
                                quote_w);

                const size_t min_str_w_last_line = 20;

                const std::string& last_line = quote_lines.back();

                // Is the length of the current last line at least as long as
                // the minimum required?
                if (last_line.length() >= min_str_w_last_line) {
                        break;
                }

                --quote_w;
        }

        if (quote_w > 0) {
                int quote_y;

                if (quote_lines.size() < (labels.size() - 1)) {
                        quote_y = menu_pos.y + 1;
                } else if (quote_lines.size() > (labels.size() + 1)) {
                        quote_y = menu_pos.y - 1;
                } else {
                        // Number of quote lines is within +/- 1 difference from
                        // number of main menu labels
                        quote_y = menu_pos.y;
                }

                pos.set(
                        std::max((quote_w / 2) + 2, (screen_dims.x * 3) / 10),
                        quote_y);

                for (const std::string& line : quote_lines) {
                        io::draw_text_center(
                                line,
                                Panel::screen,
                                pos,
                                quote_clr);

                        ++pos.y;
                }
        }

        std::string build_str;

        if (version_info::g_version_str.empty()) {
                build_str = "Build " + s_git_sha1_str;
        } else {
                build_str =
                        version_info::g_version_str +
                        " (" +
                        s_git_sha1_str +
                        ")";
        }

        build_str += ", " + version_info::g_date_str;

        io::draw_text_right(
                " " + build_str + " ",
                Panel::screen,
                {panels::x1(Panel::screen) - 1, 0},
                colors::gray());

        io::draw_text_center(
                std::string(
                        " " +
                        version_info::g_copyright_str +
                        ", " +
                        version_info::g_license_str +
                        " "),
                Panel::screen,
                {panels::center_x(Panel::screen), panels::y1(Panel::screen)},
                colors::gray());

        io::update_screen();

} // draw

void MainMenuState::update()
{
        const auto input = io::get();

        const MenuAction action =
                m_browser.read(
                        input,
                        MenuInputMode::scrolling_and_letters);

        switch (action) {
        case MenuAction::selected:
                switch (m_browser.y()) {
                case 0: {
#ifndef NDEBUG
                        if (!config::is_bot_playing())
#endif // NDEBUG
                        {
                                if (saving::is_save_available()) {
                                        const bool should_proceed =
                                                query_overwrite_savefile();

                                        if (!should_proceed) {
                                                return;
                                        }
                                }
                        }

                        audio::fade_out_music();

                        init::init_session();

                        auto new_game_state = std::make_unique<NewGameState>();

                        states::push(std::move(new_game_state));
                } break;

                case 1: {
                        // Load game
                        if (saving::is_save_available()) {
                                audio::fade_out_music();

                                init::init_session();

                                saving::load_game();

                                auto game_state = std::make_unique<GameState>(
                                        GameEntryMode::load_game);

                                states::push(std::move(game_state));
                        } else {
                                // No save available
                                popup::Popup(popup::AddToMsgHistory::no)
                                        .set_msg("No saved game found")
                                        .run();
                        }
                } break;

                case 2: {
                        // Manual
                        std::unique_ptr<State> browse_manual_state(
                                new BrowseManual);

                        states::push(std::move(browse_manual_state));
                } break;

                case 3: {
                        // Options
                        std::unique_ptr<State> config_state(new ConfigState);

                        states::push(std::move(config_state));
                } break;

                case 4: {
                        // Highscores
                        std::unique_ptr<State> browse_highscore_state(
                                new BrowseHighscore);

                        states::push(std::move(browse_highscore_state));
                } break;

                case 5: {
                        // Exit
                        states::pop();
                } break;

                } // switch
                break;

        default:
                break;

        } // switch
} // update

void MainMenuState::on_start()
{
        s_git_sha1_str = version_info::read_git_sha1_str_from_file();

        s_current_quote = rnd::element(s_quotes);

        audio::play_music(audio::MusId::cthulhiana_madness);
}

void MainMenuState::on_resume()
{
        audio::play_music(audio::MusId::cthulhiana_madness);
}
