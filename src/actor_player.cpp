#include "actor_player.hpp"

#include <string>
#include <cmath>

#include "actor_death.hpp"
#include "actor_factory.hpp"
#include "actor_mon.hpp"
#include "attack.hpp"
#include "audio.hpp"
#include "bot.hpp"
#include "create_character.hpp"
#include "drop.hpp"
#include "explosion.hpp"
#include "feature_door.hpp"
#include "feature_mob.hpp"
#include "feature_trap.hpp"
#include "fov.hpp"
#include "game.hpp"
#include "init.hpp"
#include "insanity.hpp"
#include "inventory.hpp"
#include "inventory_handling.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_device.hpp"
#include "item_factory.hpp"
#include "item_potion.hpp"
#include "item_rod.hpp"
#include "item_scroll.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "minimap.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "popup.hpp"
#include "property.hpp"
#include "property_handler.hpp"
#include "query.hpp"
#include "reload.hpp"
#include "saving.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static void print_aware_invis_mon_msg(const Mon& mon)
{
        std::string mon_ref;

        if (mon.data->is_ghost)
        {
                mon_ref = "some foul entity";
        }
        else if (mon.data->is_humanoid)
        {
                mon_ref = "someone";
        }
        else
        {
                mon_ref = "a creature";
        }

        msg_log::add(
                "There is " + mon_ref + " here!",
                colors::msg_note(),
                false,
                MorePromptOnMsg::yes);
}

static const std::vector<std::string> item_feeling_messages_ = {
        "I feel like I should examine this place thoroughly.",
        "I feel like there is something of great interest here.",
        "I sense an object of great power here."
};

// -----------------------------------------------------------------------------
// Player
// -----------------------------------------------------------------------------
Player::Player() :
    Actor(),
    active_medical_bag_(nullptr),
    handle_armor_countdown_(0),
    armor_putting_on_backpack_idx_(-1),
    is_dropping_armor_from_body_slot_(false),
    active_explosive_(nullptr),
    tgt_(nullptr),
    wait_turns_left(-1),
    ins_(0),
    shock_(0.0),
    shock_tmp_(0.0),
    perm_shock_taken_current_turn_(0.0),
    nr_turns_until_ins_(-1),
    quick_move_dir_(Dir::END),
    has_taken_quick_move_step_(false),
    nr_turns_until_rspell_(-1),
    unarmed_wpn_(nullptr) {}

Player::~Player()
{
    delete active_explosive_;
    delete unarmed_wpn_;
}

void Player::save() const
{
    properties.save();

    saving::put_int(ins_);
    saving::put_int((int)shock_);
    saving::put_int(hp);
    saving::put_int(base_max_hp);
    saving::put_int(sp);
    saving::put_int(base_max_sp);
    saving::put_int(pos.x);
    saving::put_int(pos.y);
    saving::put_int(nr_turns_until_rspell_);

    ASSERT(unarmed_wpn_);

    saving::put_int((int)unarmed_wpn_->id());

    for (int i = 0; i < (int)AbilityId::END; ++i)
    {
        const int v = data->ability_values.raw_val(AbilityId(i));

        saving::put_int(v);
    }
}

void Player::load()
{
    properties.load();

    ins_ = saving::get_int();
    shock_ = double(saving::get_int());
    hp = saving::get_int();
    base_max_hp = saving::get_int();
    sp = saving::get_int();
    base_max_sp = saving::get_int();
    pos.x = saving::get_int();
    pos.y = saving::get_int();
    nr_turns_until_rspell_ = saving::get_int();

    ItemId unarmed_wpn_id = ItemId(saving::get_int());

    ASSERT(unarmed_wpn_id < ItemId::END);

    delete unarmed_wpn_;
    unarmed_wpn_ = nullptr;

    Item* const unarmed_item = item_factory::make(unarmed_wpn_id);

    ASSERT(unarmed_item);

    unarmed_wpn_ = static_cast<Wpn*>(unarmed_item);

    for (int i = 0; i < (int)AbilityId::END; ++i)
    {
        const int v = saving::get_int();

        data->ability_values.set_val(AbilityId(i), v);
    }
}

bool Player::can_see_actor(const Actor& other) const
{
    if (this == &other)
    {
        return true;
    }

    if (init::is_cheat_vision_enabled)
    {
        return true;
    }

    const Cell& cell = map::cells.at(other.pos);

    // Dead actors are seen if the cell is seen
    if (!other.is_alive() &&
        cell.is_seen_by_player)
    {
        return true;
    }

    // Player is blind?
    if (!properties.allow_see())
    {
        return false;
    }

    const Mon* const mon = static_cast<const Mon*>(&other);

    // LOS blocked hard (e.g. a wall)?
    if (cell.player_los.is_blocked_hard)
    {
        return false;
    }

    const bool can_see_invis = properties.has(PropId::see_invis);

    // Monster is invisible, and player cannot see invisible?
    if ((other.properties.has(PropId::invis) ||
         other.properties.has(PropId::cloaked)) &&
        !can_see_invis)
    {
        return false;
    }

    // Blocked by darkness, and not seeing monster with infravision?
    const bool has_darkvision = properties.has(PropId::darkvision);

    const bool can_see_other_in_drk =
        can_see_invis ||
        has_darkvision;

    if (cell.player_los.is_blocked_by_drk &&
        !can_see_other_in_drk)
    {
        return false;
    }

    if (mon->is_sneaking() &&
        !can_see_invis)
    {
        return false;
    }

    // OK, all checks passed, actor can bee seen!
    return true;
}

std::vector<Actor*> Player::seen_actors() const
{
    std::vector<Actor*> out;

    for (Actor* actor : game_time::actors)
    {
        if ((actor != this) &&
            actor->is_alive())
        {
            if (can_see_actor(*actor))
            {
                out.push_back(actor);
            }
        }
    }

    return out;
}

std::vector<Actor*> Player::seen_foes() const
{
    std::vector<Actor*> out;

    for (Actor* actor : game_time::actors)
    {
        if ((actor != this) &&
            actor->is_alive() &&
            map::player->can_see_actor(*actor) &&
            !is_leader_of(actor))
        {
            out.push_back(actor);
        }
    }

    return out;
}

void Player::on_hit(int& dmg,
                    const DmgType dmg_type,
                    const DmgMethod method,
                    const AllowWound allow_wound)
{
    (void)method;

    if (!insanity::has_sympt(InsSymptId::masoch))
    {
        incr_shock(1, ShockSrc::misc);
    }

    const bool is_enough_dmg_for_wound = dmg >= min_dmg_to_wound;
    const bool is_physical = dmg_type == DmgType::physical;

    // Ghoul trait Indomitable Fury makes player immune to Wounds while Frenzied
    const bool is_ghoul_resist_wound =
        player_bon::traits[(size_t)Trait::indomitable_fury] &&
        properties.has(PropId::frenzied);

    if (allow_wound == AllowWound::yes &&
        (hp - dmg) > 0 &&
        is_enough_dmg_for_wound &&
        is_physical &&
        !is_ghoul_resist_wound &&
        !config::is_bot_playing())
    {
        Prop* const prop = new PropWound();

        prop->set_indefinite();

        auto nr_wounds = [&]()
        {
            if (properties.has(PropId::wound))
            {
                const Prop* const prop = properties.prop(PropId::wound);

                const PropWound* const wound =
                    static_cast<const PropWound*>(prop);

                return wound->nr_wounds();
            }

            return 0;
        };

        const int nr_wounds_before = nr_wounds();

        properties.apply(prop);

        const int nr_wounds_after = nr_wounds();

        if (nr_wounds_after > nr_wounds_before)
        {
            if (insanity::has_sympt(InsSymptId::masoch))
            {
                game::add_history_event("Experienced a very pleasant wound.");

                msg_log::add("Hehehe...");

                const double shock_restored = 10.0;

                shock_ = std::max(0.0, shock_ - shock_restored);
            }
            else // Not masochistic
            {
                game::add_history_event("Sustained a severe wound.");
            }
        }
    }
}

int Player::enc_percent() const
{
    const int total_w = inv.total_item_weight();
    const int max_w = carry_weight_lmt();

    return (int)(((double)total_w / (double)max_w) * 100.0);
}

int Player::carry_weight_lmt() const
{
    int carry_weight_mod = 0;

    if (player_bon::has_trait(Trait::strong_backed))
    {
        carry_weight_mod += 50;
    }

    if (properties.has(PropId::weakened))
    {
        carry_weight_mod -= 15;
    }

    return (player_carry_weight_base * (carry_weight_mod + 100)) / 100;
}

int Player::shock_resistance(const ShockSrc shock_src) const
{
    int res = 0;

    if (player_bon::traits[(size_t)Trait::cool_headed])
    {
        res += 20;
    }

    if (player_bon::traits[(size_t)Trait::courageous])
    {
        res += 20;
    }

    if (player_bon::traits[(size_t)Trait::fearless])
    {
        res += 10;
    }

    switch (shock_src)
    {
    case ShockSrc::use_strange_item:
    case ShockSrc::cast_intr_spell:
        if (player_bon::bg() == Bg::occultist)
        {
            res += 50;
        }
        break;

    case ShockSrc::see_mon:
        if (player_bon::bg() == Bg::ghoul)
        {
            res += 50;
        }
        break;

    case ShockSrc::time:
    case ShockSrc::misc:
    case ShockSrc::END:
        break;
    }

    return constr_in_range(0, res, 100);
}

double Player::shock_taken_after_mods(const double base_shock,
                                      const ShockSrc shock_src) const
{
    const double shock_res_db = (double)shock_resistance(shock_src);

    return (base_shock * (100.0 - shock_res_db)) / 100.0;
}

double Player::shock_lvl_to_value(const ShockLvl shock_lvl) const
{
    double shock_value = 0.0;

    switch (shock_lvl)
    {
    case ShockLvl::unsettling:
        shock_value = 2.0;
        break;

    case ShockLvl::frightening:
        shock_value = 4.0;
        break;

    case ShockLvl::terrifying:
        shock_value = 12.0;
        break;

    case ShockLvl::mind_shattering:
        shock_value = 50.0;
        break;

    case ShockLvl::none:
    case ShockLvl::END:
        break;
    }

    return shock_value;
}

void Player::incr_shock(double shock, ShockSrc shock_src)
{
    shock = shock_taken_after_mods(shock, shock_src);

    shock_ += shock;

    perm_shock_taken_current_turn_ += shock;

    shock_ = std::max(0.0, shock_);
}

void Player::incr_shock(const ShockLvl shock_lvl, ShockSrc shock_src)
{
    double shock_value = shock_lvl_to_value(shock_lvl);

    if (shock_value > 0.0)
    {
        incr_shock(shock_value, shock_src);
    }
}

void Player::restore_shock(const int amount_restored,
                           const bool is_temp_shock_restored)
{
    shock_ = std::max(0.0, shock_ - amount_restored);

    if (is_temp_shock_restored)
    {
        shock_tmp_ = 0.0;
    }
}

void Player::incr_insanity()
{
    TRACE << "Increasing insanity" << std::endl;

    if (!config::is_bot_playing())
    {
        const int ins_incr = rnd::range(10, 15);

        ins_ += ins_incr;
    }

    if (ins() >= 100)
    {
        const std::string msg =
            "My mind can no longer withstand what it has grasped. "
            "I am hopelessly lost.";

        popup::msg(msg, "Insane!", SfxId::insanity_rise);

        actor::kill(
                *this,
                IsDestroyed::yes,
                AllowGore::no,
                AllowDropItems::no);

        return;
    }

    // This point reached means insanity is below 100%
    insanity::run_sympt();

    restore_shock(999, true);
}

void Player::item_feeling()
{
        if ((player_bon::bg() != Bg::rogue) ||
            !rnd::percent(80))
        {
                return;
        }

        bool print_feeling = false;

        auto is_nice = [](const Item& item) {
                return item.data().value == ItemValue::supreme_treasure;
        };

        for (auto& cell : map::cells)
        {
                // Nice item on the floor, which is not seen by the player?
                if (cell.item &&
                    is_nice(*cell.item) &&
                    !cell.is_seen_by_player)
                {
                        print_feeling = true;

                        break;
                }

                // Nice item in container?
                const auto& cont_items = cell.rigid->item_container_.items_;

                for (const auto* const item : cont_items)
                {
                        if (is_nice(*item))
                        {
                                print_feeling = true;

                                break;
                        }
                }

                if (print_feeling)
                {
                        const std::string msg =
                                rnd::element(item_feeling_messages_);

                        msg_log::add(
                                msg,
                                colors::light_cyan(),
                                false,
                                MorePromptOnMsg::yes);

                        return;
                }
        } // map cells loop
}

void Player::on_new_dlvl_reached()
{
    mon_feeling();

    item_feeling();

    for (auto& slot: inv.slots)
    {
        // NOTE: The thrown slot never owns the actual item, it is located
        // somewhere else
        if (slot.item && (slot.id != SlotId::thrown))
        {
            slot.item->on_player_reached_new_dlvl();
        }
    }

    for (auto* const item: inv.backpack)
    {
        item->on_player_reached_new_dlvl();
    }
}

void Player::mon_feeling()
{
    if (player_bon::bg() != Bg::rogue)
    {
        return;
    }

    bool print_unique_mon_feeling = false;

    for (Actor* actor : game_time::actors)
    {
        // Not a hostile, living monster?
        if (actor->is_player() ||
            map::player->is_leader_of(actor) ||
            !actor->is_alive())
        {
            continue;
        }

        auto* mon = static_cast<Mon*>(actor);

        // Print monster feeling for new monsters spawned during the level?
        // (We do the actual printing once, after the loop, so that we don't
        // print something silly like "A chill runs down my spine (x2)")
        if (mon->data->is_unique &&
            mon->is_player_feeling_msg_allowed_)
        {
            print_unique_mon_feeling = true;

            mon->is_player_feeling_msg_allowed_ = false;
        }
    }

    if (print_unique_mon_feeling &&
        rnd::percent(80))
    {
        std::vector<std::string> msg_bucket
        {
            "A chill runs down my spine.",
            "I sense a great danger.",
        };

        // This message only makes sense if the player is fearful
        if (!player_bon::traits[(size_t)Trait::fearless] &&
            !properties.has(PropId::frenzied))
        {
            msg_bucket.push_back("I feel anxious.");
        }

        const auto msg = rnd::element(msg_bucket);

        msg_log::add(msg,
                     colors::msg_note(),
                     false,
                     MorePromptOnMsg::yes);
    }
}

void Player::set_quick_move(const Dir dir)
{
    ASSERT(dir != Dir::END);

    quick_move_dir_ = dir;

    has_taken_quick_move_step_ = false;
}

void Player::act()
{
    if (!is_alive())
    {
        return;
    }

    if (properties.on_act() == DidAction::yes)
    {
        return;
    }

    if (tgt_ && (tgt_->state != ActorState::alive))
    {
        tgt_ = nullptr;
    }

    if (active_medical_bag_)
    {
        active_medical_bag_->continue_action();

        return;
    }

    if (handle_armor_countdown_ > 0)
    {
        --handle_armor_countdown_;

        // Done handling armor?
        if (handle_armor_countdown_ == 0)
        {
            // Putting on armor?
            if (armor_putting_on_backpack_idx_ >= 0)
            {
                ASSERT(!inv.slots[(size_t)SlotId::body].item);

                inv.equip_backpack_item(
                        armor_putting_on_backpack_idx_,
                        SlotId::body);

                armor_putting_on_backpack_idx_ = -1;
            }
            // Dropping armor?
            else if (is_dropping_armor_from_body_slot_)
            {
                item_drop::drop_item_from_inv(
                    *map::player,
                    InvType::slots,
                    (size_t)SlotId::body);

                is_dropping_armor_from_body_slot_ = false;
            }
            else // Taking off armor
            {
                ASSERT(inv.slots[(size_t)SlotId::body].item);

                inv.unequip_slot(SlotId::body);
            }
        }
        else // Not done handling armor yet
        {
            game_time::tick();
        }

        return;
    }

    if (wait_turns_left > 0)
    {
        --wait_turns_left;

        move(Dir::center);

        return;
    }

    // Quick move
    if (quick_move_dir_ != Dir::END)
    {
        const bool is_first_quick_move_step = !has_taken_quick_move_step_;

        const P target = pos + dir_utils::offset(quick_move_dir_);

        bool is_target_adj_to_unseen_cell = false;

        for (const P& d : dir_utils::dir_list_w_center)
        {
            const P p_adj(target + d);

            const Cell& adj_cell = map::cells.at(p_adj);

            if (!adj_cell.is_seen_by_player)
            {
                is_target_adj_to_unseen_cell = true;

                break;
            }
        }

        const Cell& target_cell = map::cells.at(target);

        {
            bool should_abort = !target_cell.rigid->can_move(*this);

            if (!should_abort &&
                has_taken_quick_move_step_)
            {
                const auto target_rigid_id = target_cell.rigid->id();

                const bool is_target_known_trap =
                    target_cell.is_seen_by_player &&
                    (target_rigid_id == FeatureId::trap) &&
                    !static_cast<const Trap*>(target_cell.rigid)->is_hidden();

                should_abort =
                    is_target_known_trap ||
                    (target_rigid_id == FeatureId::chains) ||
                    (target_rigid_id == FeatureId::liquid_shallow) ||
                    (target_rigid_id == FeatureId::vines) ||
                    (target_cell.rigid->burn_state_ == BurnState::burning);
            }

            if (should_abort)
            {
                quick_move_dir_ = Dir::END;

                return;
            }
        }

        auto adj_known_closed_doors = [](const P& p)
        {
            std::vector<const Door*> doors;

            for (const P& d : dir_utils::dir_list_w_center)
            {
                const P p_adj(p + d);

                const Cell& adj_cell = map::cells.at(p_adj);

                if (adj_cell.is_seen_by_player &&
                    (adj_cell.rigid->id() == FeatureId::door))
                {
                    const auto* const door =
                        static_cast<const Door*>(adj_cell.rigid);

                    if (!door->is_secret() &&
                        !door->is_open())
                    {
                        doors.push_back(door);
                    }
                }
            }

            return doors;
        };

        const auto adj_known_closed_doors_before = adj_known_closed_doors(pos);

        move(quick_move_dir_);

        has_taken_quick_move_step_ = true;

        update_fov();

        if (quick_move_dir_ == Dir::END)
        {
            return;
        }

        bool should_abort = false;

        if (map::dark.at(pos))
        {
            should_abort = true;
        }

        if (!should_abort)
        {
            const auto adj_known_closed_doors_after =
                adj_known_closed_doors(pos);

            bool is_new_known_adj_closed_door = false;

            for (const auto* const door_after : adj_known_closed_doors_after)
            {
                is_new_known_adj_closed_door =
                    std::find(begin(adj_known_closed_doors_before),
                              end(adj_known_closed_doors_before),
                              door_after) ==
                    end(adj_known_closed_doors_before);

                if (is_new_known_adj_closed_door)
                {
                    break;
                }
            }

            if (!is_first_quick_move_step)
            {
                if (is_target_adj_to_unseen_cell ||
                    is_new_known_adj_closed_door)
                {
                    should_abort = true;
                }
            }
        }

        if (should_abort)
        {
            quick_move_dir_ = Dir::END;
        }

        return;
    }

    // If this point is reached - read input
    if (config::is_bot_playing())
    {
        bot::act();
    }
    else // Not bot playing
    {
        const InputData& input = io::get();

        game::handle_player_input(input);
    }
}

bool Player::is_seeing_burning_feature() const
{
    const R fov_r = fov::fov_rect(pos, map::dims());

    bool is_fire_found = false;

    for (int x = fov_r.p0.x; x <= fov_r.p1.x; ++x)
    {
        for (int y = fov_r.p0.y; y <= fov_r.p1.y; ++y)
        {
            const auto& cell = map::cells.at(x, y);

            if (cell.is_seen_by_player &&
                (cell.rigid->burn_state_ == BurnState::burning))
            {
                is_fire_found = true;

                break;
            }
        }

        if (is_fire_found)
        {
            break;
        }
    }

    return is_fire_found;
}

void Player::on_actor_turn()
{
    reset_perm_shock_taken_current_turn();

    update_fov();

    update_mon_awareness();

    // Set current temporary shock from darkness etc
    update_tmp_shock();

    auto is_busy = [this]() {
            return
                    active_medical_bag_ ||
                    (handle_armor_countdown_ > 0) ||
                    (wait_turns_left > 0) ||
                    (quick_move_dir_ != Dir::END);
    };

    if (is_busy() && is_seeing_burning_feature())
    {
            msg_log::add(msg_fire_prevent_cmd,
                         colors::text(),
                         true);
    }

    Array2<int> vigilant_flood(map::dims());

    if (player_bon::has_trait(Trait::vigilant))
    {
        Array2<bool> blocks_sound(map::dims());

        const int d = 3;

        const R area(
            P(std::max(0, pos.x - d),
              std::max(0, pos.y - d)),
            P(std::min(map::w() - 1, pos.x + d),
              std::min(map::h() - 1, pos.y + d)));

        map_parsers::BlocksSound()
            .run(blocks_sound,
                 area,
                 MapParseMode::overwrite);

        vigilant_flood = floodfill(pos, blocks_sound, d);
    }

    bool is_old_actor_seen = false;

    Actor* actor_to_warn_about = nullptr;

    for (Actor* actor : game_time::actors)
    {
        // Not a hostile, living monster?
        if (actor->is_player() ||
            map::player->is_leader_of(actor) ||
            !actor->is_alive())
        {
            continue;
        }

        Mon& mon = *static_cast<Mon*>(actor);

        const bool is_mon_seen = can_see_actor(*actor);

        if (is_mon_seen)
        {
            mon.is_player_feeling_msg_allowed_ = false;

            if (mon.is_msg_mon_in_view_printed_)
            {
                    is_old_actor_seen = true;
            }

            if (is_busy() ||
                (config::always_warn_new_mon() && !is_old_actor_seen))
            {
                    actor_to_warn_about = actor;
            }
            else
            {
                    actor_to_warn_about = nullptr;
            }

            mon.is_msg_mon_in_view_printed_ = true;
        }
        else // Monster is not seen
        {
            if (mon.player_aware_of_me_counter_ <= 0)
            {
                mon.is_msg_mon_in_view_printed_ = false;
            }

            const bool is_cell_seen = map::cells.at(mon.pos).is_seen_by_player;

            const bool is_mon_invis =
                mon.properties.has(PropId::invis) ||
                mon.properties.has(PropId::cloaked);

            const bool can_player_see_invis =
                properties.has(PropId::see_invis);

            // NOTE: We only run the flodofill within a limited area, so ANY
            // cell reached by the flood is considered as within distance
            const bool is_mon_within_vigilant_dist =
                    vigilant_flood.at(mon.pos) > 0;

            // If the monster is invisible (and the player cannot see invisible
            // monsters), or if the cells is not seen, being Vigilant will make
            // the player aware of the monster if within a certain distance
            if (player_bon::has_trait(Trait::vigilant) &&
                is_mon_within_vigilant_dist &&
                ((is_mon_invis && !can_player_see_invis) ||
                 !is_cell_seen))
            {
                if (mon.player_aware_of_me_counter_ <= 0)
                {
                    if (is_cell_seen)
                    {
                        // The monster must be invisible
                        print_aware_invis_mon_msg(mon);
                    }
                    else // Became aware of a monster in an unseen cell
                    {
                        // Abort quick move
                        quick_move_dir_ = Dir::END;
                    }
                }

                mon.set_player_aware_of_me();
            }
            else if (mon.is_sneaking())
            {
                auto sneak_result = ActionResult::success;

                if (player_bon::has_trait(Trait::vigilant) &&
                    is_mon_within_vigilant_dist)
                {
                    sneak_result = ActionResult::fail;
                }
                // Monster cannot be discovered with Vigilant
                else if (is_cell_seen)
                {
                        SneakData sneak_data;

                        sneak_data.actor_sneaking = &mon;
                        sneak_data.actor_searching = this;

                        sneak_result = actor::roll_sneak(sneak_data);
                }

                if (sneak_result <= ActionResult::fail)
                {
                    mon.set_player_aware_of_me();

                    const std::string mon_name = mon.name_a();

                    msg_log::add(
                            "I spot " + mon_name + "!",
                            colors::msg_note(),
                            true,
                            MorePromptOnMsg::yes);

                    mon.is_msg_mon_in_view_printed_ = true;

                    is_old_actor_seen = true;

                    actor_to_warn_about = nullptr;
                }
            }
        }
    } // actor loop

    if (actor_to_warn_about)
    {
            const std::string name_a =
                    text_format::first_to_upper(
                            actor_to_warn_about->name_a());

            msg_log::add(
                    name_a + " is in my view.",
                    colors::text(),
                    true,
                    MorePromptOnMsg::yes);
    }

    mon_feeling();

    const auto my_seen_foes = seen_foes();

    for (Actor* actor : my_seen_foes)
    {
        static_cast<Mon*>(actor)->set_player_aware_of_me();

        game::on_mon_seen(*actor);
    }

    add_shock_from_seen_monsters();

    if (properties.allow_act())
    {
        // Passive shock taken over time
        double passive_shock_taken = 0.1075;

        if (player_bon::bg() == Bg::rogue)
        {
            passive_shock_taken *= 0.75;
        }

        incr_shock(passive_shock_taken, ShockSrc::time);

        // Passive shock taken over time due to items
        bool is_item_shock_taken = false;

        double item_shock_taken = 0.0;

        for (auto& slot : inv.slots)
        {
            // NOTE: The thrown slot never owns the actual item, it is located
            // somewhere else
            if (slot.item && (slot.id != SlotId::thrown))
            {
                const ItemData& d = slot.item->data();

                // NOTE: Having an item equiped also counts as carrying it
                if (d.is_carry_shocking || d.is_equiped_shocking)
                {
                    item_shock_taken += shock_from_disturbing_items;

                    is_item_shock_taken = true;
                }
            }
        }

        for (const Item* const item : inv.backpack)
        {
            if (item->data().is_carry_shocking)
            {
                item_shock_taken += shock_from_disturbing_items;

                is_item_shock_taken = true;
            }
        }

        if (is_item_shock_taken)
        {
            incr_shock(item_shock_taken, ShockSrc::use_strange_item);
        }
    }

    // Run new turn events on all items
    auto& inv = map::player->inv;

    for (Item* const item : inv.backpack)
    {
        item->on_actor_turn_in_inv(InvType::backpack);
    }

    for (InvSlot& slot : inv.slots)
    {
        // NOTE: The thrown slot never owns the actual item, it is located
        // somewhere else
        if (slot.item  && (slot.id != SlotId::thrown))
        {
            slot.item->on_actor_turn_in_inv(InvType::slots);
        }
    }

    if (!is_alive())
    {
        return;
    }

    // Take sanity hit from high shock?
    if (shock_tot() >= 100)
    {
        nr_turns_until_ins_ =
            (nr_turns_until_ins_ < 0) ?
            3 :
            (nr_turns_until_ins_ - 1);

        if (nr_turns_until_ins_ > 0)
        {
            msg_log::add("I feel my sanity slipping...",
                         colors::msg_note(),
                         true,
                         MorePromptOnMsg::yes);
        }
        else // Time to go crazy!
        {
            nr_turns_until_ins_ = -1;

            incr_insanity();

            if (is_alive())
            {
                game_time::tick();
            }

            return;
        }
    }
    else // Total shock is less than 100%
    {
        nr_turns_until_ins_ = -1;
    }

    insanity::on_new_player_turn(my_seen_foes);
}

void Player::add_shock_from_seen_monsters()
{
    if (!properties.allow_see())
    {
        return;
    }

    double val = 0.0;

    for (Actor* actor : game_time::actors)
    {
        if (actor->is_player() ||
            !actor->is_alive() ||
            (is_leader_of(actor)))
        {
            continue;
        }

        Mon* mon = static_cast<Mon*>(actor);

        if (mon->player_aware_of_me_counter_ <= 0)
        {
            continue;
        }

        ShockLvl shock_lvl = ShockLvl::none;

        if (can_see_actor(*mon))
        {
            shock_lvl = mon->data->mon_shock_lvl;
        }
        else // Monster cannot be seen
        {
            const P mon_p = mon->pos;

            if (map::cells.at(mon_p).is_seen_by_player)
            {
                // There is an invisible monster here! How scary!
                shock_lvl = ShockLvl::terrifying;
            }
        }

        switch (shock_lvl)
        {
        case ShockLvl::unsettling:
            val += 0.05;
            break;

        case ShockLvl::frightening:
            val += 0.3;
            break;

        case ShockLvl::terrifying:
            val += 0.6;
            break;

        case ShockLvl::mind_shattering:
            val += 1.75;
            break;

        case ShockLvl::none:
        case ShockLvl::END:
            break;
        }
    }

    // Dampen the progression (it doesn't seem right that e.g. 8 monsters are
    // twice as scary as 4 monsters).
    val = std::sqrt(val);

    // Cap the value
    const double cap = 5.0;

    val = std::min(cap, val);

    incr_shock(val, ShockSrc::see_mon);
}

void Player::update_tmp_shock()
{
    shock_tmp_ = 0.0;

    const int tot_shock_before = shock_tot();

    // Minimum temporary shock

    // NOTE: In case the total shock is currently at 100, we do NOT want to
    // allow lowering the shock e.g. by turning on the Electric Lantern, since
    // you could interrupt the 3 turns countdown until the insanity event
    // happens just ny turning the lantern on for one turn. Therefore we only
    // allow negative temporary shock while below 100%.
    double shock_tmp_min =
        (tot_shock_before < 100) ?
        -999.0 :
        0.0;

    // "Obessions" raise the minimum temporary shock
    if (insanity::has_sympt(InsSymptId::sadism) ||
        insanity::has_sympt(InsSymptId::masoch))
    {
        shock_tmp_ = std::max(shock_tmp_,
                              (double)shock_from_obsession);

        shock_tmp_min = (double)shock_from_obsession;
    }

    if (properties.allow_see())
    {
        // Shock reduction from light?
        if (map::light.at(pos))
        {
            shock_tmp_ -= 20.0;
        }
        // Not lit - shock from darkness?
        else if (map::dark.at(pos) && (player_bon::bg() != Bg::ghoul))
        {
            double shock_value = 20.0;

            if (insanity::has_sympt(InsSymptId::phobia_dark))
            {
                shock_value = 30.0;
            }

            shock_tmp_ += shock_taken_after_mods(shock_value, ShockSrc::misc);
        }

        // Temporary shock from seen features?
        for (const P& d : dir_utils::dir_list_w_center)
        {
            const P p(pos + d);

            const double feature_shock_db =
                (double)map::cells.at(p).rigid->shock_when_adj();

            shock_tmp_ += shock_taken_after_mods(feature_shock_db,
                                                 ShockSrc::misc);
        }
    }
    // Is blind
    else if (!properties.has(PropId::fainted))
    {
        shock_tmp_ += shock_taken_after_mods(30.0, ShockSrc::misc);
    }

    const double shock_tmp_max = 100.0 - shock_;

    constr_in_range(shock_tmp_min,
                    shock_tmp_,
                    shock_tmp_max);
}

int Player::shock_tot() const
{
    double shock_tot_db = shock_ + shock_tmp_;

    shock_tot_db = std::max(0.0, shock_tot_db);

    shock_tot_db = floor(shock_tot_db);

    int result = (int)shock_tot_db;

    result = properties.affect_shock(result);

    return result;
}

int Player::ins() const
{
    int result = ins_;

    result = std::min(100, result);

    return result;
}

void Player::on_std_turn()
{
#ifndef NDEBUG
    // Sanity check: Disease and infection should not be active at the same time
    ASSERT(!properties.has(PropId::diseased) ||
           !properties.has(PropId::infected));
#endif // NDEBUG

    if (!is_alive())
    {
        return;
    }

    // Spell resistance
    const int spell_shield_turns_base = 125 + rnd::range(0, 25);

    const int spell_shield_turns_bon =
            player_bon::has_trait(Trait::mighty_spirit) ? 100 :
            player_bon::has_trait(Trait::strong_spirit) ? 50 : 0;

    int nr_turns_to_recharge_spell_shield = std::max(
            1,
            spell_shield_turns_base - spell_shield_turns_bon);

    // Halved number of turns due to the Talisman of Reflection?
    if (inv.has_item_in_backpack(ItemId::refl_talisman))
    {
        nr_turns_to_recharge_spell_shield /= 2;
    }

    // If we already have spell resistance (e.g. due to the Spell Shield spell),
    // then always (re)set the cooldown to max number of turns
    if (properties.has(PropId::r_spell))
    {
        nr_turns_until_rspell_ = nr_turns_to_recharge_spell_shield;
    }
    // Spell resistance not currently active
    else if (player_bon::has_trait(Trait::stout_spirit))
    {
        if (nr_turns_until_rspell_ <= 0)
        {
            // Cooldown has finished, OR countdown not initialized

            if (nr_turns_until_rspell_ == 0)
            {
                // Cooldown has finished
                auto prop = new PropRSpell();

                prop->set_indefinite();

                properties.apply(prop);

                msg_log::more_prompt();
            }

            nr_turns_until_rspell_ = nr_turns_to_recharge_spell_shield;
        }

        if (!properties.has(PropId::r_spell) &&
            (nr_turns_until_rspell_ > 0))
        {
            // Spell resistance is in cooldown state, decrement number of
            // remaining turns
            --nr_turns_until_rspell_;
        }
    }

    if (active_explosive_)
    {
        active_explosive_->on_std_turn_player_hold_ignited();
    }

    // Regenerate Hit Points
    if (!properties.has(PropId::poisoned) &&
        player_bon::bg() != Bg::ghoul)
    {
        int nr_turns_per_hp;

        // Rapid Recoverer trait affects hp regen?
        if (player_bon::traits[(size_t)Trait::rapid_recoverer])
        {
            nr_turns_per_hp = 2;
        }
        else
        {
            nr_turns_per_hp = 20;
        }

        // Wounds affect hp regen?
        int nr_wounds = 0;

        if (properties.has(PropId::wound))
        {
            Prop* const prop = properties.prop(PropId::wound);

            nr_wounds = static_cast<PropWound*>(prop)->nr_wounds();
        }

        const bool is_survivalist =
            player_bon::traits[(size_t)Trait::survivalist];

        const int wound_effect_div = is_survivalist ? 2 : 1;

        nr_turns_per_hp +=
            ((nr_wounds * 4) / wound_effect_div);

        // Items affect hp regen?
        for (const auto& slot : inv.slots)
        {
            // NOTE: The thrown slot never owns the actual item, it is located
            // somewhere else
            if (slot.item && (slot.id != SlotId::thrown))
            {
                nr_turns_per_hp += slot.item->hp_regen_change(InvType::slots);
            }
        }

        for (const Item* const item : inv.backpack)
        {
            nr_turns_per_hp += item->hp_regen_change(InvType::backpack);
        }

        nr_turns_per_hp = std::max(1, nr_turns_per_hp);

        const int turn = game_time::turn_nr();
        const int max_hp = actor::max_hp(*this);

        if ((hp < max_hp) &&
            ((turn % nr_turns_per_hp) == 0) &&
            turn > 1)
        {
            ++hp;
        }
    }

    // Try to spot hidden traps and doors

    // NOTE: Skill value retrieved here is always at least 1
    const int player_search_skill =
        map::player->ability(AbilityId::searching, true);

    if (!properties.has(PropId::confused) &&
        properties.allow_see())
    {
        for (size_t i = 0; i < map::cells.length(); ++i)
        {
                if (!map::cells.at(i).is_seen_by_player)
                {
                        continue;
                }

                auto* f = map::cells.at(i).rigid;

                const int lit_mod = map::light.at(i) ? 5 : 0;

                const int dist = king_dist(pos, f->pos());

                const int dist_mod = -((dist - 1) * 5);

                int skill_tot =
                        player_search_skill +
                        lit_mod +
                        dist_mod;

                if (skill_tot > 0)
                {
                        const bool is_spotted =
                                ability_roll::roll(skill_tot) >=
                                ActionResult::success;

                        if (is_spotted)
                        {
                                f->reveal(Verbosity::verbose);
                        }
                }
        }
    }
}

void Player::on_log_msg_printed()
{
    // NOTE: There cannot be any calls to msg_log::add() in this function, as
    // that would cause infinite recursion!

    // All messages abort waiting
    wait_turns_left = -1;

    // All messages abort quick move
    quick_move_dir_ = Dir::END;
}

void Player::interrupt_actions()
{
    // Abort healing
    if (active_medical_bag_)
    {
        active_medical_bag_->interrupted();
        active_medical_bag_ = nullptr;
    }

    // Abort putting on / taking off armor?
    if (handle_armor_countdown_ > 0)
    {
        bool should_continue = true;

        if (properties.has(PropId::burning))
        {
            should_continue = false;
        }

        if (should_continue)
        {
            const std::string turns_left_str =
                std::to_string(handle_armor_countdown_);

            std::string msg = "";

            if (armor_putting_on_backpack_idx_ >= 0)
            {
                auto* const item =
                        inv.backpack[armor_putting_on_backpack_idx_];

                const std::string armor_name =
                    item->name(ItemRefType::a, ItemRefInf::yes);

                msg =
                        "Putting on " + armor_name +
                        " (" + turns_left_str + " turns left).";
            }
            else // Taking off armor, or dropping from armor slot
            {
                auto* const item = inv.item_in_slot(SlotId::body);

                ASSERT(item);

                const std::string armor_name =
                        item->name(ItemRefType::a, ItemRefInf::yes);

                msg =
                        "Taking off " + armor_name +
                        " (" + turns_left_str + " turns left).";
            }

            msg_log::add(msg, colors::light_white());

            msg_log::add("Continue? [y/n]", colors::light_white());

            should_continue = (query::yes_or_no() == BinaryAnswer::yes);

            msg_log::clear();
        }

        if (!should_continue)
        {
            handle_armor_countdown_ = 0;

            armor_putting_on_backpack_idx_ = -1;

            is_dropping_armor_from_body_slot_ = false;
        }
    }

    // Abort waiting
    wait_turns_left = -1;

    // Abort quick move
    quick_move_dir_ = Dir::END;
}

void Player::hear_sound(const Snd& snd,
                        const bool is_origin_seen_by_player,
                        const Dir dir_to_origin,
                        const int percent_audible_distance)
{
    (void)is_origin_seen_by_player;

    if (properties.has(PropId::deaf))
    {
        return;
    }

    const SfxId sfx = snd.sfx();

    const std::string& msg = snd.msg();

    const bool has_snd_msg = !msg.empty() && msg != " ";

    if (has_snd_msg)
    {
        msg_log::add(msg,
                     colors::text(),
                     false,
                     snd.should_add_more_prompt_on_msg());
    }

    // Play audio after message to ensure sync between audio and animation.
    audio::play(sfx, dir_to_origin, percent_audible_distance);

    if (has_snd_msg)
    {
        Actor* const actor_who_made_snd = snd.actor_who_made_sound();

        if (actor_who_made_snd &&
            (actor_who_made_snd != this))
        {
            static_cast<Mon*>(actor_who_made_snd)->set_player_aware_of_me();
        }
    }

    snd.on_heard(*this);
}

void Player::move(Dir dir)
{
    if (!is_alive())
    {
        return;
    }

    const Dir intended_dir = dir;

    properties.affect_move_dir(pos, dir);

    const P tgt(pos + dir_utils::offset(dir));

    // Attacking, bumping stuff, staggering from encumbrance, etc
    if (dir != Dir::center)
    {
        // Check if map features are blocking (used later)
        Cell& cell = map::cells.at(tgt);

        bool is_features_allow_move = cell.rigid->can_move(*this);

        std::vector<Mob*> mobs;
        game_time::mobs_at_pos(tgt, mobs);

        if (is_features_allow_move)
        {
            for (auto* m : mobs)
            {
                if (!m->can_move(*this))
                {
                    is_features_allow_move = false;
                    break;
                }
            }
        }

        Mon* const mon = static_cast<Mon*>(map::actor_at_pos(tgt));

        // Hostile monster here?
        if (mon && !is_leader_of(mon))
        {
            const bool can_see_mon = map::player->can_see_actor(*mon);

            const bool is_aware_of_mon = mon->player_aware_of_me_counter_ > 0;

            if (is_aware_of_mon)
            {
                if (properties.allow_attack_melee(Verbosity::verbose))
                {
                    Item* const wpn_item = inv.item_in_slot(SlotId::wpn);

                    if (wpn_item)
                    {
                        Wpn* const wpn = static_cast<Wpn*>(wpn_item);

                        // If this is also a ranged weapon, ask if player really
                        // intended to use it as melee weapon
                        if (wpn->data().ranged.is_ranged_wpn &&
                            config::is_ranged_wpn_meleee_prompt())
                        {
                            const std::string wpn_name =
                                wpn->name(ItemRefType::a);

                            const std::string mon_name =
                                    can_see_mon
                                    ? mon->name_the()
                                    : "it";

                            msg_log::add(
                                    "Attacking " +
                                    mon_name +
                                    " with " +
                                    wpn_name +
                                    ".",
                                    colors::light_white());

                            msg_log::add(
                                    "Continue? [y/n]",
                                    colors::light_white());

                            if (query::yes_or_no() == BinaryAnswer::no)
                            {
                                msg_log::clear();

                                return;
                            }
                        }

                        attack::melee(this,
                                      pos,
                                      *mon,
                                      *wpn);

                        tgt_ = mon;

                        return;
                    }
                    else // No melee weapon wielded
                    {
                        hand_att(*mon);
                    }
                }

                return;
            }
            else // There is a monster here that player is unaware of
            {
                // If player is unaware of the monster, it should never be seen
                // ASSERT(!can_see_mon);

                if (is_features_allow_move)
                {
                    // Cell is not blocked, reveal monster here and return
                    mon->set_player_aware_of_me();

                    print_aware_invis_mon_msg(*mon);

                    return;
                }

                // NOTE: The target is blocked by map features. Do NOT reveal
                // the monster - just act like the monster isn't there, and let
                // the code below handle the situation.
            }
        }

        if (is_features_allow_move)
        {
            // Encumbrance, wounds, or spraining affecting movement
            const int enc = enc_percent();

            Prop* const wound_prop = properties.prop(PropId::wound);

            int nr_wounds = 0;

            if (wound_prop)
            {
                nr_wounds = static_cast<PropWound*>(wound_prop)->nr_wounds();
            }

            const int min_nr_wounds_for_stagger = 3;

            // Cannot move at all due to encumbrance?
            if (enc >= enc_immobile_lvl)
            {
                msg_log::add("I am too encumbered to move!");

                return;
            }
            // Move at half speed due to encumbrance or wounds?
            else if ((enc >= 100) ||
                     (nr_wounds >= min_nr_wounds_for_stagger))
            {
                msg_log::add("I stagger.", colors::msg_note());

                properties.apply(new PropWaiting());
            }

            // Displace allied monster
            if (mon && is_leader_of(mon))
            {
                if (mon->player_aware_of_me_counter_ > 0)
                {
                    std::string mon_name =
                        can_see_actor(*mon) ?
                        mon->name_a() :
                        "it";

                    msg_log::add("I displace " + mon_name + ".");
                }

                mon->pos = pos;
            }

            map::cells.at(pos).rigid->on_leave(*this);

            pos = tgt;

            // Walking on item?
            Item* const item = map::cells.at(pos).item;

            if (item)
            {
                // Only print the item name if the item will not be "found" by
                // stepping on it, otherwise there would be redundant messages,
                // e.g. "A Muddy Potion." -> "I have found a Muddy Potion!"
                if ((item->data().xp_on_found <= 0) ||
                    item->data().is_found)
                {
                    std::string item_name =
                        item->name(ItemRefType::plural,
                                   ItemRefInf::yes,
                                   ItemRefAttInf::wpn_main_att_mode);

                    item_name = text_format::first_to_upper(item_name);

                    msg_log::add(item_name + ".");
                }

                item->on_player_found();
            }

            // Print message if walking on corpses
            for (auto* const actor : game_time::actors)
            {
                if ((actor->pos == pos) &&
                    (actor->state == ActorState::corpse))
                {
                    const std::string name =
                        text_format::first_to_upper(
                            actor->data->corpse_name_a);

                    msg_log::add(name + ".");
                }
            }
        }

        // NOTE: bump() prints block messages.
        for (auto* mob : mobs)
        {
            mob->bump(*this);
        }

        map::cells.at(tgt).rigid->bump(*this);
    }

    // If position is at the destination now, it means that the player either:
    // * did an actual move to another position, or
    // * that player waited in the current position on purpose, or
    // * that the player was stuck (e.g. in a spider web)
    // In either case, the game time is ticked here (since no melee attack or
    // other "time advancing" action has occurred)
    if (pos == tgt)
    {
        // If the player intended to wait in the current position, perform
        // "standing still" actions
        if (intended_dir == Dir::center)
        {
            auto did_action = DidAction::no;

            // Ghoul feed on corpses?
            if (player_bon::bg() == Bg::ghoul)
            {
                    did_action = try_eat_corpse();
            }

            if (did_action == DidAction::no)
            {
                    // Reorganize pistol magazines?
                    const auto my_seen_foes = seen_foes();

                    if (my_seen_foes.empty())
                    {
                            reload::player_arrange_pistol_mags();
                    }
            }
        }

        game_time::tick();
    }
}

Color Player::color() const
{
    if (!is_alive())
    {
        return colors::red();
    }

    if (active_explosive_)
    {
        return colors::yellow();
    }

    Color tmp_color;

    if (properties.affect_actor_color(tmp_color))
    {
        return tmp_color;
    }

    const auto* const lantern_item = inv.item_in_backpack(ItemId::lantern);

    if (lantern_item)
    {
            const auto* const lantern =
                    static_cast<const DeviceLantern*>(lantern_item);

            if (lantern->is_activated)
            {
                    return colors::yellow();
            }
    }

    if (shock_tot() >= 75)
    {
        return colors::magenta();
    }

    if (properties.has(PropId::invis) ||
        properties.has(PropId::cloaked))
    {
        return colors::gray();
    }

    return data->color;
}

SpellSkill Player::spell_skill(const SpellId id) const
{
    return player_spells::spell_skill(id);
}

void Player::auto_melee()
{
    if (tgt_ &&
        tgt_->state == ActorState::alive &&
        is_pos_adj(pos, tgt_->pos, false) &&
        can_see_actor(*tgt_))
    {
        move(dir_utils::dir(tgt_->pos - pos));
        return;
    }

    // If this line reached, there is no adjacent cur target.
    for (const P& d : dir_utils::dir_list)
    {
        Actor* const actor = map::actor_at_pos(pos + d);

        if (actor &&
            !is_leader_of(actor) &&
            can_see_actor(*actor))
        {
            tgt_ = actor;
            move(dir_utils::dir(d));
            return;
        }
    }
}

void Player::kick_mon(Actor& defender)
{
    Wpn* kick_wpn = nullptr;

    const ActorData& d = *defender.data;

    // TODO: This is REALLY hacky, it should be done another way. Why even have
    // a "stomp" attack?? Why not just kick them as well?
    if (d.actor_size == ActorSize::floor &&
        (d.is_spider ||
         d.is_rat ||
         d.is_snake ||
         d.id == ActorId::worm_mass ||
         d.id == ActorId::mind_worms))
    {
        kick_wpn = static_cast<Wpn*>(item_factory::make(ItemId::player_stomp));
    }
    else
    {
        kick_wpn = static_cast<Wpn*>(item_factory::make(ItemId::player_kick));
    }

    attack::melee(this,
                  pos,
                  defender,
                  *kick_wpn);

    delete kick_wpn;
}

Wpn& Player::unarmed_wpn()
{
    ASSERT(unarmed_wpn_);

    return *unarmed_wpn_;
}

void Player::hand_att(Actor& defender)
{
    Wpn& wpn = unarmed_wpn();

    attack::melee(this, pos, defender, wpn);
}

void Player::add_light_hook(Array2<bool>& light_map) const
{
    LgtSize lgt_size = LgtSize::none;

    if (active_explosive_)
    {
        if (active_explosive_->data().id == ItemId::flare)
        {
            lgt_size = LgtSize::fov;
        }
    }

    if (lgt_size != LgtSize::fov)
    {
        for (Item* const item : inv.backpack)
        {
            LgtSize item_lgt_size = item->lgt_size();

            if ((int)lgt_size < (int)item_lgt_size)
            {
                lgt_size = item_lgt_size;
            }
        }
    }

    switch (lgt_size)
    {
    case LgtSize::fov:
    {
         Array2<bool> hard_blocked(map::dims());

         const R fov_lmt = fov::fov_rect(pos, hard_blocked.dims());

        map_parsers::BlocksLos()
            .run(hard_blocked,
                 fov_lmt,
                 MapParseMode::overwrite);

        FovMap fov_map;
        fov_map.hard_blocked = &hard_blocked;
        fov_map.light = &map::light;
        fov_map.dark = &map::dark;

        const auto player_fov = fov::run(pos, fov_map);

        for (int x = fov_lmt.p0.x; x <= fov_lmt.p1.x; ++x)
        {
            for (int y = fov_lmt.p0.y; y <= fov_lmt.p1.y; ++y)
            {
                if (!player_fov.at(x, y).is_blocked_hard)
                {
                    light_map.at(x, y) = true;
                }
            }
        }
    }
    break;

    case LgtSize::small:
        for (int y = pos.y - 1; y <= pos.y + 1; ++y)
        {
            for (int x = pos.x - 1; x <= pos.x + 1; ++x)
            {
                light_map.at(x, y) = true;
            }
        }
        break;

    case LgtSize::none:
        break;
    }
}

void Player::update_fov()
{
    for (auto& cell : map::cells)
    {
        cell.is_seen_by_player = false;

        cell.player_los.is_blocked_hard = true;

        cell.player_los.is_blocked_by_drk = false;
    }

    const bool has_darkvision = properties.has(PropId::darkvision);

    if (properties.allow_see())
    {
         Array2<bool> hard_blocked(map::dims());

         const R fov_lmt = fov::fov_rect(pos, hard_blocked.dims());

        map_parsers::BlocksLos()
            .run(hard_blocked,
                 fov_lmt,
                 MapParseMode::overwrite);

        FovMap fov_map;
        fov_map.hard_blocked = &hard_blocked;
        fov_map.light = &map::light;
        fov_map.dark = &map::dark;

        const auto player_fov = fov::run(pos, fov_map);

        for (int x = fov_lmt.p0.x; x <= fov_lmt.p1.x; ++x)
        {
            for (int y = fov_lmt.p0.y; y <= fov_lmt.p1.y; ++y)
            {
                const LosResult& los = player_fov.at(x, y);

                Cell& cell = map::cells.at(x, y);

                cell.is_seen_by_player =
                    !los.is_blocked_hard &&
                    (!los.is_blocked_by_drk || has_darkvision);

                cell.player_los = los;

#ifndef NDEBUG
                // Sanity check - if the cell is ONLY blocked by darkness
                // (i.e. not by a wall or other blocking feature), it should NOT
                // be lit
                if (!los.is_blocked_hard && los.is_blocked_by_drk)
                {
                        ASSERT(!map::light.at(x, y));
                }
#endif // NDEBUG
            }
        }

        fov_hack();
    }

    // The player's current cell is always seen - mostly to update item info
    // while blind (i.e. when you pick up an item you should see it disappear)
    map::cells.at(pos).is_seen_by_player = true;

    // Cheat vision
    if (init::is_cheat_vision_enabled)
    {
        // Show all cells adjacent to cells which can be shot or seen through
        Array2<bool> reveal(map::dims());

        map_parsers::BlocksProjectiles()
                .run(reveal, reveal.rect());

        map_parsers::BlocksLos()
            .run(reveal,
                 reveal.rect(),
                 MapParseMode::append);

        for (auto& reveal_cell : reveal)
        {
                reveal_cell = !reveal_cell;
        }

        const auto reveal_expanded =
                map_parsers::expand(reveal, reveal.rect());

        for (size_t i = 0; i < map::cells.length(); ++i)
        {
            if (reveal_expanded.at(i))
            {
                map::cells.at(i).is_seen_by_player = true;
            }
        }
    }

    // Explore
    for (int x = 0; x < map::w(); ++x)
    {
        for (int y = 0; y < map::h(); ++y)
        {
            const bool allow_explore =
                !map::dark.at(x, y) ||
                map_parsers::BlocksLos().cell(P(x, y)) ||
                map_parsers::BlocksWalking(ParseActors::no ).cell(P(x, y)) ||
                has_darkvision;

            Cell& cell = map::cells.at(x, y);

            // Do not explore dark floor cells
            if (cell.is_seen_by_player && allow_explore)
            {
                cell.is_explored = true;
            }
        }
    }

    minimap::update();
}

void Player::fov_hack()
{
    Array2<bool> blocked_los(map::dims());

    map_parsers::BlocksLos()
            .run(blocked_los, blocked_los.rect());

    Array2<bool> blocked(map::dims());

    map_parsers::BlocksWalking(ParseActors::no)
            .run(blocked, blocked.rect());

        const std::vector<FeatureId> free_features =
        {
                FeatureId::liquid_deep,
                FeatureId::chasm
        };

        for (int x = 0; x < blocked.w(); ++x)
        {
                for (int y = 0; y < blocked.h(); ++y)
                {
                        const P p(x, y);

                        if (map_parsers::IsAnyOfFeatures(free_features).cell(p))
                        {
                                blocked.at(p) = false;
                        }
                }
        }

    const bool has_darkvision = properties.has(PropId::darkvision);

    for (int x = 0; x < map::w(); ++x)
    {
        for (int y = 0; y < map::h(); ++y)
        {
            if (blocked_los.at(x, y) && blocked.at(x, y))
            {
                const P p(x, y);

                for (const P& d : dir_utils::dir_list)
                {
                    const P p_adj(p + d);

                    if (map::is_pos_inside_map(p_adj) &&
                        map::cells.at(p_adj).is_seen_by_player)
                    {
                        const bool allow_explore =
                                (!map::dark.at(p_adj) ||
                                 map::light.at(p_adj) ||
                                 has_darkvision) &&
                                !blocked.at(p_adj);

                        if (allow_explore)
                        {
                            Cell& cell = map::cells.at(x, y);

                            cell.is_seen_by_player = true;

                            cell.player_los.is_blocked_hard = false;

                            break;
                        }
                    }
                }
            }
        }
    }
}

void Player::update_mon_awareness()
{
    const auto my_seen_actors = seen_actors();

    for (Actor* const actor : my_seen_actors)
    {
        static_cast<Mon*>(actor)->set_player_aware_of_me();
    }
}

bool Player::is_leader_of(const Actor* const actor) const
{
    if (!actor || (actor == this))
    {
        return false;
    }

    // Actor is monster

    return static_cast<const Mon*>(actor)->leader_ == this;
}

bool Player::is_actor_my_leader(const Actor* const actor) const
{
    (void)actor;

    // Should never happen
    ASSERT(false);

    return false;
}
