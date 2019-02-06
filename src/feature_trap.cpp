// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "feature_trap.hpp"

#include <algorithm>

#include "actor_factory.hpp"
#include "actor_hit.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "attack.hpp"
#include "common_text.hpp"
#include "drop.hpp"
#include "explosion.hpp"
#include "feature_data.hpp"
#include "init.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "popup.hpp"
#include "postmortem.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "query.hpp"
#include "sound.hpp"
#include "teleport.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Trap
// -----------------------------------------------------------------------------
Trap::Trap(const P& feature_pos,
           Rigid* const mimic_feature,
           TrapId id) :
        Rigid(feature_pos),
        mimic_feature_(mimic_feature),
        is_hidden_(true)
{
        ASSERT(id != TrapId::END);

        auto* const rigid_here = map::cells.at(feature_pos).rigid;

        if (!rigid_here->can_have_rigid())
        {
                TRACE << "Cannot place trap on feature id: "
                      << (int)rigid_here->id()
                      << std::endl
                      << "Trap id: " << (int)id
                      << std::endl;

                ASSERT(false);
                return;
        }

        auto try_place_trap_or_discard = [&](const TrapId trap_id) {
                auto* impl = make_trap_impl_from_id(trap_id);

                auto valid = impl->on_place();

                if (valid == TrapPlacementValid::yes)
                {
                        trap_impl_ = impl;
                }
                else // Placement not valid
                {
                        delete impl;
                }
        };

        if (id == TrapId::any)
        {
                // Attempt to set a trap implementation until succeeding
                while (true)
                {
                        const auto random_id =
                                (TrapId)rnd::range(0, (int)TrapId::END - 1);

                        try_place_trap_or_discard(random_id);

                        if (trap_impl_)
                        {
                                // Trap placement is good!
                                break;
                        }
                }
        }
        else // Make a specific trap type
        {
                // NOTE: This may fail, in which case we have no trap
                // implementation. The trap creator is responsible for handling
                // this situation.
                try_place_trap_or_discard(id);
        }
}

Trap::~Trap()
{
        delete trap_impl_;
        delete mimic_feature_;
}

TrapImpl* Trap::make_trap_impl_from_id(const TrapId trap_id)
{
        switch (trap_id)
        {
        case TrapId::dart:
                return new TrapDart(pos_, this);
                break;

        case TrapId::spear:
                return new TrapSpear(pos_, this);
                break;

        case TrapId::gas_confusion:
                return new TrapGasConfusion(pos_, this);
                break;

        case TrapId::gas_paralyze:
                return new TrapGasParalyzation(pos_, this);
                break;

        case TrapId::gas_fear:
                return new TrapGasFear(pos_, this);
                break;

        case TrapId::blinding:
                return new TrapBlindingFlash(pos_, this);
                break;

        case TrapId::deafening:
                return new TrapDeafening(pos_, this);
                break;

        case TrapId::teleport:
                return new TrapTeleport(pos_, this);
                break;

        case TrapId::summon:
                return new TrapSummonMon(pos_, this);
                break;

        case TrapId::spi_drain:
                return new TrapSpiDrain(pos_, this);
                break;

        case TrapId::smoke:
                return new TrapSmoke(pos_, this);
                break;

        case TrapId::fire:
                return new TrapFire(pos_, this);
                break;

        case TrapId::alarm:
                return new TrapAlarm(pos_, this);
                break;

        case TrapId::web:
                return new TrapWeb(pos_, this); break;

        case TrapId::slow:
                return new TrapSlow(pos_, this);

        case TrapId::curse:
                return new TrapCurse(pos_, this);

        case TrapId::END:
        case TrapId::any:
                break;
        }

        return nullptr;
}

void Trap::on_hit(const int dmg,
                  const DmgType dmg_type,
                  const DmgMethod dmg_method,
                  Actor* const actor)
{
        (void)dmg;
        (void)dmg_type;
        (void)dmg_method;
        (void)actor;
}

TrapId Trap::type() const
{
        ASSERT(trap_impl_);
        return trap_impl_->type_;
}

bool Trap::is_magical() const
{
        ASSERT(trap_impl_);

        return trap_impl_->is_magical();
}

void Trap::on_new_turn_hook()
{
        if (nr_turns_until_trigger_ > 0)
        {
                --nr_turns_until_trigger_;

                TRACE_VERBOSE << "Number of turns until trigger: "
                              << nr_turns_until_trigger_ << std::endl;

                if (nr_turns_until_trigger_ == 0)
                {
                        // NOTE: This will reset number of turns until triggered
                        trigger_trap(nullptr);
                }
        }
}

void Trap::trigger_start(const Actor* actor)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        ASSERT(trap_impl_);

        if (actor == map::player)
        {
                // Reveal trap if triggered by player stepping on it
                reveal(Verbosity::silent);

                map::player->update_fov();

                states::draw();
        }

        if (is_magical())
        {
                // TODO: Play sfx for magic traps (if player)
        }
        else // Not magical
        {
                if (type() != TrapId::web)
                {
                        std::string msg = "I hear a click.";

                        auto alerts = AlertsMon::no;

                        if (actor == map::player)
                        {
                                alerts = AlertsMon::yes;

                                // If player is triggering, make the message a
                                // bit more "foreboding"
                                msg += "..";
                        }

                        // TODO: Make a sound effect for this
                        Snd snd(msg,
                                SfxId::END,
                                IgnoreMsgIfOriginSeen::no,
                                pos_,
                                nullptr,
                                SndVol::low,
                                alerts);

                        snd_emit::run(snd);

                        if (actor == map::player)
                        {
                                if (map::player->properties.has(PropId::deaf))
                                {
                                        msg_log::add(
                                                "I feel the ground shifting "
                                                "slightly under my foot.");
                                }

                                msg_log::more_prompt();
                        }
                }
        }

        // Get a randomized value for number of remaining turns
        const Range turns_range = trap_impl_->nr_turns_range_to_trigger();

        const int rnd_nr_turns = turns_range.roll();

        // Set number of remaining turns to the randomized value if not set
        // already, or if the new value will make it trigger sooner
        if ((nr_turns_until_trigger_ == -1) ||
            (rnd_nr_turns < nr_turns_until_trigger_))
        {
                nr_turns_until_trigger_ = rnd_nr_turns;
        }

        TRACE_VERBOSE << "nr_turns_until_trigger_: "
                      << nr_turns_until_trigger_
                      << std::endl;

        ASSERT(nr_turns_until_trigger_ > -1);

        // If number of remaining turns is zero, trigger immediately
        if (nr_turns_until_trigger_ == 0)
        {
                // NOTE: This will reset number of turns until triggered
                trigger_trap(nullptr);
        }

        TRACE_FUNC_END_VERBOSE;
}

AllowAction Trap::pre_bump(Actor& actor_bumping)
{
        if (!actor_bumping.is_player() ||
            actor_bumping.properties.has(PropId::confused))
        {
                return AllowAction::yes;
        }

        if (map::cells.at(pos_).is_seen_by_player &&
            !is_hidden_ &&
            !actor_bumping.properties.has(PropId::ethereal) &&
            !actor_bumping.properties.has(PropId::flying))
        {
                // The trap is known, and will be triggered by the player

                const std::string name_the = name(Article::the);

                msg_log::add(
                        std::string(
                                "Step into " +
                                name_the +
                                "? " +
                                common_text::yes_or_no_hint),
                        colors::light_white());

                const auto query_result = query::yes_or_no();

                msg_log::clear();

                return
                        (query_result == BinaryAnswer::no)
                        ? AllowAction::no
                        : AllowAction::yes;
        }
        else
        {
                // The trap is unknown, or will not be triggered by the player -
                // delegate the question to the mimicked feature

                const auto result = mimic_feature_->pre_bump(actor_bumping);

                return result;
        }
}

void Trap::bump(Actor& actor_bumping)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        const ActorData& d = *actor_bumping.data;

        if (actor_bumping.properties.has(PropId::ethereal) ||
            actor_bumping.properties.has(PropId::flying) ||
            (d.actor_size < ActorSize::humanoid) ||
            d.is_spider)
        {
                TRACE_FUNC_END_VERBOSE;

                return;
        }

        if (!actor_bumping.is_player())
        {
                Mon* const mon = static_cast<Mon*>(&actor_bumping);

                if (mon->aware_of_player_counter_ <= 0)
                {
                        TRACE_FUNC_END_VERBOSE;

                        return;
                }
        }

        trigger_start(&actor_bumping);

        TRACE_FUNC_END_VERBOSE;
}

void Trap::disarm()
{
        if (is_magical() && (player_bon::bg() != Bg::occultist))
        {
                msg_log::add("I do not know how to dispel magic traps.");

                return;
        }

        msg_log::add(trap_impl_->disarm_msg());

        destroy();
}

void Trap::destroy()
{
        ASSERT(mimic_feature_);

        // Magical traps and webs simply "dissapear" (place their mimic
        // feature), and mechanical traps puts rubble.

        if (is_magical() || type() == TrapId::web)
        {
                Rigid* const f_tmp = mimic_feature_;

                mimic_feature_ = nullptr;

                // NOTE: This call destroys the object!
                map::put(f_tmp);
        }
        else // "Mechanical" trap
        {
                map::put(new RubbleLow(pos_));
        }
}

DidTriggerTrap Trap::trigger_trap(Actor* const actor)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        (void)actor;

        TRACE_VERBOSE << "Name of trap triggering: "
                      << trap_impl_->name(Article::a)
                      << std::endl;

        nr_turns_until_trigger_ = -1;

        TRACE_VERBOSE << "Calling trigger in trap implementation" << std::endl;

        trap_impl_->trigger();

        // NOTE: This object may now be deleted (e.g. a web was torn down)!

        TRACE_FUNC_END_VERBOSE;
        return DidTriggerTrap::yes;
}

void Trap::reveal(const Verbosity verbosity)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (!is_hidden_)
        {
                return;
        }

        is_hidden_ = false;

        clear_gore();

        if (map::cells.at(pos_).is_seen_by_player)
        {
                states::draw();

                if (verbosity == Verbosity::verbose)
                {
                        std::string msg = "";

                        const std::string trap_name_a =
                                trap_impl_->name(Article::a);

                        if (pos_ == map::player->pos)
                        {
                                msg += "There is " + trap_name_a + " here!";
                        }
                        else // Trap is not at player position
                        {
                                msg = "I spot " + trap_name_a + ".";
                        }

                        msg_log::add(msg);
                }
        }

        TRACE_FUNC_END_VERBOSE;
}

std::string Trap::name(const Article article) const
{
        if (is_hidden_)
        {
                return mimic_feature_->name(article);
        }
        else // Not hidden
        {
                return trap_impl_->name(article);
        }
}

Color Trap::color_default() const
{
        return
                is_hidden_
                ? mimic_feature_->color()
                : trap_impl_->color();
}

Color Trap::color_bg_default() const
{
        const auto* const item = map::cells.at(pos_).item;

        const auto* const corpse = map::actor_at_pos(pos_, ActorState::corpse);

        if (!is_hidden_ && (item || corpse))
        {
                return trap_impl_->color();
        }
        else // Is hidden, or nothing is over the trap
        {
                return colors::black();
        }
}

char Trap::character() const
{
        return
                is_hidden_
                ? mimic_feature_->character()
                : trap_impl_->character();
}

TileId Trap::tile() const
{
        return
                is_hidden_
                ? mimic_feature_->tile()
                : trap_impl_->tile();
}

Matl Trap::matl() const
{
        return
                is_hidden_
                ? mimic_feature_->matl()
                : data().matl_type;
}

// -----------------------------------------------------------------------------
// Trap implementations
// -----------------------------------------------------------------------------
TrapPlacementValid MagicTrapImpl::on_place()
{
        // Do not allow placing magic traps next to blocking features
        // (non-Occultist characters cannot disarm them)
        for (const P& d : dir_utils::dir_list)
        {
                const P p(pos_ + d);

                const auto* const f = map::cells.at(p).rigid;

                if (!f->is_walkable())
                {
                        return TrapPlacementValid::no;
                }
        }

        return TrapPlacementValid::yes;
}

TrapDart::TrapDart(P pos, Trap* const base_trap) :
        MechTrapImpl(pos, TrapId::dart, base_trap),
        is_poisoned_((map::dlvl >= dlvl_harder_traps) && rnd::one_in(3)),
        dart_origin_(),
        is_dart_origin_destroyed_(false) {}

TrapPlacementValid TrapDart::on_place()
{
        auto offsets = dir_utils::cardinal_list;

        rnd::shuffle(offsets);

        const int nr_steps_min = 2;
        const int nr_steps_max = fov_radi_int;

        auto trap_plament_valid = TrapPlacementValid::no;

        for (const P& d : offsets)
        {
                P p = pos_;

                for (int i = 0; i <= nr_steps_max; ++i)
                {
                        p += d;

                        const Rigid* const rigid = map::cells.at(p).rigid;

                        const bool is_wall = rigid->id() == FeatureId::wall;

                        const bool is_passable =
                                rigid->is_projectile_passable();

                        if (!is_passable &&
                            ((i < nr_steps_min) || !is_wall))
                        {
                                // We are blocked too early - OR - blocked by a
                                // rigid feature other than a wall. Give up on
                                // this direction.
                                break;
                        }

                        if ((i >= nr_steps_min) && is_wall)
                        {
                                // This is a good origin!
                                dart_origin_ = p;
                                trap_plament_valid = TrapPlacementValid::yes;
                                break;
                        }
                }

                if (trap_plament_valid == TrapPlacementValid::yes)
                {
                        // A valid origin has been found

                        if (rnd::fraction(2, 3))
                        {
                                map::make_gore(pos_);
                                map::make_blood(pos_);
                        }

                        break;
                }
        }

        return trap_plament_valid;
}

void TrapDart::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        ASSERT(dart_origin_.x == pos_.x || dart_origin_.y == pos_.y);
        ASSERT(dart_origin_ != pos_);

        const auto& origin_cell = map::cells.at(dart_origin_);

        if (origin_cell.rigid->id() != FeatureId::wall)
        {
                // NOTE: This is permanently set from now on
                is_dart_origin_destroyed_ = true;
        }

        if (is_dart_origin_destroyed_)
        {
                return;
        }

        // Aim target is the wall on the other side of the map
        P aim_pos = dart_origin_;

        if (dart_origin_.x == pos_.x)
        {
                aim_pos.y =
                        (dart_origin_.y > pos_.y)
                        ? 0
                        : (map::h() - 1);
        }
        else // Dart origin is on same vertial line as the trap
        {
                aim_pos.x =
                        (dart_origin_.x > pos_.x)
                        ? 0
                        : (map::w() - 1);
        }

        if (origin_cell.is_seen_by_player)
        {
                const std::string name = origin_cell.rigid->name(Article::the);

                msg_log::add("A dart is launched from " + name + "!");
        }

        // Make a temporary dart weapon
        Wpn* wpn = nullptr;

        if (is_poisoned_)
        {
                wpn = static_cast<Wpn*>(
                        item_factory::make(ItemId::trap_dart_poison));
        }
        else // Not poisoned
        {
                wpn = static_cast<Wpn*>(
                        item_factory::make(ItemId::trap_dart));
        }

        // Fire!
        attack::ranged(
                nullptr,
                dart_origin_,
                aim_pos,
                *wpn);

        delete wpn;

        TRACE_FUNC_END_VERBOSE;
}

TrapSpear::TrapSpear(P pos, Trap* const base_trap) :
        MechTrapImpl(pos, TrapId::spear, base_trap),
        is_poisoned_((map::dlvl >= dlvl_harder_traps) && rnd::one_in(4)),
        spear_origin_(),
        is_spear_origin_destroyed_(false) {}

TrapPlacementValid TrapSpear::on_place()
{
        auto offsets = dir_utils::cardinal_list;

        rnd::shuffle(offsets);

        auto trap_plament_valid = TrapPlacementValid::no;

        for (const P& d : offsets)
        {
                const P p = pos_ + d;

                const Rigid* const rigid = map::cells.at(p).rigid;

                const bool is_wall = rigid->id() == FeatureId::wall;

                const bool is_passable = rigid->is_projectile_passable();

                if (is_wall && !is_passable)
                {
                        // This is a good origin!
                        spear_origin_ = p;
                        trap_plament_valid = TrapPlacementValid::yes;

                        if (rnd::fraction(2, 3))
                        {
                                map::make_gore(pos_);
                                map::make_blood(pos_);
                        }

                        break;
                }
        }

        return trap_plament_valid;
}

void TrapSpear::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        ASSERT(spear_origin_.x == pos_.x || spear_origin_.y == pos_.y);
        ASSERT(spear_origin_ != pos_);

        const auto& origin_cell = map::cells.at(spear_origin_);

        if (origin_cell.rigid->id() != FeatureId::wall)
        {
                // NOTE: This is permanently set from now on
                is_spear_origin_destroyed_ = true;
        }

        if (is_spear_origin_destroyed_)
        {
                return;
        }

        if (origin_cell.is_seen_by_player)
        {
                const std::string name = origin_cell.rigid->name(Article::the);

                msg_log::add("A spear shoots out from " + name + "!");
        }

        // Is anyone standing on the trap now?
        Actor* const actor_on_trap = map::actor_at_pos(pos_);

        if (actor_on_trap)
        {
                // Make a temporary spear weapon
                Wpn* wpn = nullptr;

                if (is_poisoned_)
                {
                        wpn = static_cast<Wpn*>(
                                item_factory::make(
                                        ItemId::trap_spear_poison));
                }
                else // Not poisoned
                {
                        wpn = static_cast<Wpn*>(
                                item_factory::make(
                                        ItemId::trap_spear));
                }

                // Attack!
                attack::melee(nullptr,
                              spear_origin_,
                              *actor_on_trap,
                              *wpn);

                delete wpn;
        }

        TRACE_FUNC_BEGIN_VERBOSE;
}

void TrapGasConfusion::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::cells.at(pos_).is_seen_by_player)
        {
                msg_log::add(
                        "A burst of gas is released from a vent in the floor!");
        }

        Snd snd("I hear a burst of gas.",
                SfxId::gas,
                IgnoreMsgIfOriginSeen::yes,
                pos_,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);

        explosion::run(pos_,
                       ExplType::apply_prop,
                       EmitExplSnd::no,
                       -1,
                       ExplExclCenter::no,
                       {new PropConfused()},
                       colors::magenta(),
                       ExplIsGas::yes);

        TRACE_FUNC_END_VERBOSE;
}

void TrapGasParalyzation::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::cells.at(pos_).is_seen_by_player)
        {
                msg_log::add(
                        "A burst of gas is released from a vent in the floor!");
        }

        Snd snd("I hear a burst of gas.",
                SfxId::gas,
                IgnoreMsgIfOriginSeen::yes,
                pos_,
                nullptr,
                SndVol::low, AlertsMon::yes);

        snd_emit::run(snd);

        explosion::run(pos_,
                       ExplType::apply_prop,
                       EmitExplSnd::no,
                       -1,
                       ExplExclCenter::no,
                       {new PropParalyzed()},
                       colors::magenta(),
                       ExplIsGas::yes);

        TRACE_FUNC_END_VERBOSE;
}

void TrapGasFear::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::cells.at(pos_).is_seen_by_player)
        {
                msg_log::add(
                        "A burst of gas is released from a vent in the floor!");
        }

        Snd snd("I hear a burst of gas.",
                SfxId::gas,
                IgnoreMsgIfOriginSeen::yes,
                pos_,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);

        explosion::run(pos_,
                       ExplType::apply_prop,
                       EmitExplSnd::no,
                       -1,
                       ExplExclCenter::no,
                       {new PropTerrified()},
                       colors::magenta(),
                       ExplIsGas::yes);

        TRACE_FUNC_END_VERBOSE;
}

void TrapBlindingFlash::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::cells.at(pos_).is_seen_by_player)
        {
                msg_log::add("There is an intense flash of light!");
        }

        explosion::run(pos_,
                       ExplType::apply_prop,
                       EmitExplSnd::no,
                       -1,
                       ExplExclCenter::no,
                       {new PropBlind()},
                       colors::yellow());

        TRACE_FUNC_END_VERBOSE;
}

void TrapDeafening::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::cells.at(pos_).is_seen_by_player)
        {
                msg_log::add(
                        "There is suddenly a crushing pressure in the air!");
        }

        explosion::run(pos_,
                       ExplType::apply_prop,
                       EmitExplSnd::no,
                       -1,
                       ExplExclCenter::no,
                       {new PropDeaf()},
                       colors::light_white());

        TRACE_FUNC_END_VERBOSE;
}

void TrapTeleport::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        Actor* const actor_here = map::actor_at_pos(pos_);

        ASSERT(actor_here);

        if (!actor_here)
        {
                // Should never happen
                return;
        }

        const bool is_player = actor_here->is_player();

        const bool can_see = actor_here->properties.allow_see();

        const bool player_sees_actor = map::player->can_see_actor(*actor_here);

        const std::string actor_name = actor_here->name_the();

        const bool is_hidden = base_trap_->is_hidden();

        if (is_player)
        {
                map::update_vision();

                if (can_see)
                {
                        std::string msg = "A beam of light shoots out from";

                        if (!is_hidden)
                        {
                                msg += " a curious shape on";
                        }

                        msg += " the floor!";

                        msg_log::add(msg);
                }
                else // Cannot see
                {
                        msg_log::add("I feel a peculiar energy around me!");
                }
        }
        else // Is a monster
        {
                if (player_sees_actor)
                {
                        msg_log::add(
                                "A beam shoots out under " + actor_name + ".");
                }
        }

        teleport(*actor_here);

        TRACE_FUNC_END_VERBOSE;
}

void TrapSummonMon::trigger()
{
        TRACE_FUNC_BEGIN;

        Actor* const actor_here = map::actor_at_pos(pos_);

        ASSERT(actor_here);

        if (!actor_here)
        {
                // Should never happen
                return;
        }

        const bool is_player = actor_here->is_player();
        const bool is_hidden = base_trap_->is_hidden();

        TRACE_VERBOSE << "Is player: " << is_player << std::endl;

        if (!is_player)
        {
                TRACE_VERBOSE << "Not triggered by player" << std::endl;
                TRACE_FUNC_END_VERBOSE;
                return;
        }

        const bool can_see = actor_here->properties.allow_see();
        TRACE_VERBOSE << "Actor can see: " << can_see << std::endl;

        const std::string actor_name = actor_here->name_the();
        TRACE_VERBOSE << "Actor name: " << actor_name << std::endl;

        map::player->update_fov();

        if (can_see)
        {
                std::string msg = "A beam of light shoots out from";

                if (!is_hidden)
                {
                        msg += " a curious shape on";
                }

                msg += " the floor!";

                msg_log::add(msg);
        }
        else // Cannot see
        {
                msg_log::add("I feel a peculiar energy around me!");
        }

        TRACE << "Finding summon candidates" << std::endl;
        std::vector<ActorId> summon_bucket;

        for (size_t i = 0; i < (size_t)ActorId::END; ++i)
        {
                const ActorData& data = actor_data::data[i];

                if (data.can_be_summoned_by_mon &&
                    data.spawn_min_dlvl <= map::dlvl + 3)
                {
                        summon_bucket.push_back((ActorId)i);
                }
        }

        if (summon_bucket.empty())
        {
                TRACE_VERBOSE << "No eligible candidates found" << std::endl;
        }
        else // Eligible monsters found
        {
                const size_t idx = rnd::range(0, summon_bucket.size() - 1);

                const ActorId id_to_summon = summon_bucket[idx];

                TRACE_VERBOSE << "Actor id: " << int(id_to_summon) << std::endl;

                const auto summoned =
                        actor_factory::spawn(pos_,
                                             {id_to_summon},
                                             map::rect())
                        .make_aware_of_player()
                        .for_each([](Mon* const mon)
                        {
                                auto prop_summoned = new PropSummoned();

                                prop_summoned->set_indefinite();

                                mon->properties.apply(prop_summoned);

                                auto prop_waiting = new PropWaiting();

                                prop_waiting->set_duration(2);

                                mon->properties.apply(prop_waiting);

                                if (map::player->can_see_actor(*mon))
                                {
                                        states::draw();

                                        const std::string name_a =
                                                text_format::first_to_upper(
                                                        mon->name_a());

                                        msg_log::add(name_a + " appears!");
                                }
                        });
        }

        TRACE_FUNC_END;
}

void TrapSpiDrain::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        Actor* const actor_here = map::actor_at_pos(pos_);

        ASSERT(actor_here);

        if (!actor_here)
        {
                // Should never happen
                return;
        }

        const bool is_player = actor_here->is_player();
        const bool is_hidden = base_trap_->is_hidden();

        TRACE_VERBOSE << "Is player: " << is_player << std::endl;

        if (!is_player)
        {
                TRACE_VERBOSE << "Not triggered by player" << std::endl;

                TRACE_FUNC_END_VERBOSE;

                return;
        }

        const bool can_see = actor_here->properties.allow_see();

        TRACE_VERBOSE << "Actor can see: " << can_see << std::endl;

        const std::string actor_name = actor_here->name_the();

        TRACE_VERBOSE << "Actor name: " << actor_name << std::endl;

        if (can_see)
        {
                std::string msg = "A beam of light shoots out from";

                if (!is_hidden)
                {
                        msg += " a curious shape on";
                }

                msg += " the floor!";

                msg_log::add(msg);
        }
        else // Cannot see
        {
                msg_log::add("I feel a peculiar energy around me!");
        }

        TRACE << "Draining player spirit" << std::endl;

        // Never let spirit draining traps insta-kill the player
        const int sp_drained = map::player->sp - 1;

        if (sp_drained > 0)
        {
                actor::hit_sp(*map::player, sp_drained);
        }
        else
        {
                msg_log::add("I feel somewhat drained.");
        }

        TRACE_FUNC_END_VERBOSE;
}

void TrapSmoke::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::cells.at(pos_).is_seen_by_player)
        {
                msg_log::add(
                        "A burst of smoke is released from a vent in the "
                        "floor!");
        }

        Snd snd("I hear a burst of gas.",
                SfxId::gas,
                IgnoreMsgIfOriginSeen::yes,
                pos_,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);

        explosion::run_smoke_explosion_at(pos_);

        TRACE_FUNC_END_VERBOSE;
}

void TrapFire::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::cells.at(pos_).is_seen_by_player)
        {
                msg_log::add("Flames burst out from a vent in the floor!");
        }

        Snd snd("I hear a burst of flames.",
                SfxId::END,
                IgnoreMsgIfOriginSeen::yes,
                pos_,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);

        explosion::run(pos_,
                       ExplType::apply_prop,
                       EmitExplSnd::no,
                       -1,
                       ExplExclCenter::no,
                       {new PropBurning()});

        TRACE_FUNC_END_VERBOSE;
}

void TrapAlarm::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::cells.at(pos_).is_seen_by_player)
        {
                msg_log::add("An alarm sounds!");
        }

        Snd snd("I hear an alarm sounding!",
                SfxId::END,
                IgnoreMsgIfOriginSeen::yes,
                pos_,
                nullptr,
                SndVol::high,
                AlertsMon::yes);

        snd_emit::run(snd);

        TRACE_FUNC_END_VERBOSE;
}

void TrapWeb::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        Actor* const actor_here = map::actor_at_pos(pos_);

        ASSERT(actor_here);

        if (!actor_here)
        {
                return;
        }

        if (actor_here->is_player())
        {
                if (actor_here->properties.allow_see())
                {
                        msg_log::add(
                                "I am entangled in a spider web!");
                }
                else // Cannot see
                {
                        msg_log::add(
                                "I am entangled in a sticky mass of threads!");
                }
        }
        else // Is a monster
        {
                if (map::player->can_see_actor(*actor_here))
                {
                        const std::string actor_name =
                                text_format::first_to_upper(
                                        actor_here->name_the());

                        msg_log::add(actor_name +
                                     " is entangled in a huge spider web!");
                }
        }

        Prop* entangled = new PropEntangled();

        entangled->set_indefinite();

        actor_here->properties.apply(
                entangled,
                PropSrc::intr,
                false,
                Verbosity::silent);

        // Players getting stuck in spider webs alerts all spiders
        if (actor_here->is_player())
        {
                for (Actor* const actor : game_time::actors)
                {
                        if (actor->is_player() || !actor->data->is_spider)
                        {
                                continue;
                        }

                        auto* const mon = static_cast<Mon*>(actor);

                        mon->become_aware_player(false);
                }
        }

        base_trap_->destroy();

        TRACE_FUNC_END_VERBOSE;
}

void TrapSlow::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        Actor* const actor_here = map::actor_at_pos(pos_);

        ASSERT(actor_here);

        if (!actor_here)
        {
                // Should never happen
                return;
        }

        actor_here->properties.apply(new PropSlowed());

        TRACE_FUNC_END_VERBOSE;
}

void TrapCurse::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        Actor* const actor_here = map::actor_at_pos(pos_);

        ASSERT(actor_here);

        if (!actor_here)
        {
                // Should never happen
                return;
        }

        actor_here->properties.apply(new PropCursed());

        TRACE_FUNC_END_VERBOSE;
}
