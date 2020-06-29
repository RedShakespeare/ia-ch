// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "terrain_trap.hpp"

#include <algorithm>

#include "actor_factory.hpp"
#include "actor_hit.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "actor_see.hpp"
#include "attack.hpp"
#include "common_text.hpp"
#include "drop.hpp"
#include "explosion.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "popup.hpp"
#include "postmortem.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "property_handler.hpp"
#include "query.hpp"
#include "sound.hpp"
#include "teleport.hpp"
#include "terrain_data.hpp"
#include "text_format.hpp"

namespace terrain {

// -----------------------------------------------------------------------------
// Trap
// -----------------------------------------------------------------------------
Trap::Trap(const P& pos, Terrain* const mimic_terrain, TrapId id) :
        Terrain(pos),
        m_mimic_terrain(mimic_terrain)
{
        ASSERT(id != TrapId::END);

        m_is_hidden = true;

        auto* const terrain_here = map::g_cells.at(pos).terrain;

        if (!terrain_here->can_have_trap()) {
                TRACE << "Cannot place trap on terrain id: "
                      << (int)terrain_here->id()
                      << std::endl
                      << "Trap id: " << (int)id
                      << std::endl;

                ASSERT(false);
                return;
        }

        auto try_place_trap_or_discard = [&](const TrapId trap_id) {
                auto* impl = make_trap_impl_from_id(trap_id);

                auto valid = impl->on_place();

                if (valid == TrapPlacementValid::yes) {
                        m_trap_impl = impl;
                } else {
                        // Placement not valid
                        delete impl;
                }
        };

        if (id == TrapId::any) {
                // Attempt to set a trap implementation until succeeding
                while (true) {
                        const auto random_id =
                                (TrapId)rnd::range(0, (int)TrapId::END - 1);

                        try_place_trap_or_discard(random_id);

                        if (m_trap_impl) {
                                // Trap placement is good!
                                break;
                        }
                }
        } else {
                // Make a specific trap type

                // NOTE: This may fail, in which case we have no trap
                // implementation. The trap creator is responsible for handling
                // this situation.
                try_place_trap_or_discard(id);
        }
}

Trap::~Trap()
{
        delete m_trap_impl;
        delete m_mimic_terrain;
}

TrapImpl* Trap::make_trap_impl_from_id(const TrapId trap_id)
{
        switch (trap_id) {
        case TrapId::dart:
                return new TrapDart(m_pos, this);
                break;

        case TrapId::spear:
                return new TrapSpear(m_pos, this);
                break;

        case TrapId::gas_confusion:
                return new TrapGasConfusion(m_pos, this);
                break;

        case TrapId::gas_paralyze:
                return new TrapGasParalyzation(m_pos, this);
                break;

        case TrapId::gas_fear:
                return new TrapGasFear(m_pos, this);
                break;

        case TrapId::blinding:
                return new TrapBlindingFlash(m_pos, this);
                break;

        case TrapId::deafening:
                return new TrapDeafening(m_pos, this);
                break;

        case TrapId::teleport:
                return new TrapTeleport(m_pos, this);
                break;

        case TrapId::summon:
                return new TrapSummonMon(m_pos, this);
                break;

        case TrapId::hp_sap:
                return new TrapHpSap(m_pos, this);
                break;

        case TrapId::spi_sap:
                return new TrapSpiSap(m_pos, this);
                break;

        case TrapId::smoke:
                return new TrapSmoke(m_pos, this);
                break;

        case TrapId::fire:
                return new TrapFire(m_pos, this);
                break;

        case TrapId::alarm:
                return new TrapAlarm(m_pos, this);
                break;

        case TrapId::web:
                return new TrapWeb(m_pos, this);
                break;

        case TrapId::slow:
                return new TrapSlow(m_pos, this);

        case TrapId::curse:
                return new TrapCurse(m_pos, this);

        case TrapId::unlearn_spell:
                return new TrapUnlearnSpell(m_pos, this);

        case TrapId::END:
        case TrapId::any:
                break;
        }

        return nullptr;
}

void Trap::on_hit(
        const int dmg,
        const DmgType dmg_type,
        const DmgMethod dmg_method,
        actor::Actor* const actor)
{
        (void)dmg;
        (void)dmg_type;
        (void)dmg_method;
        (void)actor;
}

TrapId Trap::type() const
{
        ASSERT(m_trap_impl);

        return m_trap_impl->m_type;
}

bool Trap::is_magical() const
{
        ASSERT(m_trap_impl);

        return m_trap_impl->is_magical();
}

void Trap::on_new_turn_hook()
{
        if (m_nr_turns_until_trigger > 0) {
                --m_nr_turns_until_trigger;

                TRACE_VERBOSE << "Number of turns until trigger: "
                              << m_nr_turns_until_trigger << std::endl;

                if (m_nr_turns_until_trigger == 0) {
                        // NOTE: This will reset number of turns until triggered
                        trigger_trap(nullptr);
                }
        }
}

void Trap::trigger_start(const actor::Actor* actor)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        ASSERT(m_trap_impl);

        if (actor == map::g_player) {
                // Reveal trap if triggered by player stepping on it
                if (is_hidden()) {
                        reveal(Verbose::no);
                }

                map::g_player->update_fov();

                states::draw();
        }

        if (is_magical()) {
                // TODO: Play sfx for magic traps (if player)
        } else {
                // Not magical
                if (type() != TrapId::web) {
                        std::string msg = "I hear a click.";

                        auto alerts = AlertsMon::no;

                        if (actor == map::g_player) {
                                alerts = AlertsMon::yes;

                                // If player is triggering, make the message a
                                // bit more "foreboding"
                                msg += "..";
                        }

                        // TODO: Make a sound effect for this
                        Snd snd(msg,
                                audio::SfxId::END,
                                IgnoreMsgIfOriginSeen::no,
                                m_pos,
                                nullptr,
                                SndVol::low,
                                alerts);

                        snd_emit::run(snd);

                        if (actor == map::g_player) {
                                const bool is_deaf =
                                        map::g_player->m_properties.has(
                                                PropId::deaf);

                                if (is_deaf) {
                                        msg_log::add(
                                                "I feel the ground shifting "
                                                "slightly under my foot.");
                                }

                                msg_log::more_prompt();
                        }
                }
        }

        // Get a randomized value for number of remaining turns
        const Range turns_range = m_trap_impl->nr_turns_range_to_trigger();

        const int rnd_nr_turns = turns_range.roll();

        // Set number of remaining turns to the randomized value if not set
        // already, or if the new value will make it trigger sooner
        if ((m_nr_turns_until_trigger == -1) ||
            (rnd_nr_turns < m_nr_turns_until_trigger)) {
                m_nr_turns_until_trigger = rnd_nr_turns;
        }

        ASSERT(m_nr_turns_until_trigger >= 0);

        // If number of remaining turns is zero, trigger immediately
        if (m_nr_turns_until_trigger == 0) {
                // NOTE: This will reset number of turns until triggered
                trigger_trap(nullptr);
        }

        TRACE_FUNC_END_VERBOSE;
}

AllowAction Trap::pre_bump(actor::Actor& actor_bumping)
{
        if (!actor_bumping.is_player() ||
            actor_bumping.m_properties.has(PropId::confused)) {
                return AllowAction::yes;
        }

        if (map::g_cells.at(m_pos).is_seen_by_player &&
            !m_is_hidden &&
            !actor_bumping.m_properties.has(PropId::ethereal) &&
            !actor_bumping.m_properties.has(PropId::flying)) {
                // The trap is known, and will be triggered by the player

                const std::string name_the = name(Article::the);

                const std::string msg =
                        "Step into " +
                        name_the +
                        "? " +
                        common_text::g_yes_or_no_hint;

                msg_log::add(
                        msg,
                        colors::light_white(),
                        MsgInterruptPlayer::no,
                        MorePromptOnMsg::no,
                        CopyToMsgHistory::no);

                const auto query_result = query::yes_or_no();

                msg_log::clear();

                return (query_result == BinaryAnswer::no)
                        ? AllowAction::no
                        : AllowAction::yes;
        } else {
                // The trap is unknown, or will not be triggered by the player -
                // delegate the question to the mimicked terrain

                const auto result = m_mimic_terrain->pre_bump(actor_bumping);

                return result;
        }
}

void Trap::bump(actor::Actor& actor_bumping)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        const auto& d = *actor_bumping.m_data;

        if (actor_bumping.m_properties.has(PropId::ethereal) ||
            actor_bumping.m_properties.has(PropId::flying) ||
            (d.actor_size < actor::Size::humanoid) ||
            d.is_spider) {
                TRACE_FUNC_END_VERBOSE;

                return;
        }

        if (!actor_bumping.is_player()) {
                // Put some extra restrictions on monsters triggering traps.
                // This helps prevent stupid situations like a group of monsters
                // in a small room repeatedly triggering a trap.
                if (!actor_bumping.m_ai_state.is_target_seen ||
                    !actor_bumping.is_aware_of_player() ||
                    is_hidden()) {
                        TRACE_FUNC_END_VERBOSE;

                        return;
                }
        }

        trigger_start(&actor_bumping);

        TRACE_FUNC_END_VERBOSE;
}

void Trap::disarm()
{
        const bool is_magic_trap = is_magical();

        if (is_magic_trap && (player_bon::bg() != Bg::occultist)) {
                msg_log::add("I do not know how to dispel magic traps.");

                return;
        }

        msg_log::add(m_trap_impl->disarm_msg());

        destroy();

        if (is_magic_trap) {
                map::g_player->restore_sp(
                        rnd::range(1, 6),
                        true); // Can go above max
        }
}

void Trap::destroy()
{
        ASSERT(m_mimic_terrain);

        // Magical traps and webs simply "dissapear" (place their mimic
        // terrain), and mechanical traps puts rubble.

        if (is_magical() || type() == TrapId::web) {
                auto* const f_tmp = m_mimic_terrain;

                m_mimic_terrain = nullptr;

                // NOTE: This call destroys the object!
                map::put(f_tmp);
        } else {
                // "Mechanical" trap
                map::put(new RubbleLow(m_pos));
        }
}

DidTriggerTrap Trap::trigger_trap(actor::Actor* const actor)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        (void)actor;

        TRACE_VERBOSE << "Name of trap triggering: "
                      << m_trap_impl->name(Article::a)
                      << std::endl;

        m_nr_turns_until_trigger = -1;

        TRACE_VERBOSE << "Calling trigger in trap implementation" << std::endl;

        m_trap_impl->trigger();

        // NOTE: This object may now be deleted (e.g. a web was torn down)!

        TRACE_FUNC_END_VERBOSE;
        return DidTriggerTrap::yes;
}

void Trap::reveal(const Verbose verbose)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        m_is_hidden = false;

        clear_gore();

        if (map::g_cells.at(m_pos).is_seen_by_player) {
                states::draw();

                if (verbose == Verbose::yes) {
                        std::string msg;

                        const std::string trap_name_a =
                                m_trap_impl->name(Article::a);

                        if (m_pos == map::g_player->m_pos) {
                                msg += "There is " + trap_name_a + " here!";
                        } else {
                                // Trap is not at player position
                                msg = "I spot " + trap_name_a + ".";
                        }

                        msg_log::add(msg);
                }
        }

        TRACE_FUNC_END_VERBOSE;
}

void Trap::on_revealed_from_searching()
{
        game::incr_player_xp(1);
}

std::string Trap::name(const Article article) const
{
        return m_is_hidden
                ? m_mimic_terrain->name(article)
                : m_trap_impl->name(article);
}

Color Trap::color_default() const
{
        return m_is_hidden
                ? m_mimic_terrain->color()
                : m_trap_impl->color();
}

Color Trap::color_bg_default() const
{
        const auto* const item = map::g_cells.at(m_pos).item;

        const auto* const corpse =
                map::first_actor_at_pos(
                        m_pos,
                        ActorState::corpse);

        if (!m_is_hidden && (item || corpse)) {
                return m_trap_impl->color();
        } else {
                // Is hidden, or nothing is over the trap
                return colors::black();
        }
}

char Trap::character() const
{
        return m_is_hidden
                ? m_mimic_terrain->character()
                : m_trap_impl->character();
}

gfx::TileId Trap::tile() const
{
        return m_is_hidden
                ? m_mimic_terrain->tile()
                : m_trap_impl->tile();
}

Matl Trap::matl() const
{
        return m_is_hidden
                ? m_mimic_terrain->matl()
                : data().matl_type;
}

// -----------------------------------------------------------------------------
// Trap implementations
// -----------------------------------------------------------------------------
TrapPlacementValid MagicTrapImpl::on_place()
{
        // Do not allow placing magic traps next to blocking terrains
        // (non-Occultist characters cannot disarm them)
        for (const P& d : dir_utils::g_dir_list) {
                const P p(m_pos + d);

                const auto* const t = map::g_cells.at(p).terrain;

                if (!t->is_walkable()) {
                        return TrapPlacementValid::no;
                }
        }

        return TrapPlacementValid::yes;
}

TrapDart::TrapDart(P pos, Trap* const base_trap) :
        MechTrapImpl(pos, TrapId::dart, base_trap),
        m_is_poisoned((map::g_dlvl >= g_dlvl_harder_traps) && rnd::one_in(3)),

        m_is_dart_origin_destroyed(false)
{}

TrapPlacementValid TrapDart::on_place()
{
        auto offsets = dir_utils::g_cardinal_list;

        rnd::shuffle(offsets);

        const int nr_steps_min = 2;
        const int nr_steps_max = g_fov_radi_int;

        auto trap_plament_valid = TrapPlacementValid::no;

        for (const P& d : offsets) {
                P p = m_pos;

                for (int i = 0; i <= nr_steps_max; ++i) {
                        p += d;

                        const auto* const terrain = map::g_cells.at(p).terrain;

                        const bool is_wall = terrain->id() == terrain::Id::wall;

                        const bool is_passable =
                                terrain->is_projectile_passable();

                        if (!is_passable &&
                            ((i < nr_steps_min) || !is_wall)) {
                                // We are blocked too early - OR - blocked by a
                                // terrain other than a wall. Give up on this
                                // direction.
                                break;
                        }

                        if ((i >= nr_steps_min) && is_wall) {
                                // This is a good origin!
                                m_dart_origin = p;
                                trap_plament_valid = TrapPlacementValid::yes;
                                break;
                        }
                }

                if (trap_plament_valid == TrapPlacementValid::yes) {
                        // A valid origin has been found

                        if (rnd::fraction(2, 3)) {
                                map::make_gore(m_pos);
                                map::make_blood(m_pos);
                        }

                        break;
                }
        }

        return trap_plament_valid;
}

void TrapDart::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        ASSERT((m_dart_origin.x == m_pos.x) || (m_dart_origin.y == m_pos.y));
        ASSERT(m_dart_origin != m_pos);

        const auto& origin_cell = map::g_cells.at(m_dart_origin);

        if (origin_cell.terrain->id() != terrain::Id::wall) {
                // NOTE: This is permanently set from now on
                m_is_dart_origin_destroyed = true;
        }

        if (m_is_dart_origin_destroyed) {
                return;
        }

        // Aim target is the wall on the other side of the map
        P aim_pos = m_dart_origin;

        if (m_dart_origin.x == m_pos.x) {
                aim_pos.y =
                        (m_dart_origin.y > m_pos.y)
                        ? 0
                        : (map::h() - 1);
        } else {
                // Dart origin is on same vertial line as the trap
                aim_pos.x =
                        (m_dart_origin.x > m_pos.x)
                        ? 0
                        : (map::w() - 1);
        }

        if (origin_cell.is_seen_by_player) {
                const std::string name =
                        origin_cell.terrain->name(Article::the);

                msg_log::add("A dart is launched from " + name + "!");
        }

        // Make a temporary dart weapon
        item::Wpn* wpn = nullptr;

        if (m_is_poisoned) {
                wpn = static_cast<item::Wpn*>(
                        item::make(item::Id::trap_dart_poison));
        } else {
                // Not poisoned
                wpn = static_cast<item::Wpn*>(
                        item::make(item::Id::trap_dart));
        }

        // Fire!
        attack::ranged(
                nullptr,
                m_dart_origin,
                aim_pos,
                *wpn);

        delete wpn;

        TRACE_FUNC_END_VERBOSE;
}

TrapSpear::TrapSpear(P pos, Trap* const base_trap) :
        MechTrapImpl(pos, TrapId::spear, base_trap),
        m_is_poisoned((map::g_dlvl >= g_dlvl_harder_traps) && rnd::one_in(4)),

        m_is_spear_origin_destroyed(false)
{}

TrapPlacementValid TrapSpear::on_place()
{
        auto offsets = dir_utils::g_cardinal_list;

        rnd::shuffle(offsets);

        auto trap_plament_valid = TrapPlacementValid::no;

        for (const P& d : offsets) {
                const P p = m_pos + d;

                const auto* const terrain = map::g_cells.at(p).terrain;

                const bool is_wall = terrain->id() == terrain::Id::wall;

                const bool is_passable = terrain->is_projectile_passable();

                if (is_wall && !is_passable) {
                        // This is a good origin!
                        m_spear_origin = p;
                        trap_plament_valid = TrapPlacementValid::yes;

                        if (rnd::fraction(2, 3)) {
                                map::make_gore(m_pos);
                                map::make_blood(m_pos);
                        }

                        break;
                }
        }

        return trap_plament_valid;
}

void TrapSpear::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        ASSERT(m_spear_origin.x == m_pos.x || m_spear_origin.y == m_pos.y);
        ASSERT(m_spear_origin != m_pos);

        const auto& origin_cell = map::g_cells.at(m_spear_origin);

        if (origin_cell.terrain->id() != terrain::Id::wall) {
                // NOTE: This is permanently set from now on
                m_is_spear_origin_destroyed = true;
        }

        if (m_is_spear_origin_destroyed) {
                return;
        }

        if (origin_cell.is_seen_by_player) {
                const std::string name =
                        origin_cell.terrain->name(Article::the);

                msg_log::add("A spear shoots out from " + name + "!");
        }

        // Is anyone standing on the trap now?
        auto* const actor_on_trap = map::first_actor_at_pos(m_pos);

        if (actor_on_trap) {
                // Make a temporary spear weapon
                item::Wpn* wpn = nullptr;

                if (m_is_poisoned) {
                        wpn = static_cast<item::Wpn*>(
                                item::make(item::Id::trap_spear_poison));
                } else {
                        // Not poisoned
                        wpn = static_cast<item::Wpn*>(
                                item::make(item::Id::trap_spear));
                }

                // Attack!
                attack::melee(
                        nullptr,
                        m_spear_origin,
                        *actor_on_trap,
                        *wpn);

                delete wpn;
        }

        TRACE_FUNC_BEGIN_VERBOSE;
}

void TrapGasConfusion::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::g_cells.at(m_pos).is_seen_by_player) {
                msg_log::add(
                        "A burst of gas is released from a vent in the floor!");
        }

        Snd snd("I hear a burst of gas.",
                audio::SfxId::gas,
                IgnoreMsgIfOriginSeen::yes,
                m_pos,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);

        explosion::run(m_pos, ExplType::apply_prop, EmitExplSnd::no, -1, ExplExclCenter::no, {new PropConfused()}, colors::magenta(), ExplIsGas::yes);

        TRACE_FUNC_END_VERBOSE;
}

void TrapGasParalyzation::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::g_cells.at(m_pos).is_seen_by_player) {
                msg_log::add(
                        "A burst of gas is released from a vent in the floor!");
        }

        Snd snd("I hear a burst of gas.",
                audio::SfxId::gas,
                IgnoreMsgIfOriginSeen::yes,
                m_pos,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);

        explosion::run(
                m_pos,
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

        if (map::g_cells.at(m_pos).is_seen_by_player) {
                msg_log::add(
                        "A burst of gas is released from a vent in the floor!");
        }

        Snd snd("I hear a burst of gas.",
                audio::SfxId::gas,
                IgnoreMsgIfOriginSeen::yes,
                m_pos,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);

        explosion::run(
                m_pos,
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

        if (map::g_cells.at(m_pos).is_seen_by_player) {
                msg_log::add("There is an intense flash of light!");
        }

        explosion::run(
                m_pos,
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

        if (map::g_cells.at(m_pos).is_seen_by_player) {
                msg_log::add(
                        "There is suddenly a crushing pressure in the air!");
        }

        explosion::run(
                m_pos,
                ExplType::apply_prop,
                EmitExplSnd::no,
                -1,
                ExplExclCenter::no,
                {property_factory::make(PropId::deaf)},
                colors::light_white());

        TRACE_FUNC_END_VERBOSE;
}

void TrapTeleport::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        auto* const actor_here = map::first_actor_at_pos(m_pos);

        ASSERT(actor_here);

        if (!actor_here) {
                // Should never happen
                return;
        }

        const auto is_player = actor_here->is_player();
        const auto can_see = actor_here->m_properties.allow_see();
        const auto player_sees_actor = actor::can_player_see_actor(*actor_here);
        const auto actor_name = actor_here->name_the();
        const auto is_hidden = m_base_trap->is_hidden();

        if (is_player) {
                map::update_vision();

                if (can_see) {
                        std::string msg = "A beam of light shoots out from";

                        if (!is_hidden) {
                                msg += " a curious shape on";
                        }

                        msg += " the floor!";

                        msg_log::add(msg);
                } else {
                        // Cannot see
                        msg_log::add("I feel a peculiar energy around me!");
                }
        } else {
                // Is a monster
                if (player_sees_actor) {
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

        auto* const actor_here = map::first_actor_at_pos(m_pos);

        ASSERT(actor_here);

        if (!actor_here) {
                // Should never happen
                return;
        }

        const bool is_player = actor_here->is_player();
        const bool is_hidden = m_base_trap->is_hidden();

        TRACE_VERBOSE << "Is player: " << is_player << std::endl;

        if (!is_player) {
                TRACE_VERBOSE << "Not triggered by player" << std::endl;
                TRACE_FUNC_END_VERBOSE;
                return;
        }

        const bool can_see = actor_here->m_properties.allow_see();
        TRACE_VERBOSE << "Actor can see: " << can_see << std::endl;

        const std::string actor_name = actor_here->name_the();
        TRACE_VERBOSE << "Actor name: " << actor_name << std::endl;

        map::g_player->update_fov();

        if (can_see) {
                std::string msg = "A beam of light shoots out from";

                if (!is_hidden) {
                        msg += " a curious shape on";
                }

                msg += " the floor!";

                msg_log::add(msg);
        } else {
                // Cannot see
                msg_log::add("I feel a peculiar energy around me!");
        }

        TRACE << "Finding summon candidates" << std::endl;
        std::vector<actor::Id> summon_bucket;

        for (size_t i = 0; i < (size_t)actor::Id::END; ++i) {
                const auto& data = actor::g_data[i];

                if (data.can_be_summoned_by_mon &&
                    data.spawn_min_dlvl <= map::g_dlvl + 3) {
                        summon_bucket.push_back((actor::Id)i);
                }
        }

        if (summon_bucket.empty()) {
                TRACE_VERBOSE << "No eligible candidates found" << std::endl;
        } else {
                // Eligible monsters found
                const auto id_to_summon = rnd::element(summon_bucket);

                TRACE_VERBOSE << "Actor id: " << int(id_to_summon) << std::endl;

                const auto summoned =
                        actor::spawn(m_pos, {id_to_summon}, map::rect())
                                .make_aware_of_player();

                std::for_each(
                        std::begin(summoned.monsters),
                        std::end(summoned.monsters),
                        [](auto* const mon) {
                                auto prop_summoned = new PropSummoned();

                                prop_summoned->set_indefinite();

                                mon->m_properties.apply(prop_summoned);

                                auto prop_waiting = new PropWaiting();

                                prop_waiting->set_duration(2);

                                mon->m_properties.apply(prop_waiting);

                                if (actor::can_player_see_actor(*mon)) {
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

void TrapHpSap::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        auto* const actor_here = map::first_actor_at_pos(m_pos);

        ASSERT(actor_here);

        if (!actor_here) {
                // Should never happen
                return;
        }

        const bool is_player = actor_here->is_player();
        const bool is_hidden = m_base_trap->is_hidden();

        TRACE_VERBOSE << "Is player: " << is_player << std::endl;

        if (!is_player) {
                TRACE_VERBOSE << "Not triggered by player" << std::endl;

                TRACE_FUNC_END_VERBOSE;

                return;
        }

        const bool can_see = actor_here->m_properties.allow_see();

        TRACE_VERBOSE << "Actor can see: " << can_see << std::endl;

        const std::string actor_name = actor_here->name_the();

        TRACE_VERBOSE << "Actor name: " << actor_name << std::endl;

        if (can_see) {
                std::string msg = "A beam of light shoots out from";

                if (!is_hidden) {
                        msg += " a curious shape on";
                }

                msg += " the floor!";

                msg_log::add(msg);
        } else {
                // Cannot see
                msg_log::add("I feel a peculiar energy around me!");
        }

        auto* const hp_sap = new PropHpSap();

        hp_sap->set_indefinite();

        actor_here->m_properties.apply(hp_sap);

        TRACE_FUNC_END_VERBOSE;
}

void TrapSpiSap::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        auto* const actor_here = map::first_actor_at_pos(m_pos);

        ASSERT(actor_here);

        if (!actor_here) {
                // Should never happen
                return;
        }

        const bool is_player = actor_here->is_player();
        const bool is_hidden = m_base_trap->is_hidden();

        TRACE_VERBOSE << "Is player: " << is_player << std::endl;

        if (!is_player) {
                TRACE_VERBOSE << "Not triggered by player" << std::endl;

                TRACE_FUNC_END_VERBOSE;

                return;
        }

        const bool can_see = actor_here->m_properties.allow_see();

        TRACE_VERBOSE << "Actor can see: " << can_see << std::endl;

        const std::string actor_name = actor_here->name_the();

        TRACE_VERBOSE << "Actor name: " << actor_name << std::endl;

        if (can_see) {
                std::string msg = "A beam of light shoots out from";

                if (!is_hidden) {
                        msg += " a curious shape on";
                }

                msg += " the floor!";

                msg_log::add(msg);
        } else {
                // Cannot see
                msg_log::add("I feel a peculiar energy around me!");
        }

        auto* const sp_sap = new PropSpiSap();

        sp_sap->set_indefinite();

        actor_here->m_properties.apply(sp_sap);

        TRACE_FUNC_END_VERBOSE;
}

void TrapSmoke::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::g_cells.at(m_pos).is_seen_by_player) {
                msg_log::add(
                        "A burst of smoke is released from a vent in the "
                        "floor!");
        }

        Snd snd("I hear a burst of gas.",
                audio::SfxId::gas,
                IgnoreMsgIfOriginSeen::yes,
                m_pos,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);

        explosion::run_smoke_explosion_at(m_pos);

        TRACE_FUNC_END_VERBOSE;
}

void TrapFire::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        if (map::g_cells.at(m_pos).is_seen_by_player) {
                msg_log::add("Flames burst out from a vent in the floor!");
        }

        Snd snd("I hear a burst of flames.",
                audio::SfxId::END,
                IgnoreMsgIfOriginSeen::yes,
                m_pos,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);

        explosion::run(
                m_pos,
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

        if (map::g_cells.at(m_pos).is_seen_by_player) {
                msg_log::add("An alarm sounds!");
        }

        Snd snd("I hear an alarm sounding!",
                audio::SfxId::END,
                IgnoreMsgIfOriginSeen::yes,
                m_pos,
                nullptr,
                SndVol::high,
                AlertsMon::yes);

        snd_emit::run(snd);

        TRACE_FUNC_END_VERBOSE;
}

void TrapWeb::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        auto* const actor_here = map::first_actor_at_pos(m_pos);

        ASSERT(actor_here);

        if (!actor_here) {
                return;
        }

        if (actor_here->is_player()) {
                if (actor_here->m_properties.allow_see()) {
                        msg_log::add(
                                "I am entangled in a spider web!");
                } else {
                        // Cannot see
                        msg_log::add(
                                "I am entangled in a sticky mass of threads!");
                }
        } else {
                // Is a monster
                if (actor::can_player_see_actor(*actor_here)) {
                        const std::string actor_name =
                                text_format::first_to_upper(
                                        actor_here->name_the());

                        msg_log::add(
                                actor_name +
                                " is entangled in a huge spider web!");
                }
        }

        Prop* const entangled = new PropEntangled();

        entangled->set_indefinite();

        actor_here->m_properties.apply(
                entangled,
                PropSrc::intr,
                false,
                Verbose::no);

        // Players getting stuck in spider webs alerts all spiders
        if (actor_here->is_player()) {
                for (auto* const actor : game_time::g_actors) {
                        if (actor->is_player() || !actor->m_data->is_spider) {
                                continue;
                        }

                        auto* const mon = static_cast<actor::Mon*>(actor);

                        mon->become_aware_player(actor::AwareSource::other);
                }
        }

        m_base_trap->destroy();

        TRACE_FUNC_END_VERBOSE;
}

void TrapSlow::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        auto* const actor_here = map::first_actor_at_pos(m_pos);

        ASSERT(actor_here);

        if (!actor_here) {
                // Should never happen
                return;
        }

        actor_here->m_properties.apply(new PropSlowed());

        TRACE_FUNC_END_VERBOSE;
}

void TrapCurse::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        auto* const actor_here = map::first_actor_at_pos(m_pos);

        ASSERT(actor_here);

        if (!actor_here) {
                // Should never happen
                return;
        }

        actor_here->m_properties.apply(new PropCursed());

        TRACE_FUNC_END_VERBOSE;
}

void TrapUnlearnSpell::trigger()
{
        TRACE_FUNC_BEGIN_VERBOSE;

        auto* const actor_here = map::first_actor_at_pos(m_pos);

        if (!actor_here) {
                // Should never happen
                ASSERT(false);

                return;
        }

        // TODO: Monsters could unlearn spells too
        if (!actor_here->is_player()) {
                return;
        }

        const bool can_see = actor_here->m_properties.allow_see();
        const bool is_hidden = m_base_trap->is_hidden();

        if (can_see) {
                std::string msg = "A beam of light shoots out from";

                if (!is_hidden) {
                        msg += " a curious shape on";
                }

                msg += " the floor!";

                msg_log::add(msg);
        } else {
                // Cannot see
                msg_log::add("I feel a peculiar energy around me!");
        }

        std::vector<SpellId> id_bucket;
        id_bucket.reserve((size_t)SpellId::END);

        for (int i = 0; i < (int)SpellId::END; ++i) {
                const auto id = (SpellId)i;

                // Only allow unlearning spells for which there exists a scroll
                bool is_scroll = false;

                for (const auto& d : item::g_data) {
                        if (d.spell_cast_from_scroll == id) {
                                is_scroll = true;

                                break;
                        }
                }

                if (player_spells::is_spell_learned(id) && is_scroll) {
                        id_bucket.push_back(id);
                }
        }

        if (id_bucket.empty()) {
                msg_log::add("There is no apparent effect.");

                return;
        }

        const auto id = rnd::element(id_bucket);

        player_spells::unlearn_spell(id, Verbose::yes);

        TRACE_FUNC_END_VERBOSE;
}

} // namespace terrain
