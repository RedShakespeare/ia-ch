// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "map_mode_gui.hpp"

#include "actor_player.hpp"
#include "colors.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_device.hpp"
#include "map.hpp"
#include "panel.hpp"
#include "player_bon.hpp"
#include "property_handler.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static Color label_color()
{
        return colors::text();
}

static Color info_color()
{
        return colors::text();
}

static void draw_bar(
        int pct_filled,
        const P& pos,
        const int w_tot,
        const Panel panel,
        const Color& color)
{
        if (pct_filled <= 0) {
                return;
        }

        pct_filled = std::min(pct_filled, 100);

        const auto px_x0y0 =
                io::gui_to_px_coords(panel, pos)
                        .with_offsets(0, 3);

        const auto px_x1y1 =
                io::gui_to_px_coords(
                        panel,
                        {pos.x + w_tot, pos.y + 1})
                        .with_offsets(-1, -3);

        const int px_w_tot = px_x1y1.x - px_x0y0.x + 1;
        const int px_w_filled = (px_w_tot * pct_filled) / 100;
        const int px_x1_filled = px_x0y0.x + px_w_filled - 1;
        const P px_x1y1_filled(px_x1_filled, px_x1y1.y);

        io::draw_rectangle_filled(
                {px_x0y0, px_x1y1_filled},
                color);
}

static void draw_wielded_wpn(const int x, const Panel panel)
{
        const std::string label = "Weapon:";

        io::draw_text(
                label,
                panel,
                {x, 0},
                label_color(),
                io::DrawBg::no);

        const auto* wpn = map::g_player->m_inv.item_in_slot(SlotId::wpn);

        if (!wpn) {
                wpn = &map::g_player->unarmed_wpn();
        }

        std::string wpn_str =
                text_format::first_to_upper(
                        wpn->name(
                                ItemRefType::plain,
                                ItemRefInf::yes,
                                ItemRefAttInf::wpn_main_att_mode));

        io::draw_text(
                wpn_str,
                panel,
                {x + (int)label.length() + 1, 0},
                info_color(),
                io::DrawBg::no);
}

static void draw_alt_wpn(const int x, const Panel panel)
{
        const std::string label = "Alt:";

        io::draw_text(
                label,
                panel,
                {x, 0},
                label_color().fraction(2.0),
                io::DrawBg::no);

        const auto* wpn = map::g_player->m_inv.item_in_slot(SlotId::wpn_alt);

        if (!wpn) {
                wpn = &map::g_player->unarmed_wpn();
        }

        std::string wpn_str =
                text_format::first_to_upper(
                        wpn->name(
                                ItemRefType::plain,
                                ItemRefInf::yes,
                                ItemRefAttInf::wpn_main_att_mode));

        io::draw_text(
                wpn_str,
                panel,
                {x + (int)label.length() + 1, 0},
                info_color().fraction(2.0),
                io::DrawBg::no);
}

static void draw_hp(const int y, const Panel panel)
{
        const int hp = map::g_player->m_hp;
        const int max_hp = actor::max_hp(*map::g_player);

        const int hp_pct = (hp * 100) / max_hp;

        draw_bar(
                hp_pct,
                {0, y},
                panels::w(panel),
                panel,
                colors::red().fraction(1.5));

        io::draw_text(
                "Health:",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const std::string str =
                std::to_string(hp) +
                "/" +
                std::to_string(max_hp);

        io::draw_text_right(
                str,
                panel,
                {panels::w(panel) - 1, y},
                info_color(),
                io::DrawBg::no);
}

static void draw_sp(const int y, const Panel panel)
{
        const int sp = map::g_player->m_sp;
        const int max_sp = actor::max_sp(*map::g_player);

        const int sp_pct = (sp * 100) / max_sp;

        draw_bar(
                sp_pct,
                {0, y},
                panels::w(panel),
                panel,
                colors::blue().fraction(1.25));

        io::draw_text(
                "Spirit:",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const std::string str =
                std::to_string(sp) +
                "/" +
                std::to_string(max_sp);

        io::draw_text_right(
                str,
                panel,
                {panels::w(panel) - 1, y},
                info_color(),
                io::DrawBg::no);
}

static void draw_shock(const int y, const Panel panel)
{
        const int shock_pct = std::min(999, map::g_player->shock_tot());

        draw_bar(
                shock_pct,
                {0, y},
                panels::w(panel),
                panel,
                colors::magenta().fraction(1.25));

        io::draw_text(
                "Shock:",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const std::string shock_str = std::to_string(shock_pct) + "%";

        io::draw_text_right(
                shock_str,
                panel,
                {panels::w(panel) - 1, y},
                info_color(),
                io::DrawBg::no);
}

static void draw_insanity(const int y, const Panel panel)
{
        const int ins_pct = map::g_player->ins();

        draw_bar(
                ins_pct,
                {0, y},
                panels::w(panel),
                panel,
                colors::magenta().fraction(1.25));

        io::draw_text(
                "Insanity:",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const std::string ins_str = std::to_string(ins_pct) + "%";

        io::draw_text_right(
                ins_str,
                panel,
                {panels::w(panel) - 1, y},
                info_color(),
                io::DrawBg::no);
}

static void draw_weight(const int y, const Panel panel)
{
        const int weight_pct = map::g_player->enc_percent();

        draw_bar(
                weight_pct,
                {0, y},
                panels::w(panel),
                panel,
                colors::cyan().fraction(2.0));

        io::draw_text(
                "Weight:",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const std::string enc_str = std::to_string(weight_pct) + "%";

        io::draw_text_right(
                enc_str,
                panel,
                {panels::w(panel) - 1, y},
                info_color(),
                io::DrawBg::no);
}

static void draw_armor(const int y, const Panel panel)
{
        io::draw_text(
                "Armor:",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const std::string armor_str =
                std::to_string(map::g_player->armor_points());

        io::draw_text_right(
                armor_str,
                panel,
                {panels::w(panel) - 1, y},
                info_color(),
                io::DrawBg::no);
}

static void draw_name(const int y, const Panel panel)
{
        io::draw_text(
                map::g_player->name_the(),
                panel,
                {0, y},
                colors::title(),
                io::DrawBg::no);
}

static void draw_class(const int y, const Panel panel)
{
        std::string bg_title;

        const auto bg = player_bon::bg();

        if (bg == Bg::occultist) {
                const auto domain = player_bon::occultist_domain();

                bg_title = player_bon::occultist_profession_title(domain);
        } else {
                bg_title = player_bon::bg_title(bg);
        }

        io::draw_text(
                bg_title,
                panel,
                {0, y},
                colors::title(),
                io::DrawBg::no);
}

static void draw_char_lvl_and_xp(const int y, const Panel panel)
{
        const int xp_pct = game::xp_pct();

        draw_bar(
                xp_pct,
                {0, y},
                panels::w(panel),
                panel,
                colors::green().fraction(1.25));

        io::draw_text(
                "Level:",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        std::string xp_str = std::to_string(game::clvl());

        if (xp_pct < 100) {
                xp_str +=
                        " (" +
                        std::to_string(game::xp_pct()) +
                        "%)";
        }

        io::draw_text_right(
                xp_str,
                panel,
                {panels::w(panel) - 1, y},
                info_color(),
                io::DrawBg::no);
}

static void draw_dlvl(const int y, const Panel panel)
{
        io::draw_text(
                "Depth:",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        std::string dlvl_str = std::to_string(map::g_dlvl);

        io::draw_text_right(
                dlvl_str,
                panel,
                {panels::w(panel) - 1, y},
                info_color(),
                io::DrawBg::no);
}

static void draw_lantern(const int y, const Panel panel)
{
        io::draw_text(
                "Lantern:",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const auto* const item =
                map::g_player->m_inv.item_in_backpack(item::Id::lantern);

        Color color = info_color();

        std::string lantern_str = "None";

        if (item) {
                const auto* const lantern =
                        static_cast<const device::Lantern*>(item);

                if (lantern->is_activated) {
                        color = colors::yellow();
                }

                lantern_str = std::to_string(lantern->nr_turns_left);
        }

        io::draw_text_right(
                lantern_str,
                panel,
                {panels::w(panel) - 1, y},
                color,
                io::DrawBg::no);
}

static void draw_med_suppl(const int y, const Panel panel)
{
        io::draw_text(
                "Med Suppl:",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        std::string suppl_str = "-";

        const auto* const item =
                map::g_player->m_inv.item_in_backpack(item::Id::medical_bag);

        if (item) {
                const auto* const medical_bag =
                        static_cast<const item::MedicalBag*>(item);

                suppl_str = std::to_string(medical_bag->m_nr_supplies);
        }

        io::draw_text_right(
                suppl_str,
                panel,
                {panels::w(panel) - 1, y},
                info_color(),
                io::DrawBg::no);
}

static void draw_properties(int y, const Panel panel)
{
        const auto property_names =
                map::g_player->m_properties.property_names_short();

        for (const auto& name : property_names) {
                if (y == panels::y1(panel)) {
                        break;
                }

                io::draw_text(
                        name.str,
                        panel,
                        // TODO: Make a separate property panel
                        {panels::x0(Panel::map_gui_wpn), y},
                        name.color,
                        io::DrawBg::yes,
                        colors::black());

                ++y;
        }
}

// -----------------------------------------------------------------------------
// map_mode_gui
// -----------------------------------------------------------------------------
namespace map_mode_gui {

void draw()
{
        io::cover_panel(Panel::map_gui_cond, colors::black());

        // Weapon panel
        auto panel = Panel::map_gui_wpn;
        int x = 0;

        draw_wielded_wpn(x, panel);

        x = 45;

        draw_alt_wpn(x, panel);

        // Condition panel
        int y = 0;
        panel = Panel::map_gui_cond;

        draw_hp(y++, panel);
        draw_sp(y++, panel);
        draw_shock(y++, panel);
        draw_insanity(y++, panel);
        draw_weight(y++, panel);
        draw_armor(y++, panel);

        // Progress panel
        y = 0;
        panel = Panel::map_gui_progress;

        draw_name(y++, panel);
        draw_class(y++, panel);
        draw_char_lvl_and_xp(y++, panel);
        draw_dlvl(y++, panel);
        draw_lantern(y++, panel);
        draw_med_suppl(y++, panel);

        // Properties
        panel = Panel::map;
        y = 1;
        draw_properties(y, panel);

        // Turn number
        // const int turn_nr = game_time::turn_nr();
        // const std::string turn_nr_str = std::to_string(turn_nr);

        // "T:" + current turn number
        // P p(0, 0);
        // io::draw_text("T", Panel::screen, p, colors::dark_gray());
        // ++p.x;
        // io::draw_text(":", Panel::screen, p, colors::dark_gray());
        // ++p.x;
        // io::draw_text(turn_nr_str, Panel::screen, p, colors::white());
}

} // namespace map_mode_gui
