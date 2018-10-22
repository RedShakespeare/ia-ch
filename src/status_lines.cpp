#include "status_lines.hpp"

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
static const Panel panel = Panel::player_stats;

static const bool draw_text_bg = false;

static const int text_x0 = 1;

static int text_x1()
{
        return panels::w(panel) - 2;
}

static Color label_color()
{
        return colors::dark_sepia();
}

static void draw_player_name(int& y)
{
        io::draw_text(
                map::player->name_the(),
                panel,
                P(text_x0, y),
                colors::light_sepia(),
                draw_text_bg);

        ++y;
}

static void draw_player_class(int& y)
{
        std::string bg_title;

        const auto bg = player_bon::bg();

        if (bg == Bg::occultist)
        {
                const auto domain = player_bon::occultist_domain();

                bg_title = player_bon::occultist_profession_title(domain);
        }
        else
        {
                bg_title = player_bon::bg_title(bg);
        }

        const auto class_lines =
                text_format::split(
                        bg_title,
                        text_x1() - text_x0 + 1);

        for (const std::string& line : class_lines)
        {
                io::draw_text(
                        line,
                        panel,
                        P(text_x0, y),
                        colors::light_sepia(),
                        draw_text_bg);

                ++y;
        }
}

static void draw_char_lvl_and_xp(int& y)
{
        io::draw_text(
                "Level",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        const std::string xp_str =
                std::to_string(game::clvl()) +
                " (" +
                std::to_string(game::xp_pct()) +
                "%)";

        io::draw_text_right(
                xp_str,
                panel,
                P(text_x1(), y),
                colors::white(),
                draw_text_bg);

        ++y;
}

static void draw_dlvl(int& y)
{
        io::draw_text(
                "Depth",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        std::string dlvl_str = std::to_string(map::dlvl);

        io::draw_text_right(
                dlvl_str,
                panel,
                P(text_x1(), y),
                colors::white(),
                draw_text_bg);

        ++y;
}

static void draw_hp(int& y)
{
        io::draw_text(
                "Health",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        const std::string str =
                std::to_string(map::player->hp) +
                "/" +
                std::to_string(actor::max_hp(*map::player));

        io::draw_text_right(
                str,
                panel,
                P(text_x1(), y),
                colors::light_red(),
                draw_text_bg);

        ++y;
}

static void draw_sp(int& y)
{
        io::draw_text(
                "Spirit",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        const std::string str =
                std::to_string(map::player->sp) +
                "/" +
                std::to_string(actor::max_sp(*map::player));

        io::draw_text_right(
                str,
                panel,
                P(text_x1(), y),
                colors::light_blue(),
                draw_text_bg);

        ++y;
}

static void draw_shock(int& y)
{
        io::draw_text(
                "Shock",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        const int shock = std::min(999, map::player->shock_tot());

        const std::string shock_str = std::to_string(shock) + "%";

        // const Color shock_color =
        //         shock < 50  ? colors::white() :
        //         shock < 75  ? colors::yellow() :
        //         shock < 100 ? colors::magenta() :
        //         colors::light_red();

        io::draw_text_right(
                shock_str,
                panel,
                P(text_x1(), y),
                colors::magenta() /* shock_color */,
                draw_text_bg);

        ++y;
}

static void draw_insanity(int& y)
{
        io::draw_text(
                "Insanity",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        const std::string ins_str = std::to_string(map::player->ins()) + "%";

        io::draw_text_right(
                ins_str,
                panel,
                P(text_x1(), y),
                colors::magenta(),
                draw_text_bg);

        ++y;
}

static void draw_wielded_wpn(int& y)
{
        io::draw_text(
                "Wpn",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        const Item* wpn = map::player->inv.item_in_slot(SlotId::wpn);

        if (!wpn)
        {
                wpn = &map::player->unarmed_wpn();
        }

        const ItemRefAttInf att_inf =
                (wpn->data().main_att_mode == AttMode::thrown)
                ? ItemRefAttInf::melee
                : ItemRefAttInf::wpn_main_att_mode;

        const std::string wpn_dmg_str =
                wpn->dmg_str(
                        att_inf,
                        ItemRefDmg::average_and_melee_plus);

        const std::string wpn_hit_mod_str = wpn->hit_mod_str(att_inf);

        const std::string wpn_inf_str = wpn->name_inf_str();

        std::string wpn_str = "";

        text_format::append_with_space(wpn_str, wpn_dmg_str);
        text_format::append_with_space(wpn_str, wpn_hit_mod_str);
        text_format::append_with_space(wpn_str, wpn_inf_str);

        io::draw_text_right(
                wpn_str,
                panel,
                P(text_x1(), y),
                colors::white(),
                draw_text_bg);

        ++y;
}

static void draw_alt_wpn(int& y)
{
        io::draw_text(
                "Alt",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        const Item* wpn = map::player->inv.item_in_slot(SlotId::wpn_alt);

        if (!wpn)
        {
                wpn = &map::player->unarmed_wpn();
        }

        const ItemRefAttInf att_inf =
                (wpn->data().main_att_mode == AttMode::thrown)
                ? ItemRefAttInf::melee
                : ItemRefAttInf::wpn_main_att_mode;

        const std::string wpn_dmg_str =
                wpn->dmg_str(
                        att_inf,
                        ItemRefDmg::average_and_melee_plus);

        const std::string wpn_hit_mod_str = wpn->hit_mod_str(att_inf);

        const std::string wpn_inf_str = wpn->name_inf_str();

        std::string wpn_str = "";

        text_format::append_with_space(wpn_str, wpn_dmg_str);
        text_format::append_with_space(wpn_str, wpn_hit_mod_str);
        text_format::append_with_space(wpn_str, wpn_inf_str);

        io::draw_text_right(
                wpn_str,
                panel,
                P(text_x1(), y),
                colors::gray(),
                draw_text_bg);

        ++y;
}

static void draw_lantern(int& y)
{
        io::draw_text(
                "Lantern",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        const Item* const item =
                map::player->inv.item_in_backpack(ItemId::lantern);

        Color color = colors::white();

        std::string lantern_str = "None";

        if (item)
        {
                const DeviceLantern* const lantern =
                        static_cast<const DeviceLantern*>(item);

                if (lantern->is_activated)
                {

                        color = colors::yellow();
                }

                lantern_str = std::to_string(lantern->nr_turns_left);
        }

        io::draw_text_right(
                lantern_str,
                panel,
                P(text_x1(), y),
                color,
                draw_text_bg);

        ++y;
}

static void draw_med_suppl(int& y)
{
        io::draw_text(
                "Med. Suppl.",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        std::string suppl_str = "-";

        const Item* const item =
                map::player->inv.item_in_backpack(ItemId::medical_bag);

        if (item)
        {
                const MedicalBag* const medical_bag =
                        static_cast<const MedicalBag*>(item);

                suppl_str = std::to_string(medical_bag->nr_supplies_);
        }

        io::draw_text_right(
                suppl_str,
                panel,
                P(text_x1(), y),
                colors::white(),
                draw_text_bg);

        ++y;
}

static void draw_armor(int& y)
{
        io::draw_text(
                "Armor",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        const std::string armor_str =
                std::to_string(map::player->armor_points());

        io::draw_text_right(
                armor_str,
                panel,
                P(text_x1(), y),
                colors::white(),
                draw_text_bg);

        ++y;
}

static void draw_encumbrance(int& y)
{
        io::draw_text(
                "Weight",
                panel,
                P(text_x0, y),
                label_color(),
                draw_text_bg);

        const int enc = map::player->enc_percent();

        const std::string enc_str = std::to_string(enc) + "%";

        const Color enc_color =
                (enc < 100) ? colors::white() :
                (enc < enc_immobile_lvl) ? colors::yellow() :
                colors::light_red();

        io::draw_text_right(
                enc_str,
                panel,
                P(text_x1(), y),
                enc_color,
                draw_text_bg);

        ++y;
}

static void draw_properties(int& y)
{
        const auto property_names =
                map::player->properties.property_names_short();

        for (const auto& name : property_names)
        {
                if (y >= panels::y1(panel))
                {
                        break;
                }

                io::draw_text(
                        name.str,
                        panel,
                        P(text_x0, y),
                        name.color,
                        draw_text_bg);

                ++y;
        }
}

// -----------------------------------------------------------------------------
// status_lines
// -----------------------------------------------------------------------------
namespace status_lines
{

void draw()
{
        io::cover_panel(panel, colors::extra_dark_gray());

        io::draw_box(panels::area(panel));

        int y = 1;

        draw_player_name(y);
        draw_player_class(y);
        draw_char_lvl_and_xp(y);
        draw_dlvl(y);
        draw_hp(y);
        draw_sp(y);
        draw_shock(y);
        draw_insanity(y);

        ++y;

        draw_wielded_wpn(y);
        draw_alt_wpn(y);

        ++y;

        draw_lantern(y);
        draw_med_suppl(y);
        draw_armor(y);
        draw_encumbrance(y);

        ++y;

        draw_properties(y);

        // Turn number
        // const int turn_nr = game_time::turn_nr();

        // const std::string turn_nr_str = std::to_string(turn_nr);

        // // "T:" + current turn number
        // const int total_turn_info_w = turn_nr_str.size() + 2;

        // p.x = panels::x1(panel) - total_turn_info_w + 1;

        // io::draw_text("T", panel, p, colors::dark_gray(), colors::black());

        // ++p.x;

        // io::draw_text(":", panel, p, colors::dark_gray());

        // ++p.x;

        // io::draw_text(turn_nr_str, panel, p, colors::white());
}

} // status_lines
