// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "map_mode_gui.hpp"

#include "actor_player.hpp"
#include "colors.hpp"
#include "draw_box.hpp"
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
        return colors::dark_sepia();
}

static Color info_color()
{
        return colors::text();
}

// static void draw_bar(
//         int pct_filled,
//         const P& pos,
//         const int w_tot,
//         const Panel panel,
//         const Color& color)
// {
//         if (pct_filled <= 0) {
//                 return;
//         }

//         pct_filled = std::min(pct_filled, 100);

//         const auto px_x0y0 =
//                 io::gui_to_px_coords(panel, pos)
//                         .with_offsets(0, 3);

//         const auto px_x1y1 =
//                 io::gui_to_px_coords(
//                         panel,
//                         {pos.x + w_tot, pos.y + 1})
//                         .with_offsets(-1, -3);

//         const int px_w_tot = px_x1y1.x - px_x0y0.x + 1;
//         const int px_w_filled = (px_w_tot * pct_filled) / 100;
//         const int px_x1_filled = px_x0y0.x + px_w_filled - 1;
//         const P px_x1y1_filled(px_x1_filled, px_x1y1.y);

//         io::draw_rectangle_filled(
//                 {px_x0y0, px_x1y1_filled},
//                 color);
// }

static std::string make_wpn_dmg_str(const item::Item& wpn)
{
        const ItemRefAttInf att_inf =
                (wpn.data().main_att_mode == AttMode::thrown)
                ? ItemRefAttInf::melee
                : ItemRefAttInf::wpn_main_att_mode;

        const auto wpn_dmg_str =
                wpn.dmg_str(
                        att_inf,
                        ItemRefDmg::average_and_melee_plus);

        const auto wpn_hit_mod_str = wpn.hit_mod_str(att_inf);

        const auto wpn_inf_str = wpn.name_inf_str();

        std::string wpn_str;

        text_format::append_with_space(wpn_str, wpn_dmg_str);
        text_format::append_with_space(wpn_str, wpn_hit_mod_str);
        text_format::append_with_space(wpn_str, wpn_inf_str);

        return wpn_str;
}

static void draw_wielded_wpn(const int y, const Panel panel)
{
        const std::string label = "Wpn";

        io::draw_text(
                label,
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const auto* item = map::g_player->m_inv.item_in_slot(SlotId::wpn);

        if (!item) {
                item = &map::g_player->unarmed_wpn();
        }

        const auto wpn_str = make_wpn_dmg_str(*item);

        auto color = info_color();

        if (item->data().ranged.is_ranged_wpn &&
            !item->data().ranged.has_infinite_ammo &&
            (item->data().ranged.max_ammo > 0)) {
                const auto* const wpn = static_cast<const item::Wpn*>(item);

                if (wpn->m_ammo_loaded == 0) {
                        color = colors::yellow();
                }
        }

        io::draw_text_right(
                wpn_str,
                panel,
                {panels::w(panel) - 1, y},
                color,
                io::DrawBg::no);
}

static void draw_alt_wpn(const int y, const Panel panel)
{
        const std::string label = "Alt";

        io::draw_text(
                label,
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const auto* item = map::g_player->m_inv.item_in_slot(SlotId::wpn_alt);

        if (!item) {
                item = &map::g_player->unarmed_wpn();
        }

        const auto wpn_str = make_wpn_dmg_str(*item);

        auto color = info_color();

        if (item->data().ranged.is_ranged_wpn &&
            !item->data().ranged.has_infinite_ammo &&
            (item->data().ranged.max_ammo > 0)) {
                const auto* const wpn = static_cast<const item::Wpn*>(item);

                if (wpn->m_ammo_loaded == 0) {
                        color = colors::yellow();
                }
        }

        io::draw_text_right(
                wpn_str,
                panel,
                {panels::w(panel) - 1, y},
                color.fraction(2.0),
                io::DrawBg::no);
}

static void draw_hp(const int y, const Panel panel)
{
        const auto hp = map::g_player->m_hp;
        const auto max_hp = actor::max_hp(*map::g_player);

        const auto hp_pct = (hp * 100) / max_hp;

        // draw_bar(
        //         hp_pct,
        //         {0, y},
        //         panels::w(panel),
        //         panel,
        //         colors::red().fraction(1.5));

        io::draw_text(
                "Health",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const auto hp_str = std::to_string(hp);
        const auto max_hp_str = std::to_string(max_hp);

        const auto hp_str_w = (int)hp_str.length();
        const auto max_hp_str_w = (int)max_hp_str.length();
        const auto tot_w = hp_str_w + 1 + max_hp_str_w;
        const auto hp_x = panels::w(panel) - tot_w;

        const auto tint_pct = std::min(100 - hp_pct, 60);

        io::draw_text(
                hp_str,
                panel,
                {hp_x, y},
                colors::light_red().tinted(tint_pct),
                io::DrawBg::no);

        io::draw_text(
                "/" + max_hp_str,
                panel,
                {hp_x + hp_str_w, y},
                colors::light_red(),
                io::DrawBg::no);
}

static void draw_sp(const int y, const Panel panel)
{
        const auto sp = map::g_player->m_sp;
        const auto max_sp = actor::max_sp(*map::g_player);

        const auto sp_pct = (sp * 100) / max_sp;

        // draw_bar(
        //         sp_pct,
        //         {0, y},
        //         panels::w(panel),
        //         panel,
        //         colors::blue().fraction(1.5));

        io::draw_text(
                "Health",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const auto sp_str = std::to_string(sp);
        const auto max_sp_str = std::to_string(max_sp);

        const auto sp_str_w = (int)sp_str.length();
        const auto max_sp_str_w = (int)max_sp_str.length();
        const auto tot_w = sp_str_w + 1 + max_sp_str_w;
        const auto sp_x = panels::w(panel) - tot_w;

        const auto tint_pct = std::min(100 - sp_pct, 60);

        io::draw_text(
                sp_str,
                panel,
                {sp_x, y},
                colors::light_blue().tinted(tint_pct),
                io::DrawBg::no);

        io::draw_text(
                "/" + max_sp_str,
                panel,
                {sp_x + sp_str_w, y},
                colors::light_blue(),
                io::DrawBg::no);
}

static void draw_shock(const int y, const Panel panel)
{
        const auto shock_pct = std::min(999, map::g_player->shock_tot());

        // draw_bar(
        //         shock_pct,
        //         {0, y},
        //         panels::w(panel),
        //         panel,
        //         colors::magenta().fraction(2));

        io::draw_text(
                "Shock",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const auto shock_str = std::to_string(shock_pct) + "%";

        io::draw_text_right(
                shock_str,
                panel,
                {panels::w(panel) - 1, y},
                colors::magenta(),
                io::DrawBg::no);
}

static void draw_insanity(const int y, const Panel panel)
{
        const auto ins_pct = map::g_player->ins();

        // draw_bar(
        //         ins_pct,
        //         {0, y},
        //         panels::w(panel),
        //         panel,
        //         colors::magenta().fraction(2));

        io::draw_text(
                "Insanity",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const auto ins_str = std::to_string(ins_pct) + "%";

        io::draw_text_right(
                ins_str,
                panel,
                {panels::w(panel) - 1, y},
                colors::magenta().fraction(1.5),
                io::DrawBg::no);
}

static void draw_weight(const int y, const Panel panel)
{
        const auto weight_pct = map::g_player->enc_percent();

        // draw_bar(
        //         weight_pct,
        //         {0, y},
        //         panels::w(panel),
        //         panel,
        //         colors::cyan().fraction(2.0));

        io::draw_text(
                "Weight",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const auto enc_str = std::to_string(weight_pct) + "%";

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
                "Armor",
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
        io::draw_text_center(
                map::g_player->name_the(),
                panel,
                {panels::w(panel) / 2, y},
                colors::light_sepia(),
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

        io::draw_text_center(
                bg_title,
                panel,
                {panels::w(panel) / 2, y},
                colors::light_sepia(),
                io::DrawBg::no);
}

static void draw_char_lvl_and_xp(const int y, const Panel panel)
{
        const auto clvl = game::clvl();

        const bool is_max_lvl = clvl >= g_player_max_clvl;

        const int xp_pct = std::clamp(game::xp_pct(), 0, 100);

        // draw_bar(
        //         xp_pct,
        //         {0, y},
        //         panels::w(panel),
        //         panel,
        //         colors::green().fraction(1.25));

        io::draw_text(
                "Level",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const auto clvl_str = std::to_string(clvl);

        std::string xp_str;

        if (!is_max_lvl) {
                xp_str = " (" + std::to_string(xp_pct) + "%)";
        }

        const auto clvl_w = (int)clvl_str.length();
        const auto xp_w = (int)xp_str.length();
        const auto tot_w = clvl_w + xp_w;
        const auto clvl_x = panels::w(panel) - tot_w;

        io::draw_text(
                clvl_str,
                panel,
                {clvl_x, y},
                info_color(),
                io::DrawBg::no);

        if (!is_max_lvl) {
                io::draw_text(
                        xp_str,
                        panel,
                        {clvl_x + clvl_w, y},
                        colors::green().tinted(100 - xp_pct),
                        io::DrawBg::no);
        }
}

static void draw_dlvl(const int y, const Panel panel)
{
        io::draw_text(
                "Depth",
                panel,
                {0, y},
                label_color(),
                io::DrawBg::no);

        const auto dlvl = (int)map::g_dlvl;

        const auto dlvl_str = std::to_string(dlvl);

        const auto max_dlvl = g_dlvl_last;
        const auto dlvl_pct = std::clamp((dlvl * 100) / max_dlvl, 0, 100);
        const auto shade_pct = (dlvl_pct * 5) / 8;

        io::draw_text_right(
                dlvl_str,
                panel,
                {panels::w(panel) - 1, y},
                info_color().shaded(shade_pct),
                io::DrawBg::no);
}

static void draw_lantern(const int y, const Panel panel)
{
        io::draw_text(
                "Lantern",
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
                "Med Suppl",
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
                        {0, y},
                        name.color,
                        io::DrawBg::no,
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
        io::cover_panel(
                Panel::map_gui_stats_border,
                colors::extra_dark_gray());

        draw_box(panels::area(Panel::map_gui_stats_border));

        const auto panel = Panel::map_gui_stats;

        int y = 0;

        draw_name(y++, panel);
        draw_class(y++, panel);
        draw_char_lvl_and_xp(y++, panel);
        draw_dlvl(y++, panel);
        draw_hp(y++, panel);
        draw_sp(y++, panel);
        draw_shock(y++, panel);
        draw_insanity(y++, panel);

        ++y;

        draw_wielded_wpn(y++, panel);
        draw_alt_wpn(y++, panel);

        ++y;

        draw_lantern(y++, panel);
        draw_med_suppl(y++, panel);
        draw_armor(y++, panel);
        draw_weight(y++, panel);

        ++y;

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
