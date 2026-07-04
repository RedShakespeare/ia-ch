// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "spells.hpp"
#include "spells_internal.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "actor.hpp"
#include "actor_data.hpp"
#include "actor_death.hpp"
#include "actor_eat.hpp"
#include "actor_factory.hpp"
#include "actor_hit.hpp"
#include "actor_player_state.hpp"
#include "actor_see.hpp"
#include "array2.hpp"
#include "attack.hpp"
#include "audio.hpp"
#include "audio_data.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "direction.hpp"
#include "draw_blast.hpp"
#include "drop.hpp"
#include "explosion.hpp"
#include "flood.hpp"
#include "fov.hpp"
#include "game_time.hpp"
#include "gfx.hpp"
#include "global.hpp"
#include "i18n.hpp"
#include "inventory.hpp"
#include "inventory_handling.hpp"
#include "io.hpp"
#include "item.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "item_weapon.hpp"
#include "knockback.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "marker.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "pathfind.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "pos.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "property_handler.hpp"
#include "rect.hpp"
#include "sound.hpp"
#include "state.hpp"
#include "teleport.hpp"
#include "terrain.hpp"
#include "terrain_data.hpp"
#include "terrain_door.hpp"
#include "terrain_factory.hpp"
#include "terrain_trap.hpp"
#include "text_format.hpp"
#include "viewport.hpp"
#include "wpn_dmg.hpp"

static DmgType s_bolt_dmg_type = DmgType::blunt;


// -----------------------------------------------------------------------------
// Bolt spells
// -----------------------------------------------------------------------------
int SpellBolt::mon_cooldown() const
{
    return m_impl->mon_cooldown();
}

std::string SpellBolt::name() const
{
    return m_impl->name();
}

SpellId SpellBolt::id() const
{
    return m_impl->id();
}

SpellDomain SpellBolt::domain() const
{
    return SpellDomain::channeling;
}

SpellShock SpellBolt::shock_type() const
{
    return m_impl->shock_type();
}

std::vector<std::string> SpellBolt::descr_specific(const SpellSkill skill) const
{
    return m_impl->descr_specific(skill);
}

int SpellBolt::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)caster;

    return m_impl->base_max_cost(skill, caster);
}

bool SpellBolt::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

void SpellBolt::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    const int nr_projectiles = m_impl->nr_projectiles(skill);

    for (int i = 0; i < nr_projectiles; ++i) {
        map::update_vision();

        const std::vector<actor::Actor*>& current_seen_targets = actor::seen_foes(*caster);

        if (current_seen_targets.empty()) {
            if (actor::is_player(caster)) {
                msg_log::add(i18n::get(
                    "spells.dark_sphere_fizzles",
                    "A dark sphere materializes, but quickly fizzles out."));
            }

            break;
        }

        actor::Actor* const target =
            map::random_closest_actor(
                caster->m_pos,
                current_seen_targets);

        run_bolt_on_target(*caster, *target, skill, player_aware);

        if (!actor::is_alive(*map::g_player)) {
            break;
        }
    }
}

void SpellBolt::run_bolt_on_target(
    actor::Actor& caster,
    actor::Actor& target,
    SpellSkill skill,
    PlayerAwareOfCast player_aware) const
{
    Snd release_snd(
        i18n::get(
            "spells.darkbolt_release_sound",
            "I hear something rushing through the air."),
        audio::SfxId::darkbolt_release,
        IgnoreMsgIfOriginSeen::yes,
        caster.m_pos,
        &caster,
        SndVol::low,
        AlertsMon::yes);

    release_snd.run();

    if (release_snd.did_player_hear_sound()) {
        player_aware = PlayerAwareOfCast::yes;
    }

    // Spell resistance?
    if (target.m_properties.has(prop::Id::r_spell)) {
        on_resist(target);

        // Spell reflection?
        if (target.m_properties.has(prop::Id::spell_reflect)) {
            if (actor::can_player_see_actor(target)) {
                msg_log::add(spell_reflect_msg());
            }

            // Run a bolt with the target as caster, and the caster as target.
            run_bolt_on_target(target, caster, skill, player_aware);
        }

        return;
    }

    if (player_aware == PlayerAwareOfCast::yes) {
        draw_projectile_travel(caster, target, skill);
    }

    Snd impact_snd(
        i18n::get(
            "spells.impact_sound",
            "I hear an impact."),
        m_impl->impact_sfx(),
        IgnoreMsgIfOriginSeen::yes,
        target.m_pos,
        nullptr,
        SndVol::low,
        AlertsMon::yes);

    impact_snd.run();

    const P& target_p = target.m_pos;
    const bool player_see_pos = map::g_seen.at(target_p);
    const bool player_see_tgt = actor::can_player_see_actor(target);

    if (player_see_tgt || player_see_pos) {
        const int delay_div = (m_impl->nr_projectiles(skill) > 1) ? 4 : 2;

        draw_blast_at_cells({target.m_pos}, colors::magenta(), delay_div);

        Color msg_clr = colors::msg_good();

        std::string str_begin = i18n::get(
            "spells.projectile_hit_player_prefix",
            "I am");

        if (actor::is_player(&target)) {
            msg_clr = colors::msg_bad();
        }
        else {
            // Target is monster
            const std::string name_the =
                player_see_tgt
                ? text_format::first_to_upper(actor::name_the(target))
                : i18n::get("spells.projectile_hit_it", "It");

            str_begin =
                name_the +
                i18n::get("spells.projectile_hit_mon_suffix", " is");

            if (map::g_player->is_leader_of(&target)) {
                msg_clr = colors::white();
            }
        }

        const std::string hit_msg =
            str_begin +
            i18n::get("spells.projectile_hit_space", " ") +
            m_impl->hit_msg_ending();

        msg_log::add(hit_msg, msg_clr);
    }

    const Range dmg_range = m_impl->damage(skill);

    actor::hit(target, dmg_range.roll(), s_bolt_dmg_type, &caster, AllowWound::no);

    m_impl->on_hit(target, caster, skill);

    if (!actor::is_player(&target)) {
        target.become_aware_player(actor::AwareSource::spell_victim);
    }
}

void SpellBolt::draw_projectile_travel(
    const actor::Actor& caster,
    const actor::Actor& target,
    SpellSkill skill) const
{
    (void)skill;

    Array2<bool> blocked(map::dims());

    map_parsers::BlocksProjectiles()
        .run(blocked, blocked.rect());

    const auto flood = floodfill(caster.m_pos, blocked);

    const auto path = pathfind_with_flood(caster.m_pos, target.m_pos, flood);

    if (!path.empty()) {
        states::draw();

        const int idx_0 = (int)(path.size()) - 1;

        for (int i = idx_0; i > 0; --i) {
            const auto& p = path[i];

            if (!map::g_seen.at(p)) {
                continue;
            }

            states::draw();

            io::MapDrawObj draw_obj;
            draw_obj.tile = gfx::TileId::blast1;
            draw_obj.character = '*';
            draw_obj.pos = viewport::to_view_pos(p);
            draw_obj.color = colors::magenta();

            draw_obj.draw();

            io::update_screen();

            int delay = config::base_delay();

            if (m_impl->nr_projectiles(skill) > 1) {
                delay /= 2;
            }

            io::sleep(delay);
        }
    }
}

bool SpellBolt::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

SpellShock BoltImpl::shock_type() const
{
    return SpellShock::mild;
}

audio::SfxId BoltImpl::impact_sfx() const
{
    return audio::SfxId::darkbolt_impact;
}

void ForceBolt::on_hit(
    actor::Actor& actor_hit,
    actor::Actor& caster,
    const SpellSkill skill) const
{
    (void)actor_hit;
    (void)caster;
    (void)skill;
}

std::string ForceBolt::hit_msg_ending() const
{
    return i18n::get("spells.force_bolt.hit_msg_ending", "struck by a bolt!");
}

int ForceBolt::mon_cooldown() const
{
    return 3;
}

std::string ForceBolt::name() const
{
    return i18n::get("spells.force_bolt.name", "Force Bolt");
}

SpellId ForceBolt::id() const
{
    return SpellId::force_bolt;
}

int ForceBolt::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 2;
}

Range ForceBolt::damage(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {3, 4};  // Avg 3.5
    case SpellSkill::expert:       return {5, 7};  // Avg 6.0
    case SpellSkill::master:
    case SpellSkill::transcendent: return {9, 12};  // Avg 10.5
    }

    ASSERT(false);

    return {1, 1};
}

std::vector<std::string> ForceBolt::descr_specific(const SpellSkill skill) const
{
    (void)skill;

    return {};
}

std::string Darkbolt::hit_msg_ending() const
{
    return i18n::get("spells.darkbolt.hit_msg_ending", "struck by a blast!");
}

int Darkbolt::mon_cooldown() const
{
    return 5;
}

std::string Darkbolt::name() const
{
    return i18n::get("spells.darkbolt.name", "Darkbolt");
}

SpellId Darkbolt::id() const
{
    return SpellId::darkbolt;
}

int Darkbolt::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 4;
}

Range Darkbolt::damage(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {4, 9};   // Avg 6.5
    case SpellSkill::expert:       return {5, 11};  // Avg 8.0
    case SpellSkill::master:
    case SpellSkill::transcendent: return {6, 13};  // Avg 9.5
    }

    ASSERT(false);

    return {1, 1};
}

std::vector<std::string> Darkbolt::descr_specific(const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.darkbolt.descr",
            "A bolt of siphoned energy is hurled towards a target "
            "with great force. "
            "The conjured bolt has some will on its own - "
            "once released, it seeks creatures that pose a threat, "
            "precise control is therefore not possible."));

    const Range dmg_range = damage(skill);

    std::string effect_str =
        i18n::get("spells.darkbolt.impact_dmg_prefix", "The impact deals ") +
        dmg_range.str() +
        i18n::get("spells.darkbolt.impact_dmg_suffix", " damage.");

    if (skill >= SpellSkill::master) {
        effect_str += i18n::get(
            "spells.darkbolt.paralyze_burn",
            " The target is paralyzed and set aflame.");

        if (skill == SpellSkill::transcendent) {
            effect_str += i18n::get(
                "spells.darkbolt.distant_explosion",
                " If the target is sufficiently far away from "
                "the caster, the bolt explodes on impact.");
        }
    }
    else {
        // <= Expert
        effect_str += i18n::get(
            "spells.darkbolt.paralyze",
            " The target is paralyzed.");
    }

    descr.push_back(effect_str);

    return descr;
}

void Darkbolt::on_hit(
    actor::Actor& actor_hit,
    actor::Actor& caster,
    const SpellSkill skill) const
{
    if (skill == SpellSkill::transcendent) {
        const int dist = king_dist(caster.m_pos, actor_hit.m_pos);

        if (dist > g_expl_std_radi) {
            explosion::run(actor_hit.m_pos, ExplType::expl);
        }
    }

    if (!actor::is_alive(actor_hit)) {
        return;
    }

    if (!actor_hit.m_properties.is_resisting_dmg(s_bolt_dmg_type, Verbose::no)) {
        prop::Prop* paralyzed = prop::make(prop::Id::paralyzed);

        paralyzed->set_duration(rnd::range(1, 2));

        actor_hit.m_properties.apply(paralyzed);

        if (skill >= SpellSkill::master) {
            prop::Prop* burning = prop::make(prop::Id::burning);

            burning->set_duration(rnd::range(2, 3));

            actor_hit.m_properties.apply(burning);
        }
    }
}

int GnawingTorrent::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 4;
}

SpellShock GnawingTorrent::shock_type() const
{
    return SpellShock::disturbing;
}

Range GnawingTorrent::damage(const SpellSkill skill) const
{
    if (skill == SpellSkill::transcendent) {
        return {1, 2};
    }
    else {
        return {1, 1};
    }
}

int GnawingTorrent::nr_projectiles(const SpellSkill skill) const
{
    // Damage with 5/7/9/11 bolts dealing 1 damage per bolt (1-2 for transcendent skill):
    // Basic:        5
    // Expert:       7
    // Master:       9
    // Transcendent: Average 16.5 (11-22)

    return 5 + ((int)skill * 2);
}

int GnawingTorrent::mon_cooldown() const
{
    return 3;
}

void GnawingTorrent::on_hit(
    actor::Actor& actor_hit,
    actor::Actor& caster,
    const SpellSkill skill) const
{
    (void)actor_hit;
    (void)skill;

    if (actor::is_edible_living_creature(actor_hit)) {
        actor::restore_hp(caster, 1, actor::AllowRestoreAboveMax::yes, Verbose::no);
    }
}

std::string GnawingTorrent::hit_msg_ending() const
{
    return i18n::get("spells.gnawing_torrent.hit_msg_ending", "fed upon!");
}

audio::SfxId GnawingTorrent::impact_sfx() const
{
    return audio::SfxId::gnawing_torrent_impact;
}

std::string GnawingTorrent::name() const
{
    return i18n::get("spells.gnawing_torrent.name", "Gnawing Torrent");
}

SpellId GnawingTorrent::id() const
{
    return SpellId::gnawing_torrent;
}

std::vector<std::string> GnawingTorrent::descr_specific(const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(i18n::get(
        "spells.gnawing_torrent.descr",
        "Unleashes a stream of devouring energy upon the caster's victims."));

    descr.emplace_back(
        std::to_string(nr_projectiles(skill)) +
        i18n::get(
            "spells.gnawing_torrent.projectiles_prefix",
            " projectiles are conjured, each dealing ") +
        damage(skill).str() +
        i18n::get("spells.gnawing_torrent.projectiles_suffix", " damage."));

    descr.emplace_back(
        i18n::get(
            "spells.gnawing_torrent.life_feed_descr",
            "Each impact feeds life force back to the caster, providing 1 hit point "
            "(only against creatures of flesh and blood; "
            "ethereal creatures cannot be fed upon for example)."));

    descr.emplace_back(i18n::get(
        "spells.gnawing_torrent.above_max_hp_descr",
        "Hit points can be raised above the normal maximum level."));

    return descr;
}

// -----------------------------------------------------------------------------
// Azathoths wrath


// -----------------------------------------------------------------------------
// Azathoths wrath
// -----------------------------------------------------------------------------
int SpellAzaGaze::mon_cooldown() const
{
    return 6;
}

std::string SpellAzaGaze::name() const
{
    return i18n::get("spells.aza_gaze.name", "Azathoth's Gaze");
}

SpellId SpellAzaGaze::id() const
{
    return SpellId::aza_gaze;
}

SpellDomain SpellAzaGaze::domain() const
{
    return SpellDomain::channeling;
}

SpellShock SpellAzaGaze::shock_type() const
{
    return SpellShock::disturbing;
}

int SpellAzaGaze::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 8;
}

bool SpellAzaGaze::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

Range SpellAzaGaze::dmg_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {2, 5};  // Avg 3.5
    case SpellSkill::expert:       return {4, 8};  // Avg 6.0
    case SpellSkill::master:
    case SpellSkill::transcendent: return {6, 11};  // Avg 8.5
    }

    ASSERT(false);

    return {1, 1};
}

Range SpellAzaGaze::faint_duration_range(SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {2, 6};
    case SpellSkill::expert:       return {3, 7};
    case SpellSkill::master:
    case SpellSkill::transcendent: return {4, 8};
    }

    ASSERT(false);

    return {1, 1};
}

Range SpellAzaGaze::conflict_duration_range(SpellSkill skill) const
{
    (void)skill;

    return {10, 12};
}

void SpellAzaGaze::do_damage_on_target(
    actor::Actor& target,
    SpellSkill const skill,
    actor::Actor* const caster) const
{
    const int dmg = dmg_range(skill).roll();

    actor::hit(target, dmg, DmgType::explosion, caster, AllowWound::no);
}

void SpellAzaGaze::apply_properties_on_target(
    actor::Actor& target,
    SpellSkill skill) const
{
    if (!actor::is_alive(target)) {
        return;
    }

    {
        prop::Prop* prop = prop::make(prop::Id::fainted);

        const int duration = faint_duration_range(skill).roll();

        prop->set_duration(duration);

        target.m_properties.apply(prop);
    }

    if (skill >= SpellSkill::transcendent) {
        prop::Prop* prop = prop::make(prop::Id::conflict);

        const int duration = conflict_duration_range(skill).roll();

        prop->set_duration(duration);

        target.m_properties.apply(prop);
    }
}

void SpellAzaGaze::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    // TODO: Test with deaf player reading manuscript and no seen targets.
    Snd snd(
        i18n::get(
            "spells.aza_gaze_sound",
            "An insane cacophony resounds through the air!"),
        audio::SfxId::aza_gaze,
        IgnoreMsgIfOriginSeen::no,
        caster->m_pos,
        caster,
        SndVol::high,
        AlertsMon::no);

    snd.run();

    if (snd.did_player_hear_sound()) {
        player_aware = PlayerAwareOfCast::yes;
    }

    for (actor::Actor* const target : seen_targets) {
        run_effect_on_target(caster, *target, skill, player_aware);
    }
}

void SpellAzaGaze::run_effect_on_target(
    actor::Actor* const caster,
    actor::Actor& target,
    const SpellSkill skill,
    PlayerAwareOfCast player_aware) const
{
    // Spell resistance?
    if (target.m_properties.has(prop::Id::r_spell)) {
        on_resist(target);

        // Spell reflection?
        if (target.m_properties.has(prop::Id::spell_reflect)) {
            if (actor::can_player_see_actor(target)) {
                msg_log::add(spell_reflect_msg());
            }

            // Run effect with the target as caster, and the caster as seen target.
            run_effect(&target, skill, {caster}, player_aware);
        }

        return;
    }

    if (actor::can_player_see_actor(target)) {
        Color msg_clr = colors::msg_good();

        std::string hit_msg;

        if (actor::is_player(&target)) {
            hit_msg = i18n::get("spells.aza_gaze.player_hit_prefix", "I am");

            msg_clr = colors::msg_bad();
        }
        else {
            hit_msg =
                text_format::first_to_upper(actor::name_the(target)) +
                i18n::get("spells.aza_gaze.mon_hit_middle", " is");

            if (map::g_player->is_leader_of(&target)) {
                msg_clr = colors::white();
            }
        }

        hit_msg += i18n::get(
            "spells.aza_gaze.wracked_by_chaos_suffix",
            " wracked by chaos.");

        msg_log::add(hit_msg, msg_clr);

        draw_blast_at_cells({target.m_pos}, colors::light_red());
    }

    do_damage_on_target(target, skill, caster);

    if (!actor::is_player(&target)) {
        target.become_aware_player(actor::AwareSource::spell_victim);
    }

    apply_properties_on_target(target, skill);

    Snd snd(
        "",
        audio::SfxId::END,
        IgnoreMsgIfOriginSeen::yes,
        target.m_pos,
        nullptr,
        SndVol::high,
        AlertsMon::yes);

    snd.run();
}

std::vector<std::string> SpellAzaGaze::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.aza_gaze.descr",
            "Channels the chaos of Azathoth unto all visible enemies. "
            "The channel can only be opened for a fraction of a second, "
            "but even this is enough to cause great physical and mental "
            "devastation."));

    descr.push_back(
        i18n::get("spells.aza_gaze.dmg_prefix", "The spell deals ") +
        dmg_range(skill).str() +
        i18n::get("spells.aza_gaze.dmg_suffix", " damage to each creature."));

    descr.push_back(
        i18n::get(
            "spells.aza_gaze.faint_prefix",
            "Causes the victims to faint for ") +
        faint_duration_range(skill).str() +
        i18n::get(
            "spells.aza_gaze.faint_suffix",
            " turns, if they are susceptible."));

    if (skill == SpellSkill::transcendent) {
        descr.push_back(
            i18n::get(
                "spells.aza_gaze.conflict_prefix",
                "The victims become conflicted for ") +
            conflict_duration_range(skill).str() +
            i18n::get(
                "spells.aza_gaze.conflict_suffix",
                " turns, causing them to view any creature as "
                "their enemy."));
    }

    return descr;
}

bool SpellAzaGaze::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)mon;

    return !seen_targets.empty();
}

// -----------------------------------------------------------------------------
// Cataclysm


// -----------------------------------------------------------------------------
// Cataclysm
// -----------------------------------------------------------------------------
std::string SpellCataclysm::name() const
{
    return i18n::get("spells.cataclysm.name", "Cataclysm");
}

SpellId SpellCataclysm::id() const
{
    return SpellId::cataclysm;
}

SpellDomain SpellCataclysm::domain() const
{
    return SpellDomain::channeling;
}

SpellShock SpellCataclysm::shock_type() const
{
    return SpellShock::disturbing;
}

bool SpellCataclysm::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellCataclysm::destruction_radi(const SpellSkill skill) const
{
    return g_fov_radi_int + 1 + ((int)skill * 2);
}

int SpellCataclysm::nr_destruction_sweeps(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return 2;
    case SpellSkill::expert:
    case SpellSkill::master:
    case SpellSkill::transcendent: return 3;
    }

    ASSERT(false);

    return 1;
}

int SpellCataclysm::nr_explosions(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return 6;
    case SpellSkill::expert:       return 9;
    case SpellSkill::master:       return 12;
    case SpellSkill::transcendent: return 20;
    }

    ASSERT(false);

    return 1;
}

int SpellCataclysm::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 7;
}

void SpellCataclysm::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    const bool is_player = actor::is_player(caster);

    if (actor::can_player_see_actor(*caster)) {
        std::string caster_name =
            is_player
            ? i18n::get("spells.me", "me")
            : actor::name_the(*caster);

        msg_log::add(
            i18n::get(
                "spells.destruction_rages_prefix",
                "Destruction rages around ") +
            caster_name +
            i18n::get("spells.destruction_rages_suffix", "!"));
    }

    const auto& caster_pos = caster->m_pos;

    const int destr_radi = destruction_radi(skill);

    const R area(
        std::max(1, caster_pos.x - destr_radi),
        std::max(1, caster_pos.y - destr_radi),
        std::min(map::w() - 1, caster_pos.x + destr_radi) - 1,
        std::min(map::h() - 1, caster_pos.y + destr_radi) - 1);

    const auto positions = area.positions();

    // Run explosions
    std::vector<P> p_bucket;

    const int expl_radi_diff = -1;

    for (const auto& p : positions) {
        const auto* const terrain = map::g_terrain.at(p);

        if (!terrain->is_walkable()) {
            continue;
        }

        const int dist = king_dist(caster_pos, p);

        const int min_dist = g_expl_std_radi + 1 + expl_radi_diff;

        if (dist >= min_dist) {
            p_bucket.push_back(p);
        }
    }

    const int nr_expl = nr_explosions(skill);

    for (int i = 0; i < nr_expl; ++i) {
        if (p_bucket.empty()) {
            return;
        }

        const auto idx = rnd::range(0, (int)p_bucket.size() - 1);

        const auto& p = rnd::element(p_bucket);

        explosion::run(p, ExplType::expl, EmitExplSnd::yes, expl_radi_diff);

        p_bucket.erase(std::begin(p_bucket) + idx);
    }

    // Explode braziers
    for (const auto& p : positions) {
        const auto terrain_id = map::g_terrain.at(p)->id();

        if (terrain_id == terrain::Id::brazier) {
            Snd snd(
                i18n::get(
                    "spells.explosion_sound",
                    "I hear an explosion!"),
                audio::SfxId::explosion_molotov,
                IgnoreMsgIfOriginSeen::yes,
                p,
                nullptr,
                SndVol::high,
                AlertsMon::yes);

            snd.run();

            map::update_terrain(terrain::make(terrain::Id::rubble_low, p));

            prop::Prop* const burning = prop::make(prop::Id::burning);

            explosion::run(
                p,
                ExplType::apply_prop,
                EmitExplSnd::yes,
                0,
                ExplExclCenter::yes,
                {burning});
        }
    }

    // Destroy the surrounding environment
    const int nr_sweeps = nr_destruction_sweeps(skill);

    for (int i = 0; i < nr_sweeps; ++i) {
        for (const auto& p : positions) {
            if (!rnd::one_in(8)) {
                continue;
            }

            bool is_adj_to_walkable_cell = false;

            for (const P& d : dir_utils::g_dir_list) {
                const auto p_adj = p + d;

                if (map::g_terrain.at(p_adj)->is_walkable()) {
                    is_adj_to_walkable_cell = true;
                }
            }

            if (is_adj_to_walkable_cell) {
                map::g_terrain.at(p)->hit(DmgType::explosion, nullptr);
            }
        }
    }

    // Put blood, and set stuff on fire
    for (const auto& p : positions) {
        auto* const terrain = map::g_terrain.at(p);

        if (rnd::one_in(10)) {
            terrain->try_make_bloody();

            if (rnd::one_in(3)) {
                terrain->try_put_gore();
            }
        }

        if ((p != caster->m_pos) && rnd::one_in(6)) {
            terrain->hit(DmgType::fire, nullptr);
        }
    }

    Snd snd(
        "",
        audio::SfxId::END,
        IgnoreMsgIfOriginSeen::yes,
        caster_pos,
        nullptr,
        SndVol::high,
        AlertsMon::yes);

    snd.run();
}

std::vector<std::string> SpellCataclysm::descr_specific(
    const SpellSkill skill) const
{
    (void)skill;

    std::vector<std::string> descr;

    descr.emplace_back(i18n::get(
        "spells.cataclysm.descr",
        "Blasts the surrounding area with terrible force."));

    descr.emplace_back(i18n::get(
        "spells.cataclysm.skill_descr",
        "Higher skill levels increases the magnitude of the destruction."));

    return descr;
}

bool SpellCataclysm::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    // Always allow casting with a visible target.
    if (!seen_targets.empty()) {
        return true;
    }

    // Sometimes allow casting if monster has an unseen target.
    if (mon.m_ai_state.target && rnd::one_in(20)) {
        return true;
    }

    return false;
}

// -----------------------------------------------------------------------------
// Pestilence

