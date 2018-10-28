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
#include "feature_rigid.hpp"
#include "game_commands.hpp"
#include "inventory_handling.hpp"
#include "io.hpp"
#include "item_factory.hpp"
#include "line_calc.hpp"
#include "look.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
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
        marker_render_data_.resize(
                viewport::get_map_view_area().dims());

        pos_ = map::player->pos;

        if (use_player_tgt())
        {
                // First, attempt to place marker at player target.
                const bool did_go_to_tgt = try_go_to_tgt();

                if (!did_go_to_tgt)
                {
                        // If no target available, attempt to place marker at
                        // closest visible monster. This sets a new player
                        // target if successful.
                        map::player->tgt_ = nullptr;

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
        if (!viewport::is_in_view(pos_))
        {
                viewport::focus_on(pos_);
        }

        auto line =
                line_calc::calc_new_line(
                        origin_,
                        pos_,
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

                        const Cell& c = map::cells.at(p);

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
        InputData input;

        if (!config::is_bot_playing())
        {
                input = io::get();
        }

        const auto game_cmd = game_commands::to_cmd(input);

        msg_log::clear();

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
                        auto& d = marker_render_data_.at(x, y);

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

                const int dist = king_dist(origin_, line_pos);

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

                        auto& d = marker_render_data_.at(view_pos);

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
                ? origin_
                : line.back();

        if (viewport::is_in_view(head_pos))
        {
                const P view_pos = viewport::to_view_pos(head_pos);

                auto& d = marker_render_data_.at(view_pos);

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

void MarkerState::move(const Dir dir)
{
        const P new_pos(pos_ + dir_utils::offset(dir));

        // We limit the distance from the player that the marker can be moved to
        // (mostly just to avoid segfaults or weird integer wraparound behavior)
        // The limit is an arbitrary big number, larger than any map should be
        const int max_dist_from_player = 300;

        if (king_dist(map::player->pos, new_pos) <= max_dist_from_player)
        {
                pos_ = new_pos;

                on_moved();
        }
}

bool MarkerState::try_go_to_tgt()
{
        const Actor* const tgt = map::player->tgt_;

        if (!tgt)
        {
                return false;
        }

        const auto seen_foes = map::player->seen_foes();

        if (!seen_foes.empty())
        {
                for (auto* const actor : seen_foes)
                {
                        if (tgt == actor)
                        {
                                pos_ = actor->pos;

                                return true;
                        }
                }
        }

        return false;
}

void MarkerState::try_go_to_closest_enemy()
{
        const auto seen_foes = map::player->seen_foes();

        std::vector<P> seen_foes_cells;

        map::actor_cells(seen_foes, seen_foes_cells);

        // If player sees enemies, suggest one for targeting
        if (!seen_foes_cells.empty())
        {
                pos_ = closest_pos(map::player->pos, seen_foes_cells);

                map::player->tgt_ = map::actor_at_pos(pos_);
        }
}

// -----------------------------------------------------------------------------
// View state
// -----------------------------------------------------------------------------
void Viewing::on_moved()
{
        msg_log::clear();

        look::print_location_info_msgs(pos_);

        const auto* const actor = map::actor_at_pos(pos_);

        if (actor &&
            !actor->is_player() &&
            map::player->can_see_actor(*actor))
        {
                // TODO: This should not be specified here
                const auto view_key =
                        config::is_vi_keys()
                        ? 'v'
                        : 'l';

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

        msg_log::add(common_text::cancel_hint, colors::light_white());
}

void Viewing::handle_input(const InputData& input)
{
        const auto game_cmd = game_commands::to_cmd(input);

        if (game_cmd == GameCmd::look)
        {
                auto* const actor = map::actor_at_pos(pos_);

                if (actor &&
                    actor != map::player &&
                    map::player->can_see_actor(*actor))
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
        look::print_living_actor_info_msg(pos_);

        const bool is_in_range =
                king_dist(origin_, pos_) < red_from_king_dist();

        if (is_in_range)
        {
                auto* const actor = map::actor_at_pos(pos_);

                if (actor &&
                    !actor->is_player() &&
                    map::player->can_see_actor(*actor))
                {
                        RangedAttData att_data(
                                map::player,
                                origin_,
                                actor->pos, // Aim position
                                actor->pos, // Current position
                                wpn_);

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
                common_text::cancel_hint;

        msg_log::add(msg, colors::light_white());
}

void Aiming::handle_input(const InputData& input)
{
        auto game_cmd = GameCmd::undefined;

        if (config::is_bot_playing())
        {
                // Bot is playing, fire at a random position
                game_cmd = GameCmd::fire;

                pos_.set(
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
                if (pos_ != map::player->pos)
                {
                        msg_log::clear();

                        Actor* const actor = map::actor_at_pos(pos_);

                        if (actor && map::player->can_see_actor(*actor))
                        {
                                map::player->tgt_ = actor;
                        }

                        const P pos = pos_;

                        Wpn* const wpn = &wpn_;

                        states::pop();

                        // NOTE: This object is now destroyed

                        attack::ranged(
                                map::player,
                                map::player->pos,
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
        const int effective_range = wpn_.data().ranged.effective_range;

        return
                (effective_range < 0)
                ? -1
                : (effective_range + 1);
}

int Aiming::red_from_king_dist() const
{
        const int max_range = wpn_.data().ranged.max_range;

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
        look::print_living_actor_info_msg(pos_);

        const bool is_in_range =
                king_dist(origin_, pos_) < red_from_king_dist();

        if (is_in_range)
        {
                auto* const actor = map::actor_at_pos(pos_);

                if (actor &&
                    !actor->is_player() &&
                    map::player->can_see_actor(*actor))
                {
                        ThrowAttData att_data(
                                map::player,
                                actor->pos, // Aim position
                                actor->pos, // Current position
                                *inv_item_);

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
                common_text::cancel_hint;

        msg_log::add(msg, colors::light_white());
}

void Throwing::handle_input(const InputData& input)
{
        const auto game_cmd = game_commands::to_cmd(input);

        if ((game_cmd == GameCmd::throw_item) || (input.key == SDLK_RETURN))
        {
                if (pos_ != map::player->pos)
                {
                        msg_log::clear();

                        Actor* const actor = map::actor_at_pos(pos_);

                        if (actor && map::player->can_see_actor(*actor))
                        {
                                map::player->tgt_ = actor;
                        }

                        const P pos = pos_;

                        Item* item_to_throw =
                                item_factory::copy_item(*inv_item_);

                        item_to_throw->nr_items_ = 1;

                        item_to_throw->clear_actor_carrying();

                        inv_item_ = map::player->inv.decr_item(inv_item_);

                        map::player->last_thrown_item_ = inv_item_;

                        states::pop();

                        // NOTE: This object is now destroyed

                        // Perform the actual throwing
                        throwing::throw_item(
                                *map::player,
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
        const int effective_range = inv_item_->data().ranged.effective_range;

        return
                (effective_range < 0)
                ? -1
                : (effective_range + 1);
}

int Throwing::red_from_king_dist() const
{
        const int max_range = inv_item_->data().ranged.max_range;

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
        const ItemId id = explosive_.id();

        if (id != ItemId::dynamite &&
            id != ItemId::molotov &&
            id != ItemId::smoke_grenade)
        {
                return;
        }

        const R expl_area =
                explosion::explosion_area(pos_, expl_std_radi);

        const Color color_bg = colors::red().fraction(2.0);

        // Draw explosion radius area overlay
        for (int y = expl_area.p0.y; y <= expl_area.p1.y; ++y)
        {
                for (int x = expl_area.p0.x; x <= expl_area.p1.x; ++x)
                {
                        const P p(x, y);

                        if (!viewport::is_in_view(p) ||
                            !map::is_pos_inside_map(p) ||
                            !map::cells.at(p).is_explored)
                        {
                                continue;
                        }

                        const auto& render_d = draw_map::get_drawn_cell(x, y);

                        const P view_pos = viewport::to_view_pos(p);

                        const auto& marker_render_d =
                                marker_render_data_.at(view_pos);

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
        look::print_location_info_msgs(pos_);

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
                common_text::cancel_hint;

        msg_log::add(msg, colors::light_white());
}

void ThrowingExplosive::handle_input(const InputData& input)
{
        const auto game_cmd = game_commands::to_cmd(input);

        if ((game_cmd == GameCmd::throw_item) || (input.key == SDLK_RETURN))
        {
                msg_log::clear();

                const P pos = pos_;

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
        const int max_range = explosive_.data().ranged.max_range;

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
        blocked_(blocked)
{

}

int CtrlTele::chance_of_success_pct(const P& tgt) const
{
        const int dist = king_dist(map::player->pos, tgt);

        const int chance = constr_in_range(25, 100 - dist, 95);

        return chance;
}

void CtrlTele::on_start_hook()
{
        msg_log::add(
                "I have the power to control teleportation.",
                colors::white(),
                false,
                MorePromptOnMsg::yes);
}

void CtrlTele::on_moved()
{
        look::print_location_info_msgs(pos_);

        if (pos_ != map::player->pos)
        {
                const int chance_pct = chance_of_success_pct(pos_);

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
        if ((input.key == SDLK_RETURN) && (pos_ != map::player->pos))
        {
                const int chance = chance_of_success_pct(pos_);

                const bool roll_ok = rnd::percent(chance);

                const bool is_tele_success =
                        roll_ok &&
                        blocked_.rect().is_pos_inside(pos_) &&
                        !blocked_.at(pos_);

                const P tgt_p(pos_);

                states::pop();

                // NOTE: This object is now destroyed

                if (is_tele_success)
                {
                        // Teleport to this exact destination
                        teleport(*map::player, tgt_p, blocked_);
                }
                else // Failed to teleport (blocked or roll failed)
                {
                        msg_log::add(
                                "I failed to go there...",
                                colors::white(),
                                false,
                                MorePromptOnMsg::yes);

                        // Run a randomized teleport with no teleport control
                        teleport(*map::player, ShouldCtrlTele::never);
                }
        }
}
