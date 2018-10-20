#include "character_descr.hpp"

#include <algorithm>

#include "player_bon.hpp"
#include "actor_player.hpp"
#include "io.hpp"
#include "text_format.hpp"
#include "item_potion.hpp"
#include "item_scroll.hpp"
#include "item_factory.hpp"
#include "item.hpp"
#include "map.hpp"
#include "game.hpp"
#include "property_factory.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static const std::string offset = "   ";

// TODO: Magic number, this should be based on the minimum required
// window width instead (e.g. 3/4 of this width)
static const int max_w_descr = 60;

static Color clr_heading()
{
        return colors::light_white();
}

static Color color_text_dark()
{
        return colors::gray();
}

static void add_properties_descr(std::vector<ColoredString>& lines)
{
        lines.push_back(ColoredString("Current properties", clr_heading()));

        const auto prop_list =
                map::player->properties.property_names_and_descr();

        if (prop_list.empty())
        {
                lines.push_back(
                        ColoredString(
                                offset + "None",
                                colors::text()));

                lines.push_back(ColoredString("", colors::text()));
        }
        else // Has properties
        {
                for (const auto& e : prop_list)
                {
                        const auto& title = e.title;

                        lines.push_back(
                                ColoredString(offset + title.str,
                                              title.color));

                        const auto descr_formatted =
                                text_format::split(e.descr, max_w_descr);

                        for (const auto& descr_line : descr_formatted)
                        {
                                lines.push_back(
                                        ColoredString(offset + descr_line,
                                                      color_text_dark()));
                        }

                        lines.push_back(ColoredString("", colors::text()));
                }
        }
}

static void add_insanity_descr(std::vector<ColoredString>& lines)
{
        lines.push_back(ColoredString("Mental disorders", clr_heading()));

        const std::vector<const InsSympt*> sympts = insanity::active_sympts();

        if (sympts.empty())
        {
                lines.push_back(
                        ColoredString(offset + "None",
                                      colors::text()));
        }
        else // Has insanity symptoms
        {
                for (const InsSympt* const sympt : sympts)
                {
                        const std::string sympt_descr = sympt->char_descr_msg();

                        if (!sympt_descr.empty())
                        {
                                lines.push_back(
                                        ColoredString(offset + sympt_descr,
                                                      colors::text()));
                        }
                }
        }

        lines.push_back(ColoredString("", colors::text()));
}

static void add_potion_descr(std::vector<ColoredString>& lines)
{
        lines.push_back(
                ColoredString("Potion knowledge",
                              clr_heading()));

        std::vector<ColoredString> potion_list;

        for (int i = 0; i < (int)ItemId::END; ++i)
        {
                const ItemData& d = item_data::data[i];

                if ((d.type != ItemType::potion) ||
                    (!d.is_tried &&
                     !d.is_identified))
                {
                        continue;
                }

                Item* item = item_factory::make(d.id);

                const std::string name =
                        item->name(ItemRefType::plain);

                potion_list.push_back(
                        ColoredString(offset + name, d.color));

                delete item;
        }

        if (potion_list.empty())
        {
                lines.push_back(
                        ColoredString(offset + "No known potions",
                                      colors::text()));
        }
        else
        {
                sort(potion_list.begin(),
                     potion_list.end(),
                     [](const ColoredString& e1,
                        const ColoredString& e2) {
                             return e1.str < e2.str;
                     });

                for (ColoredString& e : potion_list)
                {
                        lines.push_back(e);
                }
        }

        lines.push_back(ColoredString("", colors::text()));
}

static void add_scroll_descr(std::vector<ColoredString>& lines)
{
        lines.push_back(ColoredString("Manuscript knowledge", clr_heading()));

        std::vector<ColoredString> manuscript_list;

        for (int i = 0; i < (int)ItemId::END; ++i)
        {
                const ItemData& d = item_data::data[i];

                if ((d.type != ItemType::scroll) ||
                    (!d.is_tried &&
                     !d.is_identified))
                {
                        continue;
                }

                Item* item = item_factory::make(d.id);

                const std::string name =
                        item->name(ItemRefType::plain);

                manuscript_list.push_back(
                        ColoredString(offset + name,
                                      item->interface_color()));

                delete item;
        }

        if (manuscript_list.empty())
        {
                lines.push_back(
                        ColoredString(offset + "No known manuscripts",
                                      colors::text()));
        }
        else
        {
                sort(manuscript_list.begin(),
                     manuscript_list.end(),
                     [](const ColoredString& e1,
                        const ColoredString& e2) {
                             return e1.str < e2.str;
                     });

                for (ColoredString& e : manuscript_list)
                {
                        lines.push_back(e);
                }
        }

        lines.push_back(ColoredString("", colors::text()));
}

static void add_traits_descr(std::vector<ColoredString>& lines)
{
        lines.push_back(ColoredString("Traits gained", clr_heading()));

        for (size_t i = 0; i < (size_t)Trait::END; ++i)
        {
                if (player_bon::traits[i])
                {
                        const Trait trait = Trait(i);

                        const std::string title =
                                player_bon::trait_title(trait);

                        const std::string descr =
                                player_bon::trait_descr(trait);

                        lines.push_back(
                                ColoredString(offset + title,
                                              colors::text()));

                        const auto descr_lines =
                                text_format::split(descr,
                                                   max_w_descr);

                        for (const std::string& descr_line : descr_lines)
                        {
                                lines.push_back(
                                        ColoredString(offset + descr_line,
                                                      color_text_dark()));
                        }

                        lines.push_back(ColoredString("", colors::text()));
                }
        }
}

static void add_history_descr(std::vector<ColoredString>& lines)
{
        lines.push_back(
                ColoredString("History of " + map::player->name_the(),
                              clr_heading()));

        const std::vector<HistoryEvent>& events = game::history();

        int longest_turn_w = 0;

        for (const auto& event : events)
        {
                const int turn_w = std::to_string(event.turn).size();

                longest_turn_w = std::max(turn_w, longest_turn_w);
        }

        for (const auto& event : events)
        {
                std::string ev_str = std::to_string(event.turn);

                const int turn_w = ev_str.size();

                ev_str.append(longest_turn_w - turn_w, ' ');

                ev_str += " " + event.msg;

                lines.push_back(
                        ColoredString(offset + ev_str,
                                      colors::text()));
        }

        lines.push_back(ColoredString("", colors::text()));
}

// -----------------------------------------------------------------------------
// Character description
// -----------------------------------------------------------------------------
StateId CharacterDescr::id()
{
        return StateId::descript;
}

void CharacterDescr::on_start()
{
        lines_.clear();

        add_properties_descr(lines_);

        add_insanity_descr(lines_);

        add_potion_descr(lines_);

        add_scroll_descr(lines_);

        add_traits_descr(lines_);

        add_history_descr(lines_);
}

void CharacterDescr::draw()
{
        draw_interface();

        int y_pos = 1;

        const int nr_lines_tot = lines_.size();

        int btm_nr = std::min(
                top_idx_ + max_nr_lines_on_screen() - 1,
                nr_lines_tot - 1);

        for (int i = top_idx_; i <= btm_nr; ++i)
        {
                const ColoredString& line = lines_[i];

                io::draw_text(line.str,
                              Panel::screen,
                              P(0, y_pos),
                              line.color);

                ++y_pos;
        }
}

void CharacterDescr::update()
{
        const int line_jump = 3;
        const int nr_lines_tot = lines_.size();

        const auto input = io::get();

        switch (input.key)
        {
        case '2':
        case SDLK_DOWN:
                top_idx_ += line_jump;

                if (nr_lines_tot <= max_nr_lines_on_screen())
                {
                        top_idx_ = 0;
                }
                else
                {
                        top_idx_ = std::min(
                                nr_lines_tot - max_nr_lines_on_screen(),
                                top_idx_);
                }
                break;

        case '8':
        case SDLK_UP:
                top_idx_ = std::max(0, top_idx_ - line_jump);
                break;

        case SDLK_SPACE:
        case SDLK_ESCAPE:
                // Exit screen
                states::pop();
                break;

        default:
                break;
        }
}
