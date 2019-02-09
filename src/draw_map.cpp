// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "draw_map.hpp"

#include "actor.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "feature_door.hpp"
#include "feature_mob.hpp"
#include "feature_rigid.hpp"
#include "io.hpp"
#include "map.hpp"
#include "viewport.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static Array2<CellRenderData> s_render_array(0, 0);
static Array2<CellRenderData> s_render_array_player_memory(0, 0);


static void clear_render_array()
{
        std::fill(
                std::begin(s_render_array),
                std::end(s_render_array),
                CellRenderData());
}

static void set_rigids()
{
        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                auto& cell = map::g_cells.at(i);

                if (!cell.is_seen_by_player)
                {
                        continue;
                }

                auto& render_data = s_render_array.at(i);

                const auto* const f = cell.rigid;

                TileId gore_tile = TileId::END;

                char gore_character = 0;

                if (f->can_have_gore())
                {
                        gore_tile = f->gore_tile();
                        gore_character = f->gore_character();
                }

                if (gore_tile == TileId::END)
                {
                        render_data.tile = f->tile();
                        render_data.character = f->character();
                        render_data.color = f->color();

                        const Color feature_color_bg = f->color_bg();

                        if (feature_color_bg != colors::black())
                        {
                                render_data.color_bg =
                                        feature_color_bg;
                        }
                }
                else // Has gore
                {
                        render_data.tile = gore_tile;
                        render_data.character = gore_character;
                        render_data.color = colors::red();
                }
        }
}

static void post_process_wall_tiles()
{
        for (int x = 0; x < map::w(); ++x)
        {
                for (int y = 0; y < (map::h() - 1); ++y)
                {
                        auto& render_data = s_render_array.at(x, y);

                        const auto tile = render_data.tile;

                        const bool is_wall_top_tile =
                                Wall::is_wall_top_tile(tile);

                        if (!is_wall_top_tile)
                        {
                                continue;
                        }

                        const Wall* wall = nullptr;

                        {
                                const auto* const f =
                                        map::g_cells.at(x, y).rigid;

                                const auto id = f->id();

                                if (id == FeatureId::wall)
                                {
                                        wall = static_cast<const Wall*>(f);
                                }
                                else if (id == FeatureId::door)
                                {
                                        auto door = static_cast<const Door*>(f);

                                        if (door->is_secret())
                                        {
                                                wall = door->mimic();
                                        }
                                }

                                if (!wall)
                                {
                                        continue;
                                }
                        }

                        if (map::g_cells.at(x, y + 1).is_explored)
                        {
                                const auto tile_below =
                                        s_render_array.at(x, y + 1).tile;

                                if (Wall::is_wall_front_tile(tile_below) ||
                                    Wall::is_wall_top_tile(tile_below) ||
                                    Door::is_tile_any_door(tile_below))
                                {
                                        render_data.tile =
                                                wall->top_wall_tile();
                                }
                                else if (wall)
                                {
                                        render_data.tile =
                                                wall->front_wall_tile();
                                }
                        }
                        else // Cell below is not explored
                        {
                                if (wall)
                                {
                                        render_data.tile =
                                                wall->front_wall_tile();
                                }
                        }
                }
        }
}

static void set_dead_actors()
{
        for (Actor* actor : game_time::g_actors)
        {
                const P& p(actor->m_pos);

                if (!map::g_cells.at(p).is_seen_by_player ||
                    !actor->is_corpse() ||
                    actor->m_data->character == 0 ||
                    actor->m_data->character == ' ' ||
                    actor->m_data->tile == TileId::END)
                {
                        continue;
                }

                auto& render_data = s_render_array.at(p);

                render_data.color = actor->color();
                render_data.tile = actor->tile();
                render_data.character = actor->character();
        }
}

static void set_items()
{
        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                const Item* const item = map::g_cells.at(i).item;

                if (!map::g_cells.at(i).is_seen_by_player || !item)
                {
                        continue;
                }

                auto& render_data = s_render_array.at(i);

                render_data.color = item->color();
                render_data.tile = item->tile();
                render_data.character = item->character();
        }
}

static void copy_seen_cells_to_player_memory()
{
        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                if (map::g_cells.at(i).is_seen_by_player)
                {
                        s_render_array_player_memory.at(i) =
                                s_render_array.at(i);
                }
        }
}

static void set_light()
{
        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                const auto* const f = map::g_cells.at(i).rigid;

                if (map::g_cells.at(i).is_seen_by_player &&
                    map::g_light.at(i) &&
                    f->is_los_passable() &&
                    (f->id() != FeatureId::chasm))
                {
                        auto& color = s_render_array.at(i).color;

                        color.set_r(std::min(255, color.r() + 40));
                        color.set_g(std::min(255, color.g() + 40));
                }
        }
}

static void set_mobiles()
{
        for (auto* mob : game_time::g_mobs)
        {
                const P& p = mob->pos();

                const TileId mob_tile = mob->tile();

                const char mob_character = mob->character();

                if (map::g_cells.at(p).is_seen_by_player &&
                    mob_tile != TileId::END &&
                    mob_character != 0 &&
                    mob_character != ' ')
                {
                        auto& render_data = s_render_array.at(p);

                        render_data.color = mob->color();
                        render_data.tile = mob_tile;
                        render_data.character = mob_character;
                }
        }
}

static void set_living_seen_monster(const Mon& mon,
                                    CellRenderData& render_data)
{
        if (mon.tile() == TileId::END ||
            mon.character() == 0 ||
            mon.character() == ' ')
        {
                return;
        }

        render_data.color = mon.color();
        render_data.tile = mon.tile();
        render_data.character = mon.character();

        if (map::g_player->is_leader_of(&mon))
        {
                render_data.color_bg = colors::mon_allied_bg();
        }
        else // Player is not leader of monster
        {
                if (mon.m_aware_of_player_counter <= 0)
                {
                        render_data.color_bg =
                                colors::mon_unaware_bg();
                }
                else // Monster is aware of player
                {
                        const bool has_temporary_negative_prop =
                                mon.m_properties
                                .has_temporary_negative_prop_mon();

                        if (has_temporary_negative_prop)
                        {
                                render_data.color_bg =
                                        colors::mon_temp_property_bg();
                        }
                }
        }
}

static void set_living_hidden_monster(const Mon& mon,
                                      CellRenderData& render_data)
{
        if (mon.m_player_aware_of_me_counter <= 0)
        {
                return;
        }

        const Color color_bg =
                map::g_player->is_leader_of(&mon)
                ? colors::mon_allied_bg()
                : colors::gray();

        render_data.tile = TileId::excl_mark;
        render_data.character = '!';
        render_data.color = colors::black();
        render_data.color_bg = color_bg;
}

static void set_living_monsters()
{
        for (auto* actor : game_time::g_actors)
        {
                if (actor->is_player() || !actor->is_alive())
                {
                        continue;
                }

                const P& pos = actor->m_pos;

                auto& render_data = s_render_array.at(pos);

                const auto* const mon = static_cast<const Mon*>(actor);

                if (map::g_player->can_see_actor(*actor))
                {
                        set_living_seen_monster(*mon, render_data);
                }
                else
                {
                        set_living_hidden_monster(*mon, render_data);
                }
        }
}

static void set_unseen_cells_from_player_memory()
{
        for (size_t i = 0; i < map::nr_cells(); ++i)
        {
                auto& render_data = s_render_array.at(i);

                const Cell& cell = map::g_cells.at(i);

                if (!cell.is_seen_by_player && cell.is_explored)
                {
                        render_data = s_render_array_player_memory.at(i);

                        const double div = 3.0;

                        render_data.color =
                                render_data.color.fraction(div);

                        render_data.color_bg =
                                render_data.color_bg.fraction(div);
                }
        }
}

static void draw_render_array()
{
        const R map_view = viewport::get_map_view_area();

        for (int x = map_view.p0.x; x <= map_view.p1.x; ++x)
        {
                for (int y = map_view.p0.y; y <= map_view.p1.y; ++y)
                {
                        const P map_pos = P(x, y);

                        const P view_pos = viewport::to_view_pos(map_pos);

                        if (!map::is_pos_inside_map(map_pos))
                        {
                                continue;
                        }

                        auto& render_data = s_render_array.at(map_pos);

                        if (config::is_tiles_mode() &&
                            (render_data.tile != TileId::END))
                        {
                                io::draw_tile(
                                        render_data.tile,
                                        Panel::map,
                                        view_pos,
                                        render_data.color,
                                        true, // Draw background color
                                        render_data.color_bg);
                        }
                        else if ((render_data.character != 0) &&
                                 (render_data.character != ' '))
                        {
                                io::draw_character(
                                        render_data.character,
                                        Panel::map,
                                        view_pos,
                                        render_data.color,
                                        true, // Draw background color
                                        render_data.color_bg);
                        }
                }
        }
}

static int lifebar_length(const Actor& actor)
{
        const int actor_hp = std::max(0, actor.m_hp);

        const int actor_hp_max = actor::max_hp(actor);

        if (actor_hp < actor_hp_max)
        {
                int hp_percent = (actor_hp * 100) / actor_hp_max;

                return ((config::map_cell_px_w() - 2) * hp_percent) / 100;
        }

        return -1;
}

static void draw_life_bar(const Actor& actor)
{
        const int length = lifebar_length(actor);

        if (length < 0)
        {
                return;
        }

        const P map_pos = actor.m_pos.with_y_offset(1);

        if (!viewport::is_in_view(map_pos))
        {
                return;
        }

        const P cell_dims(config::map_cell_px_w(),
                          config::map_cell_px_h());

        const int w_green = length;

        const int w_bar_tot = cell_dims.x - 2;

        const int w_red = w_bar_tot - w_green;

        const P view_pos = viewport::to_view_pos(map_pos);

        P px_pos = io::map_to_px_coords(
                Panel::map,
                view_pos);

        px_pos.y -= 2;

        const int x0_green = px_pos.x + 1;

        const int x0_red = x0_green + w_green;

        if (w_green > 0)
        {
                const P px_p0_green(x0_green, px_pos.y);

                const R px_rect_green(
                        px_p0_green,
                        px_p0_green + P(w_green, 2) - 1);

                io::draw_rectangle_filled(
                        px_rect_green,
                        colors::light_green());
        }

        if (w_red > 0)
        {
                const P px_p0_red(x0_red, px_pos.y);

                const R px_rect_red(
                        px_p0_red,
                        px_p0_red + P(w_red, 2) - 1);

                io::draw_rectangle_filled(
                        px_rect_red,
                        colors::light_red());
        }
}

static void draw_monster_life_bars()
{
        for (auto* actor : game_time::g_actors)
        {
                if (!actor->is_player() &&
                    actor->is_alive() &&
                    map::g_player->can_see_actor(*actor))
                {
                        draw_life_bar(*actor);
                }
        }
}

static void draw_player_character()
{
        const P& pos = map::g_player->m_pos;

        if (!viewport::is_in_view(pos))
        {
                return;
        }

        Item* item = map::g_player->m_inv.item_in_slot(SlotId::wpn);

        const bool is_ghoul = player_bon::bg() == Bg::ghoul;

        const Color color = map::g_player->color();

        Color color_bg = colors::black();

        bool uses_ranged_wpn = false;

        if (item)
        {
                uses_ranged_wpn = item->data().ranged.is_ranged_wpn;
        }

        const TileId tile =
                is_ghoul ? TileId::ghoul :
                uses_ranged_wpn ? TileId::player_firearm :
                TileId::player_melee;

        const char character = '@';

        auto& player_render_data = s_render_array.at(pos);

        player_render_data.tile = tile;
        player_render_data.character = character;
        player_render_data.color = color;
        player_render_data.color_bg = color_bg;

        io::draw_symbol(
                tile,
                character,
                Panel::map,
                viewport::to_view_pos(pos),
                color,
                true, // Draw background color
                color_bg);

        draw_life_bar(*map::g_player);
}

// -----------------------------------------------------------------------------
// draw_map
// -----------------------------------------------------------------------------
namespace draw_map
{

void clear()
{
        clear_render_array();

        std::fill(
                std::begin(s_render_array_player_memory),
                std::end(s_render_array_player_memory),
                CellRenderData());
}

void run()
{
        if (s_render_array.dims() != map::dims())
        {
                s_render_array.resize(map::dims());

                s_render_array_player_memory.resize(map::dims());
        }

        clear_render_array();

        set_unseen_cells_from_player_memory();

        set_rigids();

        if (config::is_tiles_mode() && !config::is_tiles_wall_full_square())
        {
                post_process_wall_tiles();
        }

        set_dead_actors();

        set_items();

        copy_seen_cells_to_player_memory();

        set_light();

        set_mobiles();

        set_living_monsters();

        draw_render_array();

        draw_monster_life_bars();

        draw_player_character();
}

const CellRenderData& get_drawn_cell(int x, int y)
{
        return s_render_array.at(x, y);
}

const CellRenderData& get_drawn_cell_player_memory(int x, int y)
{
        return s_render_array_player_memory.at(x, y);
}

} // draw_map
