// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "actor_mon.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#include "actor_factory.hpp"
#include "actor_player.hpp"
#include "ai.hpp"
#include "attack.hpp"
#include "drop.hpp"
#include "flood.hpp"
#include "fov.hpp"
#include "game_time.hpp"
#include "gods.hpp"
#include "init.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_factory.hpp"
#include "knockback.hpp"
#include "line_calc.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "popup.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "reload.hpp"
#include "sound.hpp"
#include "terrain_door.hpp"
#include "terrain_mob.hpp"
#include "terrain_trap.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static void unblock_passable_doors(
        const actor::ActorData& actor_data,
        Array2<bool>& blocked)
{
        for (size_t i = 0; i < blocked.length(); ++i) {
                const auto* const t = map::g_cells.at(i).terrain;

                if (t->id() != terrain::Id::door) {
                        continue;
                }

                const auto* const door = static_cast<const terrain::Door*>(t);

                if (door->type() == DoorType::metal) {
                        continue;
                }

                if (actor_data.can_open_doors ||
                    actor_data.can_bash_doors) {
                        blocked.at(i) = false;
                }
        }
}

// -----------------------------------------------------------------------------
// actor
// -----------------------------------------------------------------------------
namespace actor {

std::string get_cultist_phrase()
{
        std::vector<std::string> phrase_bucket = {
                "Apigami!",
                "Bhuudesco invisuu!",
                "Bhuuesco marana!",
                "Crudux cruo!",
                "Cruento paashaeximus!",
                "Cruento pestis shatruex!",
                "Cruo crunatus durbe!",
                "Cruo lokemundux!",
                "Cruo stragara-na!",
                "Gero shay cruo!",
                "In marana domus-bhaava crunatus!",
                "Caecux infirmux!",
                "Malax sayti!",
                "Marana pallex!",
                "Marana malax!",
                "Pallex ti!",
                "Peroshay bibox malax!",
                "Pestis Cruento!",
                "Pestis cruento vilomaxus pretiacruento!",
                "Pretaanluxis cruonit!",
                "Pretiacruento!",
                "Stragar-Naya!",
                "Vorox esco marana!",
                "Vilomaxus!",
                "Prostragaranar malachtose!",
                "Apigami!"};

        if (rnd::one_in(4)) {
                const God& god = gods::current_god();

                const std::vector<std::string> god_phrases = {
                        god.name + " save us!",
                        god.descr + " will save us!",
                        god.name + " watches over us!",
                        god.descr + " watches over us!",
                        god.name + ", guide us!",
                        god.descr + " guides us!",
                        "For " + god.name + "!",
                        "For " + god.descr + "!",
                        "Blood for " + god.name + "!",
                        "Blood for " + god.descr + "!",
                        "Perish for " + god.name + "!",
                        "Perish for " + god.descr + "!",
                        "In the name of " + god.name + "!",
                };

                phrase_bucket.insert(
                        end(phrase_bucket),
                        begin(god_phrases),
                        end(god_phrases));
        }

        return rnd::element(phrase_bucket);
}

std::string get_cultist_aware_msg_seen(const Actor& actor)
{
        const std::string name_the =
                text_format::first_to_upper(
                        actor.name_the());

        return name_the + ": " + get_cultist_phrase();
}

std::string get_cultist_aware_msg_hidden()
{
        return "Voice: " + get_cultist_phrase();
}

// -----------------------------------------------------------------------------
// Monster
// -----------------------------------------------------------------------------
Mon::Mon() :

        m_wary_of_player_counter(0),
        m_aware_of_player_counter(0),
        m_player_aware_of_me_counter(0),
        m_is_msg_mon_in_view_printed(false),
        m_is_player_feeling_msg_allowed(true),
        m_last_dir_moved(Dir::center),
        m_is_roaming_allowed(MonRoamingAllowed::yes),
        m_leader(nullptr),
        m_target(nullptr),
        m_is_target_seen(false),
        m_waiting(false)

{
}

Mon::~Mon()
{
        for (auto& spell : m_spells) {
                delete spell.spell;
        }
}

void Mon::on_actor_turn()
{
        if (m_aware_of_player_counter > 0) {
                --m_wary_of_player_counter;

                --m_aware_of_player_counter;
        }
}

void Mon::act()
{
        const bool is_player_leader = is_actor_my_leader(map::g_player);

#ifndef NDEBUG
        // Sanity check - verify that monster is not outside the map
        if (!map::is_pos_inside_outer_walls(m_pos)) {
                TRACE << "Monster outside map" << std::endl;

                ASSERT(false);
        }

        // Sanity check - verify that monster's leader does not have a leader
        // (never allowed)
        if (m_leader && !is_player_leader) {
                const auto* const leader_mon = static_cast<Mon*>(m_leader);

                const auto* const leader_leader = leader_mon->m_leader;

                if (leader_leader) {
                        TRACE << "Monster with name '"
                              << name_a()
                              << "' has a leader with name '"
                              << m_leader->name_a()
                              << "', which also has a leader (not allowed!), "
                              << "with name '"
                              << leader_leader->name_a()
                              << "'"
                              << std::endl
                              << "Monster is summoned?: "
                              << m_properties.has(PropId::summoned)
                              << std::endl
                              << "Leader is summoned?: "
                              << m_leader->m_properties.has(PropId::summoned)
                              << std::endl
                              << "Leader's leader is summoned?: "
                              << leader_leader->m_properties.has(
                                         PropId::summoned)
                              << std::endl;

                        ASSERT(false);
                }
        }
#endif // NDEBUG

        if ((m_wary_of_player_counter <= 0) &&
            (m_aware_of_player_counter <= 0) &&
            !is_player_leader) {
                m_waiting = !m_waiting;

                if (m_waiting) {
                        game_time::tick();

                        return;
                }
        } else // Is wary/aware, or player is leader
        {
                m_waiting = false;
        }

        // Pick a target
        std::vector<Actor*> target_bucket;

        if (m_properties.has(PropId::conflict)) {
                target_bucket = seen_actors();

                m_is_target_seen = !target_bucket.empty();
        } else // Not conflicted
        {
                target_bucket = seen_foes();

                if (target_bucket.empty()) {
                        // There are no seen foes
                        m_is_target_seen = false;

                        target_bucket = unseen_foes_aware_of();
                } else // There are seen foes
                {
                        m_is_target_seen = true;
                }
        }

        // TODO: This just returns the actor with the closest COORDINATES,
        // not the actor with the shortest free path to it - so the monster
        // could select a target which is not actually the closest to reach.
        // This could perhaps lead to especially dumb situations if the monster
        // does not move by pathding, and considers an enemy behind a wall to be
        // closer than another enemy who is in the same room.
        m_target = map::random_closest_actor(m_pos, target_bucket);

        if ((m_wary_of_player_counter > 0) ||
            (m_aware_of_player_counter > 0)) {
                m_is_roaming_allowed = MonRoamingAllowed::yes;

                if (m_leader && m_leader->is_alive()) {
                        // Monster has a living leader
                        if ((m_aware_of_player_counter > 0) &&
                            !is_player_leader) {
                                make_leader_aware_silent();
                        }
                } else {
                        // Monster does not have a living leader

                        // Monster is wary or aware - occasionally make a sound
                        if (is_alive() && rnd::one_in(12)) {
                                speak_phrase(AlertsMon::no);
                        }
                }
        }

        // ---------------------------------------------------------------------
        // Special monster actions
        // ---------------------------------------------------------------------
        // TODO: This will be removed eventually
        if ((m_leader != map::g_player) &&
            (!m_target || m_target->is_player())) {
                if (on_act() == DidAction::yes) {
                        return;
                }
        }

        // ---------------------------------------------------------------------
        // Property actions (e.g. Zombie rising, Vortex pulling, ...)
        // ---------------------------------------------------------------------
        if (m_properties.on_act() == DidAction::yes) {
                return;
        }

        // ---------------------------------------------------------------------
        // Common actions (moving, attacking, casting spells, etc)
        // ---------------------------------------------------------------------

        // NOTE: Monsters try to detect the player visually on standard turns,
        // otherwise very fast monsters are much better at finding the player

        if (m_data->ai[(size_t)AiId::avoids_blocking_friend] &&
            !is_player_leader &&
            (m_target == map::g_player) &&
            m_is_target_seen &&
            rnd::coin_toss()) {
                if (ai::action::make_room_for_friend(*this)) {
                        return;
                }
        }

        // Cast instead of attacking?
        if (rnd::one_in(5)) {
                const bool did_cast = ai::action::try_cast_random_spell(*this);

                if (did_cast) {
                        return;
                }
        }

        if (m_data->ai[(size_t)AiId::attacks] &&
            m_target &&
            m_is_target_seen) {
                const auto did_attack = try_attack(*m_target);

                if (did_attack == DidAction::yes) {
                        return;
                }
        }

        if (rnd::fraction(3, 4)) {
                const bool did_cast = ai::action::try_cast_random_spell(*this);

                if (did_cast) {
                        return;
                }
        }

        int erratic_move_pct = (int)m_data->erratic_move_pct;

        // Never move erratically if frenzied
        if (m_properties.has(PropId::frenzied)) {
                erratic_move_pct = 0;
        }

        // Move less erratically if allied to player
        if (is_player_leader) {
                erratic_move_pct /= 2;
        }

        // Move more erratically if confused
        if (m_properties.has(PropId::confused) &&
            (erratic_move_pct > 0)) {
                erratic_move_pct += 50;
        }

        set_constr_in_range(0, erratic_move_pct, 95);

        // Occasionally move erratically
        if (m_data->ai[(size_t)AiId::moves_randomly_when_unaware] &&
            rnd::percent(erratic_move_pct)) {
                if (ai::action::move_to_random_adj_cell(*this)) {
                        return;
                }
        }

        const bool is_terrified = m_properties.has(PropId::terrified);

        if (m_data->ai[(size_t)AiId::moves_to_target_when_los] &&
            !is_terrified) {
                if (ai::action::move_to_target_simple(*this)) {
                        return;
                }
        }

        std::vector<P> path;

        if ((m_data->ai[(size_t)AiId::paths_to_target_when_aware] ||
             is_player_leader) &&
            !is_terrified) {
                path = ai::info::find_path_to_target(*this);
        }

        if (ai::action::handle_closed_blocking_door(*this, path)) {
                return;
        }

        if (ai::action::step_path(*this, path)) {
                return;
        }

        if ((m_data->ai[(size_t)AiId::moves_to_leader] ||
             is_player_leader) &&
            !is_terrified) {
                path = ai::info::find_path_to_leader(*this);

                if (ai::action::step_path(*this, path)) {
                        return;
                }
        }

        if (m_data->ai[(size_t)AiId::moves_to_lair] &&
            !is_player_leader &&
            (!m_target || m_target->is_player())) {
                if (ai::action::step_to_lair_if_los(*this, m_lair_pos)) {
                        return;
                } else // No LOS to lair
                {
                        // Try to use pathfinder to travel to lair
                        path = ai::info::find_path_to_lair_if_no_los(
                                *this,
                                m_lair_pos);

                        if (ai::action::step_path(*this, path)) {
                                return;
                        }
                }
        }

        // When unaware, move randomly
        if (m_data->ai[(size_t)AiId::moves_randomly_when_unaware] &&
            (!is_player_leader || rnd::one_in(8))) {
                if (ai::action::move_to_random_adj_cell(*this)) {
                        return;
                }
        }

        // No action could be performed, just let someone else act
        game_time::tick();
}

std::vector<Actor*> Mon::unseen_foes_aware_of() const
{
        std::vector<Actor*> result;

        if (is_actor_my_leader(map::g_player)) {
                // TODO: This prevents player-allied monsters from casting
                // spells on unreachable hostile monsters - but it probably
                // doesn't matter for now

                Array2<bool> blocked(map::dims());

                map_parsers::BlocksActor(*this, ParseActors::no)
                        .run(blocked, blocked.rect());

                unblock_passable_doors(*m_data, blocked);

                const auto flood = floodfill(m_pos, blocked);

                // Add all player-hostile monsters which the player is aware of
                for (Actor* const actor : game_time::g_actors) {
                        const P& p = actor->m_pos;

                        if (!actor->is_player() &&
                            !actor->is_actor_my_leader(map::g_player) &&
                            (flood.at(p) > 0)) {
                                auto* const mon = static_cast<Mon*>(actor);

                                if (mon->m_player_aware_of_me_counter > 0) {
                                        result.push_back(actor);
                                }
                        }
                }
        }
        // Player is not my leader
        else if (m_aware_of_player_counter > 0) {
                result.push_back(map::g_player);

                for (Actor* const actor : game_time::g_actors) {
                        if (!actor->is_player() &&
                            actor->is_actor_my_leader(map::g_player)) {
                                result.push_back(actor);
                        }
                }
        }

        return result;
}

bool Mon::can_see_actor(
        const Actor& other,
        const Array2<bool>& hard_blocked_los) const
{
        const bool is_seeable = is_actor_seeable(other, hard_blocked_los);

        if (!is_seeable) {
                return false;
        }

        if (is_actor_my_leader(map::g_player)) {
                // Monster is allied to player

                if (other.is_player()) {
                        // Player-allied monster looking at the player

                        return true;
                } else {
                        // Player-allied monster looking at other monster

                        const auto* const other_mon =
                                static_cast<const Mon*>(&other);

                        return other_mon->m_player_aware_of_me_counter > 0;
                }
        } else // Monster is hostile to player
        {
                return m_aware_of_player_counter > 0;
        }

        return true;
}

bool Mon::is_actor_seeable(
        const Actor& other,
        const Array2<bool>& hard_blocked_los) const
{
        if ((this == &other) || (!other.is_alive())) {
                return true;
        }

        // Outside FOV range?
        if (!fov::is_in_fov_range(m_pos, other.m_pos)) {
                // Other actor is outside FOV range
                return false;
        }

        // Monster is blind?
        if (!m_properties.allow_see()) {
                return false;
        }

        FovMap fov_map;
        fov_map.hard_blocked = &hard_blocked_los;
        fov_map.light = &map::g_light;
        fov_map.dark = &map::g_dark;

        const LosResult los = fov::check_cell(m_pos, other.m_pos, fov_map);

        // LOS blocked hard (e.g. a wall or smoke)?
        if (los.is_blocked_hard) {
                return false;
        }

        const bool can_see_invis = m_properties.has(PropId::see_invis);

        // Actor is invisible, and monster cannot see invisible?
        if ((other.m_properties.has(PropId::invis) ||
             other.m_properties.has(PropId::cloaked)) &&
            !can_see_invis) {
                return false;
        }

        bool has_darkvision = m_properties.has(PropId::darkvision);

        const bool can_see_other_in_dark = can_see_invis || has_darkvision;

        // Blocked by darkness, and not seeing actor with infravision?
        if (los.is_blocked_by_dark && !can_see_other_in_dark) {
                return false;
        }

        // OK, all checks passed, actor can bee seen
        return true;
}

std::vector<Actor*> Mon::seen_actors() const
{
        std::vector<Actor*> out;

        Array2<bool> blocked_los(map::dims());

        R los_rect(
                std::max(0, m_pos.x - g_fov_radi_int),
                std::max(0, m_pos.y - g_fov_radi_int),
                std::min(map::w() - 1, m_pos.x + g_fov_radi_int),
                std::min(map::h() - 1, m_pos.y + g_fov_radi_int));

        map_parsers::BlocksLos()
                .run(blocked_los,
                     los_rect,
                     MapParseMode::overwrite);

        for (Actor* actor : game_time::g_actors) {
                if ((actor != this) && actor->is_alive()) {
                        const Mon* const mon = static_cast<const Mon*>(this);

                        if (mon->can_see_actor(*actor, blocked_los)) {
                                out.push_back(actor);
                        }
                }
        }

        return out;
}

std::vector<Actor*> Mon::seen_foes() const
{
        std::vector<Actor*> out;

        Array2<bool> blocked_los(map::dims());

        R los_rect(
                std::max(0, m_pos.x - g_fov_radi_int),
                std::max(0, m_pos.y - g_fov_radi_int),
                std::min(map::w() - 1, m_pos.x + g_fov_radi_int),
                std::min(map::h() - 1, m_pos.y + g_fov_radi_int));

        map_parsers::BlocksLos()
                .run(blocked_los,
                     los_rect,
                     MapParseMode::overwrite);

        for (Actor* actor : game_time::g_actors) {
                if ((actor != this) &&
                    actor->is_alive()) {
                        const bool is_hostile_to_player =
                                !is_actor_my_leader(map::g_player);

                        const bool is_other_hostile_to_player =
                                actor->is_player()
                                ? false
                                : !actor->is_actor_my_leader(map::g_player);

                        const bool is_enemy =
                                is_hostile_to_player !=
                                is_other_hostile_to_player;

                        const Mon* const mon = static_cast<const Mon*>(this);

                        if (is_enemy &&
                            mon->can_see_actor(*actor, blocked_los)) {
                                out.push_back(actor);
                        }
                }
        }

        return out;
}

std::vector<Actor*> Mon::seeable_foes() const
{
        std::vector<Actor*> out;

        Array2<bool> blocked_los(map::dims());

        const R fov_rect = fov::fov_rect(m_pos, blocked_los.dims());

        map_parsers::BlocksLos()
                .run(blocked_los,
                     fov_rect,
                     MapParseMode::overwrite);

        for (Actor* actor : game_time::g_actors) {
                if ((actor != this) &&
                    actor->is_alive()) {
                        const bool is_hostile_to_player =
                                !is_actor_my_leader(map::g_player);

                        const bool is_other_hostile_to_player =
                                actor->is_player()
                                ? false
                                : !actor->is_actor_my_leader(map::g_player);

                        const bool is_enemy =
                                is_hostile_to_player !=
                                is_other_hostile_to_player;

                        if (is_enemy &&
                            is_actor_seeable(*actor, blocked_los)) {
                                out.push_back(actor);
                        }
                }
        }

        return out;
}

bool Mon::is_sneaking() const
{
        // NOTE: We require a stealth ability greater than zero, both for the
        // basic skill value, AND when including properties properties -
        // This prevents monsters who should never be able to sneak from
        // suddenly gaining this ability due to some bonus

        const int stealth_base = ability(AbilityId::stealth, false);

        const int stealth_current = ability(AbilityId::stealth, true);

        return (m_player_aware_of_me_counter <= 0) &&
                (stealth_base > 0) &&
                (stealth_current > 0) &&
                !is_actor_my_leader(map::g_player);
}

void Mon::make_leader_aware_silent() const
{
        ASSERT(m_leader);

        if (!m_leader) {
                return;
        }

        Mon* const leader_mon = static_cast<Mon*>(m_leader);

        leader_mon->m_aware_of_player_counter =
                std::max(
                        leader_mon->m_data->nr_turns_aware,
                        leader_mon->m_aware_of_player_counter);
}

void Mon::on_std_turn()
{
        // Countdown all spell cooldowns
        for (auto& spell : m_spells) {
                int& cooldown = spell.cooldown;

                if (cooldown > 0) {
                        --cooldown;
                }
        }

        // Monsters try to detect the player visually on standard turns,
        // otherwise very fast monsters are much better at finding the player
        if (is_alive() &&
            m_data->ai[(size_t)AiId::looks] &&
            (m_leader != map::g_player) &&
            (!m_target || m_target->is_player())) {
                const bool did_become_aware_now = ai::info::look(*this);

                // If the monster became aware, give it some reaction time
                if (did_become_aware_now) {
                        auto prop = new PropWaiting();

                        prop->set_duration(1);

                        m_properties.apply(prop);
                }
        }

        on_std_turn_hook();
}

void Mon::on_hit(
        int& dmg,
        const DmgType dmg_type,
        const DmgMethod method,
        const AllowWound allow_wound)
{
        (void)dmg;
        (void)dmg_type;
        (void)method;
        (void)allow_wound;

        m_aware_of_player_counter =
                std::max(
                        m_data->nr_turns_aware,
                        m_aware_of_player_counter);
}

Color Mon::color() const
{
        if (m_state != ActorState::alive) {
                return m_data->color;
        }

        Color tmp_color;

        if (m_properties.affect_actor_color(tmp_color)) {
                return tmp_color;
        }

        return m_data->color;
}

SpellSkill Mon::spell_skill(const SpellId id) const
{
        (void)id;

        for (const auto& spell : m_spells) {
                if (spell.spell->id() == id) {
                        return spell.skill;
                }
        }

        ASSERT(false);

        return SpellSkill::basic;
}

void Mon::hear_sound(const Snd& snd)
{
        if (m_properties.has(PropId::deaf)) {
                return;
        }

        snd.on_heard(*this);

        // The monster may have become deaf through the sound callback (e.g.
        // from the Horn of Deafening artifact)
        if (m_properties.has(PropId::deaf)) {
                return;
        }

        if (is_alive() && snd.is_alerting_mon()) {
                const bool was_aware_before = (m_aware_of_player_counter > 0);

                become_aware_player(false);

                // Give the monster some reaction time
                if (!was_aware_before && !is_actor_my_leader(map::g_player)) {
                        auto prop = new PropWaiting();

                        prop->set_duration(1);

                        m_properties.apply(prop);
                }
        }
}

void Mon::speak_phrase(const AlertsMon alerts_others)
{
        const bool is_seen_by_player = map::g_player->can_see_actor(*this);

        std::string msg =
                is_seen_by_player
                ? aware_msg_mon_seen()
                : aware_msg_mon_hidden();

        msg = text_format::first_to_upper(msg);

        const SfxId sfx =
                is_seen_by_player
                ? aware_sfx_mon_seen()
                : aware_sfx_mon_hidden();

        Snd snd(
                msg,
                sfx,
                IgnoreMsgIfOriginSeen::no,
                m_pos,
                this,
                SndVol::low,
                alerts_others);

        snd_emit::run(snd);
}

std::string Mon::aware_msg_mon_seen() const
{
        if (m_data->use_cultist_aware_msg_mon_seen) {
                return get_cultist_aware_msg_seen(*this);
        }

        std::string msg_end = m_data->aware_msg_mon_seen;

        if (msg_end.empty()) {
                return "";
        }

        const std::string name = text_format::first_to_upper(name_the());

        return name + " " + msg_end;
}

std::string Mon::aware_msg_mon_hidden() const
{
        if (m_data->use_cultist_aware_msg_mon_hidden) {
                return get_cultist_aware_msg_hidden();
        }

        return m_data->aware_msg_mon_hidden;
}

void Mon::become_aware_player(const bool is_from_seeing, const int factor)
{
        if (!is_alive() || is_actor_my_leader(map::g_player)) {
                return;
        }

        const int nr_turns = m_data->nr_turns_aware * factor;

        const int aware_counter_before = m_aware_of_player_counter;

        m_aware_of_player_counter =
                std::max(nr_turns, aware_counter_before);

        m_wary_of_player_counter = m_aware_of_player_counter;

        if (aware_counter_before <= 0) {
                if (is_from_seeing &&
                    map::g_player->can_see_actor(*this)) {
                        print_player_see_mon_become_aware_msg();
                }

                if (rnd::coin_toss()) {
                        speak_phrase(AlertsMon::yes);
                }
        }
}

void Mon::become_wary_player()
{
        if (!is_alive() || is_actor_my_leader(map::g_player)) {
                return;
        }

        // NOTE: Reusing aware duration to determine number of wary turns
        const int nr_turns = m_data->nr_turns_aware;

        const int wary_counter_before = m_wary_of_player_counter;

        m_wary_of_player_counter =
                std::max(nr_turns, wary_counter_before);

        if (wary_counter_before <= 0) {
                if (map::g_player->can_see_actor(*this)) {
                        print_player_see_mon_become_wary_msg();
                }

                if (rnd::one_in(4)) {
                        speak_phrase(AlertsMon::no);
                }
        }
}

void Mon::print_player_see_mon_become_aware_msg() const
{
        std::string msg = text_format::first_to_upper(name_the()) + " sees me!";

        const std::string dir_str =
                dir_utils::compass_dir_name(map::g_player->m_pos, m_pos);

        msg += "(" + dir_str + ")";

        msg_log::add(msg);
}

void Mon::print_player_see_mon_become_wary_msg() const
{
        if (m_data->wary_msg.empty()) {
                return;
        }

        std::string msg = text_format::first_to_upper(name_the());

        msg += " " + m_data->wary_msg;

        msg += "(";
        msg += dir_utils::compass_dir_name(map::g_player->m_pos, m_pos);
        msg += ")";

        msg_log::add(msg);
}

void Mon::set_player_aware_of_me(int duration_factor)
{
        int nr_turns = 2 * duration_factor;

        if (player_bon::bg() == Bg::rogue) {
                nr_turns *= 8;
        }

        m_player_aware_of_me_counter =
                std::max(m_player_aware_of_me_counter, nr_turns);
}

DidAction Mon::try_attack(Actor& defender)
{
        if (m_state != ActorState::alive ||
            ((m_aware_of_player_counter <= 0) &&
             (m_leader != map::g_player))) {
                return DidAction::no;
        }

        map::update_vision();

        const AiAvailAttacksData my_avail_attacks = avail_attacks(defender);

        const AiAttData att = choose_attack(my_avail_attacks);

        if (!att.wpn) {
                return DidAction::no;
        }

        if (att.is_melee) {
                if (att.wpn->data().melee.is_melee_wpn) {
                        attack::melee(this, m_pos, defender, *att.wpn);

                        return DidAction::yes;
                }

                return DidAction::no;
        }

        if (att.wpn->data().ranged.is_ranged_wpn) {
                if (my_avail_attacks.should_reload) {
                        reload::try_reload(*this, att.wpn);

                        return DidAction::yes;
                }

                const bool ignore_blocking_friend = rnd::one_in(20);

                if (!ignore_blocking_friend &&
                    is_friend_blocking_ranged_attack(defender.m_pos)) {
                        return DidAction::no;
                }

                if (m_data->ranged_cooldown_turns > 0) {
                        auto prop = new PropDisabledRanged();

                        prop->set_duration(m_data->ranged_cooldown_turns);

                        m_properties.apply(prop);
                }

                const auto did_attack =
                        attack::ranged(
                                this,
                                m_pos,
                                defender.m_pos,
                                *att.wpn);

                return did_attack;
        }

        return DidAction::no;
}

bool Mon::is_friend_blocking_ranged_attack(const P& target_pos) const
{
        const auto line =
                line_calc::calc_new_line(
                        m_pos,
                        target_pos,
                        true,
                        9999,
                        false);

        for (const P& line_pos : line) {
                if ((line_pos != m_pos) && (line_pos != target_pos)) {
                        auto* const actor_here =
                                map::first_actor_at_pos(line_pos);

                        // TODO: This does not consider who is allied/hostile!
                        if (actor_here) {
                                return true;
                        }
                }
        }

        return false;
}

AiAvailAttacksData Mon::avail_attacks(Actor& defender) const
{
        AiAvailAttacksData result;

        if (!m_properties.allow_attack(Verbose::no)) {
                return result;
        }

        result.is_melee = is_pos_adj(m_pos, defender.m_pos, false);

        if (result.is_melee) {
                if (!m_properties.allow_attack_melee(Verbose::no)) {
                        return result;
                }

                result.weapons = avail_intr_melee();

                auto* wielded_wpn = avail_wielded_melee();

                if (wielded_wpn) {
                        result.weapons.push_back(wielded_wpn);
                }
        } else // Ranged attack
        {
                if (!m_properties.allow_attack_ranged(Verbose::no)) {
                        return result;
                }

                result.weapons = avail_intr_ranged();

                auto* const wielded_wpn = avail_wielded_ranged();

                if (wielded_wpn) {
                        result.weapons.push_back(wielded_wpn);

                        result.should_reload = should_reload(*wielded_wpn);
                }
        }

        return result;
}

item::Wpn* Mon::avail_wielded_melee() const
{
        auto* const item = m_inv.item_in_slot(SlotId::wpn);

        if (item) {
                auto* const wpn = static_cast<item::Wpn*>(item);

                if (wpn->data().melee.is_melee_wpn) {
                        return wpn;
                }
        }

        return nullptr;
}

item::Wpn* Mon::avail_wielded_ranged() const
{
        auto* const item = m_inv.item_in_slot(SlotId::wpn);

        if (item) {
                auto* const wpn = static_cast<item::Wpn*>(item);

                if (wpn->data().ranged.is_ranged_wpn) {
                        return wpn;
                }
        }

        return nullptr;
}

std::vector<item::Wpn*> Mon::avail_intr_melee() const
{
        std::vector<item::Wpn*> result;

        for (auto* const item : m_inv.m_intrinsics) {
                auto* wpn = static_cast<item::Wpn*>(item);

                if (wpn->data().melee.is_melee_wpn) {
                        result.push_back(wpn);
                }
        }

        return result;
}

std::vector<item::Wpn*> Mon::avail_intr_ranged() const
{
        std::vector<item::Wpn*> result;

        for (auto* const item : m_inv.m_intrinsics) {
                auto* const wpn = static_cast<item::Wpn*>(item);

                if (wpn->data().ranged.is_ranged_wpn) {
                        result.push_back(wpn);
                }
        }

        return result;
}

bool Mon::should_reload(const item::Wpn& wpn) const
{
        // TODO: This could be made more sophisticated, e.g. if the monster does
        // not see any enemies it should reload even if the weapon is not
        // completely empty
        return (wpn.m_ammo_loaded == 0) &&
                !wpn.data().ranged.has_infinite_ammo &&
                m_inv.has_ammo_for_firearm_in_inventory();
}

AiAttData Mon::choose_attack(const AiAvailAttacksData& avail_attacks) const
{
        AiAttData result;

        result.is_melee = avail_attacks.is_melee;

        if (avail_attacks.weapons.empty()) {
                return result;
        }

        result.wpn = rnd::element(avail_attacks.weapons);

        return result;
}

bool Mon::is_leader_of(const Actor* const actor) const
{
        if (actor && !actor->is_player()) {
                const auto* const mon = static_cast<const Mon*>(actor);

                return (mon->m_leader == this);
        }

        return false;
}

bool Mon::is_actor_my_leader(const Actor* const actor) const
{
        return m_leader == actor;
}

int Mon::nr_mon_in_group() const
{
        const Actor* const group_leader = m_leader ? m_leader : this;

        int ret = 1; // Starting at one to include leader

        for (const Actor* const actor : game_time::g_actors) {
                if (actor->is_actor_my_leader(group_leader)) {
                        ++ret;
                }
        }

        return ret;
}

void Mon::add_spell(SpellSkill skill, Spell* const spell)
{
        const auto search = std::find_if(
                begin(m_spells),
                end(m_spells),
                [spell](MonSpell& spell_entry) {
                        return spell_entry.spell->id() == spell->id();
                });

        if (search != end(m_spells)) {
                delete spell;

                return;
        }

        MonSpell spell_entry;

        spell_entry.spell = spell;

        spell_entry.skill = skill;

        m_spells.push_back(spell_entry);
}

// -----------------------------------------------------------------------------
// Specific monsters
// -----------------------------------------------------------------------------
// TODO: This should either be a property or be controlled by the map
DidAction Khephren::on_act()
{
        // Summon locusts
        if (!is_alive() ||
            (m_aware_of_player_counter <= 0) ||
            m_has_summoned_locusts) {
                return DidAction::no;
        }

        Array2<bool> blocked(map::dims());

        const R fov_rect = fov::fov_rect(m_pos, blocked.dims());

        map_parsers::BlocksLos()
                .run(blocked,
                     fov_rect,
                     MapParseMode::overwrite);

        if (!can_see_actor(*(map::g_player), blocked)) {
                return DidAction::no;
        }

        msg_log::add("Khephren calls a plague of Locusts!");

        map::g_player->incr_shock(ShockLvl::terrifying, ShockSrc::misc);

        Actor* const leader_of_spawned_mon = m_leader ? m_leader : this;

        const size_t nr_of_spawns = 15;

        auto summoned =
                actor::spawn(
                        m_pos,
                        {nr_of_spawns, actor::Id::locust},
                        map::rect());

        summoned.set_leader(leader_of_spawned_mon);
        summoned.make_aware_of_player();

        std::for_each(
                std::begin(summoned.monsters),
                std::end(summoned.monsters),
                [](Mon* const mon) {
                        auto prop = new PropSummoned();

                        prop->set_indefinite();

                        mon->m_properties.apply(prop);
                });

        m_has_summoned_locusts = true;

        game_time::tick();

        return DidAction::yes;
}

// TODO: Make this into a spell instead
DidAction Ape::on_act()
{
        if (m_frenzy_cooldown > 0) {
                --m_frenzy_cooldown;
        }

        if ((m_frenzy_cooldown <= 0) &&
            m_target &&
            (m_hp <= (actor::max_hp(*this) / 2))) {
                m_frenzy_cooldown = 30;

                auto prop = new PropFrenzied();

                prop->set_duration(rnd::range(4, 6));

                m_properties.apply(prop);
        }

        return DidAction::no;
}

// TODO: This should be a property (should probably be merged with
// 'corrupts_env_color')
Color StrangeColor::color() const
{
        Color color = colors::light_magenta();

        const Range range(40, 255);

        color.set_rgb(
                range.roll(),
                range.roll(),
                range.roll());

        return color;
}

SpectralWpn::SpectralWpn() :
        Mon()
{}

void SpectralWpn::on_death()
{
        // Remove the item from the inventory to avoid dropping it on the floor
        // (but do not yet delete the item, in case it's still being used in the
        // the call stack)
        auto* const item =
                m_inv.remove_item_in_slot(
                        SlotId::wpn,
                        false); // Do not delete the item

        m_discarded_item.reset(item);
}

std::string SpectralWpn::name_the() const
{
        auto* item = m_inv.item_in_slot(SlotId::wpn);

        ASSERT(item);

        const std::string name = item->name(
                ItemRefType::plain,
                ItemRefInf::yes,
                ItemRefAttInf::none);

        return "The Spectral " + name;
}

std::string SpectralWpn::name_a() const
{
        auto* item = m_inv.item_in_slot(SlotId::wpn);

        ASSERT(item);

        const std::string name = item->name(
                ItemRefType::plain,
                ItemRefInf::yes,
                ItemRefAttInf::none);

        return "A Spectral " + name;
}

char SpectralWpn::character() const
{
        auto* item = m_inv.item_in_slot(SlotId::wpn);

        ASSERT(item);

        return item->character();
}

TileId SpectralWpn::tile() const
{
        auto* item = m_inv.item_in_slot(SlotId::wpn);

        ASSERT(item);

        return item->tile();
}

std::string SpectralWpn::descr() const
{
        auto* item = m_inv.item_in_slot(SlotId::wpn);

        ASSERT(item);

        std::string str = item->name(
                ItemRefType::a,
                ItemRefInf::yes,
                ItemRefAttInf::none);

        str = text_format::first_to_upper(str);

        str += ", floating through the air as if wielded by an invisible hand.";

        return str;
}

} // namespace actor
