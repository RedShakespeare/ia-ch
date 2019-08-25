// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "marker.hpp"

#include <vector>
#include <cstring>

#include "actor_player.hpp"
#include "attack.hpp"
#include "attack_data.hpp"
#include "common_text.hpp"
#include "config.hpp"
#include "draw_map.hpp"
#include "explosion.hpp"
#include "terrain.hpp"
#include "game_commands.hpp"
#include "inventory_handling.hpp"
#include "io.hpp"
#include "item_factory.hpp"
#include "line_calc.hpp"
#include "look.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "teleport.hpp"
#include "throwing.hpp"
#include "viewport.hpp"

// -----------------------------------------------------------------------------
// Marker state
// -----------------------------------------------------------------------------
StateId MarkerState::id()
{
        return StateId::marker;
}

void MarkerState::on_start()
{
        m_marker_render_data.resize(
                viewport::get_map_view_area().dims());

        m_pos = map::g_player->m_pos;

        if (use_player_tgt())
        {
                // First, attempt to place marker at player target.
                const bool did_go_to_tgt = try_go_to_tgt();

                if (!did_go_to_tgt)
                {
                        // If no target available, attempt to place marker at
                        // closest visible monster. This sets a new player
                        // target if successful.
                        map::g_player->m_tgt = nullptr;

                        try_go_to_closest_enemy();
                }
        }

        on_start_hook();

        on_moved();
}

void MarkerState::on_popped()
{

}

void MarkerState::draw()
{
        if (!viewport::is_in_view(m_pos))
        {
                viewport::focus_on(m_pos);
        }

        auto line =
                line_calc::calc_new_line(
                        m_origin,
                        m_pos,
                        true, // Stop at target
                        INT_MAX, // Travel limit
                        true); // Allow outside map

        // Remove origin position
        if (!line.empty())
        {
                line.erase(line.begin());
        }

        const int orange_from_dist = orange_from_king_dist();

        const int red_from_dist = red_from_king_dist();

        int red_from_idx = -1;

        auto blocked_parser = map_parsers::BlocksProjectiles();

        if (show_blocked())
        {
                for (size_t i = 0; i < line.size(); ++i)
                {
                        const P& p = line[i];

                        if (!map::is_pos_inside_map(p))
                        {
                                break;
                        }

                        const Cell& c = map::g_cells.at(p);

                        if (c.is_seen_by_player && blocked_parser.cell(p))
                        {
                                red_from_idx = i;
                                break;
                        }
                }
        }

        draw_marker(
                line,
                orange_from_dist,
                red_from_dist,
                red_from_idx);

        on_draw();
}

void MarkerState::update()
{
        const int nr_jump_steps = 5;

        InputData input;

        if (!config::is_bot_playing())
        {
                input = io::get();
        }

        const auto game_cmd = game_commands::to_cmd(input);

        if (game_cmd != GameCmd::none)
        {
                msg_log::clear();
        }

        switch (game_cmd)
        {
        case GameCmd::right:
                move(Dir::right);
                break;

        case GameCmd::down:
                move(Dir::down);
                break;

        case GameCmd::left:
                move(Dir::left);
                break;

        case GameCmd::up:
                move(Dir::up);
                break;

        case GameCmd::up_right:
                move(Dir::up_right);
                break;

        case GameCmd::down_right:
                move(Dir::down_right);
                break;

        case GameCmd::up_left:
                move(Dir::up_left);
                break;

        case GameCmd::down_left:
                move(Dir::down_left);
                break;

        case GameCmd::auto_move_right:
                move(Dir::right, nr_jump_steps);
                break;

        case GameCmd::auto_move_down:
                move(Dir::down, nr_jump_steps);
                break;

        case GameCmd::auto_move_left:
                move(Dir::left, nr_jump_steps);
                break;

        case GameCmd::auto_move_up:
                move(Dir::up, nr_jump_steps);
                break;

        case GameCmd::auto_move_up_right:
                move(Dir::up_right, nr_jump_steps);
                break;

        case GameCmd::auto_move_down_right:
                move(Dir::down_right, nr_jump_steps);
                break;

        case GameCmd::auto_move_up_left:
                move(Dir::up_left, nr_jump_steps);
                break;

        case GameCmd::auto_move_down_left:
                move(Dir::down_left, nr_jump_steps);
                break;

        default:
                // Input not handled here - delegate to child classes
                handle_input(input);
        }
}

void MarkerState::draw_marker(
        const std::vector<P>& line,
        const int orange_from_king_dist,
        const int red_from_king_dist,
        const int red_from_idx)
{
        const P map_view_dims =
                viewport::get_map_view_area().dims();

        for (int x = 0; x < map_view_dims.x; ++x)
        {
                for (int y = 0; y < map_view_dims.y; ++y)
                {
                        auto& d = m_marker_render_data.at(x, y);

                        d.tile  = TileId::END;
                        d.character = 0;
                }
        }

        Color color = colors::light_green();

        // Draw the line

        // NOTE: We include the head index in this loop, so that we can set up
        // which color it should be drawn with, but we do the actual drawing of
        // the head after the loop
        for (size_t line_idx = 0; line_idx < line.size(); ++line_idx)
        {
                const P& line_pos = line[line_idx];

                if (!viewport::is_in_view(line_pos))
                {
                        continue;
                }

                const int dist = king_dist(m_origin, line_pos);

                // Draw red due to index, or due to distance?
                const bool red_by_idx =
                        (red_from_idx != -1) &&
                        ((int)line_idx >= red_from_idx);

                const bool red_by_dist =
                        (red_from_king_dist != -1) &&
                        (dist >= red_from_king_dist);

                const bool is_red = red_by_idx || red_by_dist;

                // NOTE: Final color is stored for drawing the head
                if (is_red)
                {
                        color = colors::light_red();
                }
                // Not red - orange by distance?
                else if ((orange_from_king_dist != -1) &&
                         (dist >= orange_from_king_dist))
                {
                        color = colors::orange();
                }

                // Do not draw the head yet
                const int tail_size_int = (int)line.size() - 1;

                if ((int)line_idx < tail_size_int)
                {
                        const P view_pos = viewport::to_view_pos(line_pos);

                        auto& d = m_marker_render_data.at(view_pos);

                        d.tile = TileId::aim_marker_line;

                        d.character = '*';

                        d.color = color;

                        d.color_bg = colors::black();

                        io::draw_symbol(
                                d.tile,
                                d.character,
                                Panel::map,
                                view_pos,
                                d.color,
                                true, // Draw background color
                                d.color_bg);
                }
        } // line loop

        // Draw the head
        const P& head_pos =
                line.empty()
                ? m_origin
                : line.back();

        if (viewport::is_in_view(head_pos))
        {
                const P view_pos = viewport::to_view_pos(head_pos);

                auto& d = m_marker_render_data.at(view_pos);

                d.tile = TileId::aim_marker_head;

                d.character = 'X';

                d.color = color;

                d.color_bg = colors::black();

                io::draw_symbol(
                        d.tile,
                        d.character,
                        Panel::map,
                        viewport::to_view_pos(head_pos),
                        d.color,
                        true, // Draw background color
                        d.color_bg);
        }
}

void MarkerState::move(const Dir dir, const int nr_steps)
{
        const P new_pos(m_pos + dir_utils::offset(dir).scaled_up(nr_steps));

        // We limit the distance from the player that the marker can be moved to
        // (mostly just to avoid segfaults or weird integer wraparound behavior)
        // The limit is an arbitrary big number, larger than any map should be
        const int max_dist_from_player = 300;

        if (king_dist(map::g_player->m_pos, new_pos) <= max_dist_from_player)
        {
                m_pos = new_pos;

                on_moved();
        }
}

bool MarkerState::try_go_to_tgt()
{
        const auto* const tgt = map::g_player->m_tgt;

        if (!tgt)
        {
                return false;
        }

        const auto seen_foes = map::g_player->seen_foes();

        if (!seen_foes.empty())
        {
                for (auto* const actor : seen_foes)
                {
                        if (tgt == actor)
                        {
                                m_pos = actor->m_pos;

                                return true;
                        }
                }
        }

        return false;
}

void MarkerState::try_go_to_closest_enemy()
{
        const auto seen_foes = map::g_player->seen_foes();

        std::vector<P> seen_foes_cells;

        map::actor_cells(seen_foes, seen_foes_cells);

        // If player sees enemies, suggest one for targeting
        if (!seen_foes_cells.empty())
        {
                m_pos = closest_pos(map::g_player->m_pos, seen_foes_cells);

                map::g_player->m_tgt = map::first_actor_at_pos(m_pos);
        }
}

// -----------------------------------------------------------------------------
// View state
// -----------------------------------------------------------------------------
void Viewing::on_moved()
{
        msg_log::clear();

        look::print_location_info_msgs(m_pos);

        const auto* const actor = map::first_actor_at_pos(m_pos);

        if (actor &&
            !actor->is_player() &&
            map::g_player->can_see_actor(*actor))
        {
                // TODO: This should not be specified here
                const auto view_key = 'v';

                // In debug mode, confirm that this is actually the correct key,
                // however see TODO above
#ifndef NDEBUG
                {
                        InputData dummy_input;
                        dummy_input.key = view_key;

                        const auto game_cmd =
                                game_commands::to_cmd(dummy_input);

                        ASSERT(game_cmd == GameCmd::look);
                }
#endif // NDEBUG

                const std::string msg =
                        std::string("[") +
                        view_key +
                        std::string("] for description");

                msg_log::add(msg, colors::light_white());
        }

        msg_log::add(common_text::g_cancel_hint, colors::light_white());
}

void Viewing::handle_input(const InputData& input)
{
        const auto game_cmd = game_commands::to_cmd(input);

        if (game_cmd == GameCmd::look)
        {
                auto* const actor = map::first_actor_at_pos(m_pos);

                if (actor &&
                    actor != map::g_player &&
                    map::g_player->can_see_actor(*actor))
                {
                        msg_log::clear();

                        std::unique_ptr<ViewActorDescr>
                                view_actor_descr(new ViewActorDescr(*actor));

                        states::push(std::move(view_actor_descr));
                }
        }
        else if ((input.key == SDLK_ESCAPE) || (input.key == SDLK_SPACE))
        {
                msg_log::clear();

                states::pop();
        }
}

// -----------------------------------------------------------------------------
// Aim marker state
// -----------------------------------------------------------------------------
void Aiming::on_moved()
{
        look::print_living_actor_info_msg(m_pos);

        const bool is_in_range =
                king_dist(m_origin, m_pos) < red_from_king_dist();

        if (is_in_range)
        {
                auto* const actor = map::first_actor_at_pos(m_pos);

                if (actor &&
                    !actor->is_player() &&
                    map::g_player->can_see_actor(*actor))
                {
                        RangedAttData att_data(
                                map::g_player,
                                m_origin,
                                actor->m_pos, // Aim position
                                actor->m_pos, // Current position
                                m_wpn);

                        const int hit_chance =
                                ability_roll::hit_chance_pct_actual(
                                        att_data.hit_chance_tot);

                        msg_log::add(
                                std::to_string(hit_chance) +
                                "% hit chance.",
                                colors::light_white());
                }
        }

        // TODO: This should not be specified here
        const auto fire_key = 'f';

        // In debug mode, confirm that this is actually the correct key,
        // however see TODO above
#ifndef NDEBUG
        {
                InputData dummy_input;
                dummy_input.key = fire_key;

                const auto game_cmd = game_commands::to_cmd(dummy_input);

                ASSERT(game_cmd == GameCmd::fire);
        }
#endif // NDEBUG

        const std::string msg =
                std::string("[") +
                fire_key +
                std::string("] to fire ") +
                common_text::g_cancel_hint;

        msg_log::add(msg, colors::light_white());
}

void Aiming::handle_input(const InputData& input)
{
        auto game_cmd = GameCmd::undefined;

        if (config::is_bot_playing())
        {
                // Bot is playing, fire at a random position
                game_cmd = GameCmd::fire;

                m_pos.set(
                        rnd::range(0, map::w() - 1),
                        rnd::range(0, map::h() - 1));
        }
        else
        {
                // Human player
                game_cmd = game_commands::to_cmd(input);
        }

        if ((game_cmd == GameCmd::fire) || (input.key == SDLK_RETURN))
        {
                if (m_pos != map::g_player->m_pos)
                {
                        msg_log::clear();

                        auto* const actor = map::first_actor_at_pos(m_pos);

                        if (actor && map::g_player->can_see_actor(*actor))
                        {
                                map::g_player->m_tgt = actor;
                        }

                        const P pos = m_pos;

                        auto* const wpn = &m_wpn;

                        states::pop();

                        // NOTE: This object is now destroyed

                        attack::ranged(
                                map::g_player,
                                map::g_player->m_pos,
                                pos,
                                *wpn);
                }
        }
        else if ((input.key == SDLK_ESCAPE) || (input.key == SDLK_SPACE))
        {
                states::pop();
        }
}

int Aiming::orange_from_king_dist() const
{
        const int effective_range = m_wpn.data().ranged.effective_range;

        return
                (effective_range < 0)
                ? -1
                : (effective_range + 1);
}

int Aiming::red_from_king_dist() const
{
        const int max_range = m_wpn.data().ranged.max_range;

        return
                (max_range < 0)
                ? -1
                : (max_range + 1);
}

// -----------------------------------------------------------------------------
// Throw attack marker state
// -----------------------------------------------------------------------------
void Throwing::on_moved()
{
        look::print_living_actor_info_msg(m_pos);

        const bool is_in_range =
                king_dist(m_origin, m_pos) < red_from_king_dist();

        if (is_in_range)
        {
                auto* const actor = map::first_actor_at_pos(m_pos);

                if (actor &&
                    !actor->is_player() &&
                    map::g_player->can_see_actor(*actor))
                {
                        ThrowAttData att_data(
                                map::g_player,
                                actor->m_pos, // Aim position
                                actor->m_pos, // Current position
                                *m_inv_item);

                        const int hit_chance =
                                ability_roll::hit_chance_pct_actual(
                                        att_data.hit_chance_tot);

                        msg_log::add(
                                std::to_string(hit_chance) +
                                "% hit chance.",
                                colors::light_white());
                }
        }

        // TODO: This should not be specified here
        const auto throw_key = 't';

        // In debug mode, confirm that this is actually the correct key,
        // however see TODO above
#ifndef NDEBUG
        {
                InputData dummy_input;
                dummy_input.key = throw_key;

                const auto game_cmd = game_commands::to_cmd(dummy_input);

                ASSERT(game_cmd == GameCmd::throw_item);
        }
#endif // NDEBUG

        const std::string msg =
                std::string("[") +
                throw_key +
                std::string("] to throw ") +
                common_text::g_cancel_hint;

        msg_log::add(msg, colors::light_white());
}

void Throwing::handle_input(const InputData& input)
{
        const auto game_cmd = game_commands::to_cmd(input);

        if ((game_cmd == GameCmd::throw_item) || (input.key == SDLK_RETURN))
        {
                if (m_pos != map::g_player->m_pos)
                {
                        msg_log::clear();

                        auto* const actor = map::first_actor_at_pos(m_pos);

                        if (actor && map::g_player->can_see_actor(*actor))
                        {
                                map::g_player->m_tgt = actor;
                        }

                        const P pos = m_pos;

                        auto* item_to_throw = item::copy_item(*m_inv_item);

                        item_to_throw->m_nr_items = 1;

                        item_to_throw->clear_actor_carrying();

                        m_inv_item = map::g_player->m_inv.decr_item(m_inv_item);

                        map::g_player->m_last_thrown_item = m_inv_item;

                        states::pop();

                        // NOTE: This object is now destroyed

                        // Perform the actual throwing
                        throwing::throw_item(
                                *map::g_player,
                                pos,
                                *item_to_throw);
                }
        }
        else if ((input.key == SDLK_ESCAPE) || (input.key == SDLK_SPACE))
        {
                states::pop();
        }
}

int Throwing::orange_from_king_dist() const
{
        const int effective_range = m_inv_item->data().ranged.effective_range;

        return
                (effective_range < 0)
                ? -1
                : (effective_range + 1);
}

int Throwing::red_from_king_dist() const
{
        const int max_range = m_inv_item->data().ranged.max_range;

        return
                (max_range < 0)
                ? -1
                : (max_range + 1);
}

// -----------------------------------------------------------------------------
// Throw explosive marker state
// -----------------------------------------------------------------------------
void ThrowingExplosive::on_draw()
{
        const auto id = m_explosive.id();

        if (id != item::Id::dynamite &&
            id != item::Id::molotov &&
            id != item::Id::smoke_grenade)
        {
                return;
        }

        const R expl_area =
                explosion::explosion_area(m_pos, g_expl_std_radi);

        const Color color_bg = colors::red().fraction(2.0);

        // Draw explosion radius area overlay
        for (int y = expl_area.p0.y; y <= expl_area.p1.y; ++y)
        {
                for (int x = expl_area.p0.x; x <= expl_area.p1.x; ++x)
                {
                        const P p(x, y);

                        if (!viewport::is_in_view(p) ||
                            !map::is_pos_inside_map(p) ||
                            !map::g_cells.at(p).is_explored)
                        {
                                continue;
                        }

                        const auto& render_d = draw_map::get_drawn_cell(x, y);

                        const P view_pos = viewport::to_view_pos(p);

                        const auto& marker_render_d =
                                m_marker_render_data.at(view_pos);

                        // Draw overlay if the cell contains either a map
                        // symbol, or a marker symbol
                        if ((render_d.character != 0) ||
                            (marker_render_d.character != 0))
                        {
                                const bool has_marker =
                                        marker_render_d.character != 0;

                                const auto& d =
                                        has_marker
                                        ? marker_render_d
                                        : render_d;

                                const Color color_fg =
                                        has_marker
                                        ? d.color
                                        : colors::orange();

                                io::draw_symbol(
                                        d.tile,
                                        d.character,
                                        Panel::map,
                                        view_pos,
                                        color_fg,
                                        true, // Draw background color
                                        color_bg);
                        }
                }
        }
}

void ThrowingExplosive::on_moved()
{
        look::print_location_info_msgs(m_pos);

        // TODO: This should not be specified here
        const auto throw_key = 't';

        // In debug mode, confirm that this is actually the correct key,
        // however see TODO above
#ifndef NDEBUG
        {
                InputData dummy_input;
                dummy_input.key = throw_key;

                const auto game_cmd = game_commands::to_cmd(dummy_input);

                ASSERT(game_cmd == GameCmd::throw_item);
        }
#endif // NDEBUG

        const std::string msg =
                std::string("[") +
                throw_key +
                std::string("] to throw ") +
                common_text::g_cancel_hint;

        msg_log::add(msg, colors::light_white());
}

void ThrowingExplosive::handle_input(const InputData& input)
{
        const auto game_cmd = game_commands::to_cmd(input);

        if ((game_cmd == GameCmd::throw_item) || (input.key == SDLK_RETURN))
        {
                msg_log::clear();

                const P pos = m_pos;

                states::pop();

                // NOTE: This object is now destroyed

                throwing::player_throw_lit_explosive(pos);
        }
        else if ((input.key == SDLK_ESCAPE) || (input.key == SDLK_SPACE))
        {
                states::pop();
        }
}

int ThrowingExplosive::red_from_king_dist() const
{
        const int max_range = m_explosive.data().ranged.max_range;

        return
                (max_range < 0)
                ? -1
                : (max_range + 1);
}

// -----------------------------------------------------------------------------
// Teleport control marker state
// -----------------------------------------------------------------------------
CtrlTele::CtrlTele(const P& origin, const Array2<bool>& blocked) :
        MarkerState(origin),
        m_blocked(blocked)
{

}

int CtrlTele::chance_of_success_pct(const P& tgt) const
{
        const int dist = king_dist(map::g_player->m_pos, tgt);

        const int chance = constr_in_range(25, 100 - dist, 95);

        return chance;
}

void CtrlTele::on_start_hook()
{
        msg_log::add(
                "I have the power to control teleportation.",
                colors::white(),
                MsgInterruptPlayer::no,
                MorePromptOnMsg::yes);
}

void CtrlTele::on_moved()
{
        look::print_location_info_msgs(m_pos);

        if (m_pos != map::g_player->m_pos)
        {
                const int chance_pct = chance_of_success_pct(m_pos);

                msg_log::add(
                        std::to_string(chance_pct) + "% chance of success.",
                        colors::light_white());

                msg_log::add(
                        "[enter] to try teleporting here",
                        colors::light_white());
        }
}

void CtrlTele::handle_input(const InputData& input)
{
        if ((input.key == SDLK_RETURN) && (m_pos != map::g_player->m_pos))
        {
                const int chance = chance_of_success_pct(m_pos);

                const bool roll_ok = rnd::percent(chance);

                const bool is_tele_success =
                        roll_ok &&
                        m_blocked.rect().is_pos_inside(m_pos) &&
                        !m_blocked.at(m_pos);

                const P tgt_p(m_pos);

                states::pop();

                // NOTE: This object is now destroyed

                if (is_tele_success)
                {
                        // Teleport to this exact destination
                        teleport(*map::g_player, tgt_p, m_blocked);
                }
                else // Failed to teleport (blocked or roll failed)
                {
                        msg_log::add(
                                "I failed to go there...",
                                colors::white(),
                                MsgInterruptPlayer::no,
                                MorePromptOnMsg::yes);

                        // Run a randomized teleport with no teleport control
                        teleport(*map::g_player, ShouldCtrlTele::never);
                }
        }
}
