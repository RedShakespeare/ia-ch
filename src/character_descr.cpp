// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "character_descr.hpp"

#include <algorithm>

#include "actor_player.hpp"
#include "game.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_factory.hpp"
#include "item_potion.hpp"
#include "item_scroll.hpp"
#include "map.hpp"
#include "player_bon.hpp"
#include "property_factory.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static const std::string s_offset = "   ";

// TODO: Magic number, this should be based on the minimum required
// window width instead (e.g. 3/4 of this width)
static const int s_max_w_descr = 60;


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
        lines.emplace_back("Current properties", clr_heading());

        const auto prop_list =
                map::g_player->m_properties.property_names_and_descr();

        if (prop_list.empty())
        {
                lines.emplace_back(
                                s_offset + "None",
                                colors::text());

                lines.emplace_back("", colors::text());
        }
        else // Has properties
        {
                for (const auto& e : prop_list)
                {
                        const auto& title = e.title;

                        lines.emplace_back(s_offset + title.str,
                                              title.color);

                        const auto descr_formatted =
                                text_format::split(e.descr, s_max_w_descr);

                        for (const auto& descr_line : descr_formatted)
                        {
                                lines.emplace_back(s_offset + descr_line,
                                                      color_text_dark());
                        }

                        lines.emplace_back("", colors::text());
                }
        }
}

static void add_insanity_descr(std::vector<ColoredString>& lines)
{
        lines.emplace_back("Mental disorders", clr_heading());

        const std::vector<const InsSympt*> sympts = insanity::active_sympts();

        if (sympts.empty())
        {
                lines.emplace_back(s_offset + "None",
                                      colors::text());
        }
        else // Has insanity symptoms
        {
                for (const InsSympt* const sympt : sympts)
                {
                        const std::string sympt_descr = sympt->char_descr_msg();

                        if (!sympt_descr.empty())
                        {
                                lines.emplace_back(s_offset + sympt_descr,
                                                      colors::text());
                        }
                }
        }

        lines.emplace_back("", colors::text());
}

static void add_potion_descr(std::vector<ColoredString>& lines)
{
        lines.emplace_back("Potion knowledge",
                              clr_heading());

        std::vector<ColoredString> potion_list;

        for (int i = 0; i < (int)item::Id::END; ++i)
        {
                const auto& d = item::g_data[i];

                if ((d.type != ItemType::potion) ||
                    (!d.is_tried &&
                     !d.is_identified))
                {
                        continue;
                }

                auto* item = item::make(d.id);

                const std::string name =
                        item->name(ItemRefType::plain);

                potion_list.emplace_back(s_offset + name, d.color);

                delete item;
        }

        if (potion_list.empty())
        {
                lines.emplace_back(s_offset + "No known potions",
                                      colors::text());
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

        lines.emplace_back("", colors::text());
}

static void add_scroll_descr(std::vector<ColoredString>& lines)
{
        lines.emplace_back("Manuscript knowledge", clr_heading());

        std::vector<ColoredString> manuscript_list;

        for (int i = 0; i < (int)item::Id::END; ++i)
        {
                const auto& d = item::g_data[i];

                if ((d.type != ItemType::scroll) ||
                    (!d.is_tried &&
                     !d.is_identified))
                {
                        continue;
                }

                auto* item = item::make(d.id);

                const std::string name =
                        item->name(ItemRefType::plain);

                manuscript_list.emplace_back(
                                s_offset + name,
                                item->interface_color());

                delete item;
        }

        if (manuscript_list.empty())
        {
                lines.emplace_back(
                                s_offset + "No known manuscripts",
                                colors::text());
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

        lines.emplace_back("", colors::text());
}

static void add_traits_descr(std::vector<ColoredString>& lines)
{
        lines.emplace_back("Traits gained", clr_heading());

        for (size_t i = 0; i < (size_t)Trait::END; ++i)
        {
                if (player_bon::has_trait((Trait)i))
                {
                        const auto trait = Trait(i);

                        const std::string title =
                                player_bon::trait_title(trait);

                        const std::string descr =
                                player_bon::trait_descr(trait);

                        lines.emplace_back(s_offset + title,
                                              colors::text());

                        const auto descr_lines =
                                text_format::split(descr, s_max_w_descr);

                        for (const std::string& descr_line : descr_lines)
                        {
                                lines.emplace_back(s_offset + descr_line,
                                                      color_text_dark());
                        }

                        lines.emplace_back("", colors::text());
                }
        }
}

static void add_history_descr(std::vector<ColoredString>& lines)
{
        lines.emplace_back("History of " + map::g_player->name_the(),
                              clr_heading());

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

                lines.emplace_back(s_offset + ev_str,
                                      colors::text());
        }

        lines.emplace_back("", colors::text());
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
        m_lines.clear();

        add_properties_descr(m_lines);

        add_insanity_descr(m_lines);

        add_potion_descr(m_lines);

        add_scroll_descr(m_lines);

        add_traits_descr(m_lines);

        add_history_descr(m_lines);
}

void CharacterDescr::draw()
{
        draw_interface();

        int y_pos = 1;

        const int nr_lines_tot = m_lines.size();

        int btm_nr = std::min(
                m_top_idx + max_nr_lines_on_screen() - 1,
                nr_lines_tot - 1);

        for (int i = m_top_idx; i <= btm_nr; ++i)
        {
                const ColoredString& line = m_lines[i];

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
        const int nr_lines_tot = m_lines.size();

        const auto input = io::get();

        switch (input.key)
        {
        case SDLK_KP_2:
        case SDLK_DOWN:
                m_top_idx += line_jump;

                if (nr_lines_tot <= max_nr_lines_on_screen())
                {
                        m_top_idx = 0;
                }
                else
                {
                        m_top_idx = std::min(
                                nr_lines_tot - max_nr_lines_on_screen(),
                                m_top_idx);
                }
                break;

        case SDLK_KP_8:
        case SDLK_UP:
                m_top_idx = std::max(0, m_top_idx - line_jump);
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
