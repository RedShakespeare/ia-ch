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


// -----------------------------------------------------------------------------
// Bless
// -----------------------------------------------------------------------------
std::string SpellBless::name() const
{
    return i18n::get("spells.bless.name", "Bless");
}

SpellId SpellBless::id() const
{
    return SpellId::bless;
}

SpellDomain SpellBless::domain() const
{
    return SpellDomain::warding;
}

bool SpellBless::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

SpellShock SpellBless::shock_type() const
{
    return SpellShock::mild;
}

Range SpellBless::duration_range(SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:  return {15, 30};
    case SpellSkill::expert: return {60, 120};
    case SpellSkill::master: return {150, 300};

    case SpellSkill::transcendent:
        // Unexpected, the spell should be indefinite
        break;
    }

    ASSERT(false);

    return {1, 1};
}

int SpellBless::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 6;
}

void SpellBless::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    prop::Prop* prop = prop::make(prop::Id::blessed);

    if (skill == SpellSkill::transcendent) {
        prop->set_indefinite();
    }
    else {
        prop->set_duration(duration_range(skill).roll());
    }

    caster->m_properties.apply(prop);
}

std::vector<std::string> SpellBless::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.bless.descr",
            "The caster becomes more lucky "
            "(+10% to hit chance, evasion, stealth, and searching)."));

    if (skill == SpellSkill::transcendent) {
        descr.emplace_back(spell_indefinite_duration_descr());
    }
    else {
        descr.push_back(spell_duration_descr(duration_range(skill).str()));
    }

    return descr;
}

// -----------------------------------------------------------------------------
// Cancellation


// -----------------------------------------------------------------------------
// Cancellation
// -----------------------------------------------------------------------------
std::string SpellCancellation::name() const
{
    return i18n::get("spells.cancellation.name", "Cancellation");
}

SpellId SpellCancellation::id() const
{
    return SpellId::cancellation;
}

SpellDomain SpellCancellation::domain() const
{
    return SpellDomain::warding;
}

bool SpellCancellation::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

SpellShock SpellCancellation::shock_type() const
{
    return SpellShock::mild;
}

int SpellCancellation::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 4;
}

int SpellCancellation::max_dist(SpellSkill skill) const
{
    return 3 + ((int)skill * 2);
}

Range SpellCancellation::damage_for_vulnerable_creatures() const
{
    return {1, 4};
}

std::vector<CancelledPropData> SpellCancellation::negative_effect_types_cancelled() const
{
    // NOTE: Do not overlap with the Heal spell. Keep it to more "magical" or mental effects,
    // rather than physical/mundane things like poisoning.
    return {
        {prop::Id::cursed},
        {prop::Id::doomed},
        {prop::Id::slowed},
        {prop::Id::terrified},
        {prop::Id::confused},
        {prop::Id::fainted},
        {prop::Id::conflict},
        {prop::Id::hallucinating},
    };
}

std::vector<CancelledPropData> SpellCancellation::positive_effect_types_cancelled() const
{
    return {
        {prop::Id::r_spell,
         CancelledPropIncludeInDescr::no,
         CancelledPropAllowCancelPermanent::yes},
        {prop::Id::r_phys, CancelledPropIncludeInDescr::no},
        {prop::Id::r_fire, CancelledPropIncludeInDescr::no},
        {prop::Id::r_poison, CancelledPropIncludeInDescr::no},
        {prop::Id::r_elec, CancelledPropIncludeInDescr::no},
        {prop::Id::r_sleep, CancelledPropIncludeInDescr::no},
        {prop::Id::r_fear, CancelledPropIncludeInDescr::no},
        {prop::Id::r_slow, CancelledPropIncludeInDescr::no},
        {prop::Id::r_conf, CancelledPropIncludeInDescr::no},
        {prop::Id::blessed},
        {prop::Id::hasted},
        {prop::Id::extra_hasted, CancelledPropIncludeInDescr::no},
        {prop::Id::frenzied},
        {prop::Id::cloaked},
        {prop::Id::invis},
        {prop::Id::premonition},
        {prop::Id::erudition},
        {prop::Id::magic_carapace},
        {prop::Id::extra_skill},
    };
}

void SpellCancellation::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    // TODO: What does it look like when casting from an unknown manuscript with nothing to
    // cancel and no vulnerable creatures?

    const int dist = max_dist(skill);

    std::vector<actor::Actor*> affected_actors {caster};

    for (actor::Actor* const actor : game_time::g_actors) {
        if ((actor != caster) &&
            actor::is_alive(*actor) &&
            (king_dist(caster->m_pos, actor->m_pos) <= dist)) {
            affected_actors.push_back(actor);
        }
    }

    for (actor::Actor* const actor : affected_actors) {
        if (!actor::is_alive(*actor)) {
            continue;
        }

        if (king_dist(caster->m_pos, actor->m_pos) > dist) {
            continue;
        }

        run_effect_on_actor(*actor, *caster);

        if (!actor::is_alive(*map::g_player)) {
            break;
        }
    }
}

void SpellCancellation::run_effect_on_actor(
    actor::Actor& actor,
    actor::Actor& caster) const
{
    TRACE << "Cancelling effects on actor '" << actor::name_a(actor) << "'" << std::endl;

    if (actor::is_allied(&caster, &actor)) {
        TRACE << "Caster and target are allied" << std::endl;

        cancel_negative_effects(actor);
    }
    else {
        TRACE << "Caster and target are enemies" << std::endl;

        cancel_positive_effects(actor);

        if (actor::is_alive(actor) && actor::is_alive(*map::g_player)) {
            do_damage_vulnerable_creature(actor, caster);
        }
    }
}

void SpellCancellation::cancel_negative_effects(actor::Actor& actor) const
{
    TRACE << "Cancelling negative effects" << std::endl;

    for (const CancelledPropData& data : negative_effect_types_cancelled()) {
        TRACE << "Ending '" << prop::g_data[(size_t)data.id].name << "'\n";

        if (data.allow_cancel_permanent_effect == CancelledPropAllowCancelPermanent::yes) {
            actor.m_properties.end_prop(data.id);
        }
        else {
            actor.m_properties.end_temporary_prop(data.id);
        }

        if (!actor::is_alive(actor) || !actor::is_alive(*map::g_player)) {
            return;
        }
    }
}

void SpellCancellation::cancel_positive_effects(actor::Actor& actor) const
{
    TRACE << "Cancelling positive effects" << std::endl;

    for (const CancelledPropData& data : positive_effect_types_cancelled()) {
        TRACE << "Ending '" << prop::g_data[(size_t)data.id].name << "'\n";

        bool did_end = false;

        if (data.allow_cancel_permanent_effect == CancelledPropAllowCancelPermanent::yes) {
            did_end = actor.m_properties.end_prop(data.id);
        }
        else {
            did_end = actor.m_properties.end_temporary_prop(data.id);
        }

        if (did_end) {
            // The spell "pierces through" spell shield, but a player with the
            // absorption trait shall still receive spirit points (this is also
            // consistent with the absorption trait description).
            if (data.id == prop::Id::r_spell &&
                actor::is_player(&actor) &&
                player_bon::has_trait(TraitId::absorption)) {
                give_player_sp_for_resist_with_absorption_trait();
            }
        }

        if (!actor::is_alive(actor) || !actor::is_alive(*map::g_player)) {
            return;
        }
    }
}

void SpellCancellation::do_damage_vulnerable_creature(
    actor::Actor& actor,
    actor::Actor& caster) const
{
    const std::vector<prop::Id> vulnerable_props = {
        prop::Id::outer_being,
        prop::Id::undead,
        prop::Id::summoned,
    };

    if (!actor.m_properties.has_any(vulnerable_props)) {
        return;
    }

    if (actor::can_player_see_actor(actor)) {
        const std::string name = text_format::first_to_lower(actor::name_the(actor));

        msg_log::add(
            name +
            i18n::get("spells.unravels_suffix", " unravels."));
    }

    const int dmg = damage_for_vulnerable_creatures().roll();

    actor::hit(actor, dmg, DmgType::pure, &caster);

    if (!actor::is_player(&actor)) {
        actor.become_aware_player(actor::AwareSource::spell_victim);
    }
}

std::vector<std::string> SpellCancellation::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr = {
        i18n::get(
            "spells.cancellation.descr_main",
            "Cancels temporary effects on nearby creatures. "
            "Pierces through and removes Spell Shield.")};

    descr.push_back(
        i18n::get(
            "spells.cancellation.descr_vulnerable_prefix",
            "Outer Beings, Undead or Summoned creatures also take ") +
        damage_for_vulnerable_creatures().str() +
        i18n::get(
            "spells.cancellation.descr_vulnerable_suffix",
            " damage."));

    descr.push_back(
        i18n::get(
            "spells.cancellation.descr_range_prefix",
            "The spell has a maximum range of ") +
        std::to_string(max_dist(skill)) +
        i18n::get(
            "spells.cancellation.descr_range_suffix",
            " steps, reaching through solid obstacles."));

    auto to_names = [](const std::vector<CancelledPropData>& entries) {
        std::vector<std::string> names;

        for (const CancelledPropData& data : entries) {
            if (data.include_in_descr == CancelledPropIncludeInDescr::yes) {
                names.push_back(prop::name(data.id));
            }
        }

        return names;
    };

    const std::vector<std::string> negative_effect_names =
        to_names(negative_effect_types_cancelled());

    std::vector<std::string> positive_effect_names =
        to_names(positive_effect_types_cancelled());

    descr.push_back(
        i18n::get(
            "spells.cancellation.descr_enemies_prefix",
            "Effects removed from enemies: All resistances, ") +
        text_format::make_comma_and_str(positive_effect_names) +
        i18n::get(
            "spells.cancellation.descr_enemies_suffix",
            "."));

    descr.push_back(
        i18n::get(
            "spells.cancellation.descr_allies_prefix",
            "From caster/allies: ") +
        text_format::make_comma_and_str(negative_effect_names) +
        i18n::get(
            "spells.cancellation.descr_allies_suffix",
            "."));

    return descr;
}

// -----------------------------------------------------------------------------
// Inscribe Boundary Sigil


// -----------------------------------------------------------------------------
// Inscribe Boundary Sigil
// -----------------------------------------------------------------------------
std::string SpellInscribeBoundarySigil::name() const
{
    return i18n::get("spells.boundary_sigil.name", "Inscribe Boundary Sigil");
}

SpellId SpellInscribeBoundarySigil::id() const
{
    return SpellId::inscribe_boundary_sigil;
}

SpellDomain SpellInscribeBoundarySigil::domain() const
{
    return SpellDomain::warding;
}

bool SpellInscribeBoundarySigil::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

SpellShock SpellInscribeBoundarySigil::shock_type() const
{
    return SpellShock::disturbing;
}

Range SpellInscribeBoundarySigil::nr_actions_prevented(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {3, 9};
    case SpellSkill::expert:       return {3, 12};
    case SpellSkill::master:       return {3, 15};
    case SpellSkill::transcendent: return {6, 20};
    }

    ASSERT(false);
    return {1, 1};
}

int SpellInscribeBoundarySigil::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 5;
}

void SpellInscribeBoundarySigil::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    // TODO: What does it look like when casting from an unknown manuscript (when a symbol can
    // be inscribed and when it cannot)?

    // TODO: There should be a casting sound.

    const terrain::Terrain* terrain_here = map::g_terrain.at(caster->m_pos);

    terrain::Id terrain_id_here = terrain_here->id();

    if (terrain_id_here != terrain::Id::floor && terrain_id_here != terrain::Id::trap) {
        if (map::g_player->m_properties.allow_see()) {
            msg_log::add(i18n::get(
                "spells.symbol_fails_to_bind",
                "A symbol flickers briefly, but fails to bind here."));
        }
        else {
            // NOTE: Assuming that the player is casting an already known spell (not
            // possible to cast from Manuscripts while blind).
            msg_log::add(i18n::get(
                "spells.sense_sigil_failed_to_bind",
                "I sense that the sigil failed to bind here."));
        }

        return;
    }

    auto* const trap =
        static_cast<terrain::Trap*>(
            terrain::make(terrain::Id::trap, caster->m_pos));

    // Set up mimic terrain.

    // If the terrain here is another trap (a sigil), then use its mimic terrain as a base.
    if (terrain_id_here == terrain::Id::trap) {
        terrain_here =
            static_cast<const terrain::Trap*>(terrain_here)
                ->get_mimic_terrain();

        terrain_id_here = terrain_here->id();
    }

    terrain::Terrain* const mimic = terrain::make(terrain_id_here, caster->m_pos);

    if (terrain_id_here == terrain::Id::floor) {
        // The terrain to mimic is a floor, set correct floor type.
        static_cast<terrain::Floor*>(mimic)->m_type =
            static_cast<const terrain::Floor*>(terrain_here)->m_type;
    }

    trap->set_mimic_terrain(mimic);

    const bool is_trap_ok = trap->try_init_type(terrain::TrapId::boundary);

    if (!is_trap_ok) {
        // There shouldn't be any reason for this to happen.
        ASSERT(false);

        delete trap;

        return;
    }

    auto* const boundary = static_cast<terrain::TrapBoundary*>(trap->trap_impl());

    boundary->set_nr_actions_to_prevent(nr_actions_prevented(skill).roll());

    map::update_terrain(trap);

    trap->reveal(terrain::PrintRevealMsg::no);
}

std::vector<std::string> SpellInscribeBoundarySigil::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.boundary_sigil.descr_main",
            "Inscribes a magical sigil upon the ground, "
            "preventing Outer Beings, Undead and Summoned creatures "
            "from entering it or making melee attacks across its boundary."));

    descr.emplace_back(
        i18n::get(
            "spells.boundary_sigil.descr_actions_prefix",
            "The sigil can prevent ") +
        nr_actions_prevented(skill).str() +
        i18n::get(
            "spells.boundary_sigil.descr_actions_suffix",
            " actions before it fades, "
            "though it also has a small chance to fade each turn."));

    descr.emplace_back(
        i18n::get(
            "spells.boundary_sigil.descr_floor_only",
            "Can only be inscribed on floor, but may overwrite an existing sigil."));

    return descr;
}

// -----------------------------------------------------------------------------
// Light


// -----------------------------------------------------------------------------
// Light
// -----------------------------------------------------------------------------
std::string SpellLight::name() const
{
    return i18n::get("spells.light.name", "Light");
}

SpellId SpellLight::id() const
{
    return SpellId::light;
}

SpellDomain SpellLight::domain() const
{
    return SpellDomain::warding;
}

SpellShock SpellLight::shock_type() const
{
    return SpellShock::mild;
}

int SpellLight::base_max_cost(
    SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 5;
}

bool SpellLight::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return false;
}

Range SpellLight::light_duration_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:        return {10, 20};
    case SpellSkill::expert:       return {15, 30};
    case SpellSkill::master:
    case SpellSkill::transcendent: return {20, 40};
    }

    ASSERT(false);

    return {1, 1};
}

Range SpellLight::blind_duration_range(const SpellSkill skill) const
{
    switch (skill) {
    case SpellSkill::basic:
    case SpellSkill::expert:
        // Not expected, should not cause blinding at these levels.
        break;

    case SpellSkill::master:       return {1, 3};
    case SpellSkill::transcendent: return {3, 5};
    }

    ASSERT(false);

    return {1, 1};
}

Range SpellLight::burning_duration_range() const
{
    return {3, 6};
}

void SpellLight::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    prop::Prop* radiant = prop::make(prop::Id::radiant_fov);

    radiant->set_duration(light_duration_range(skill).roll());

    caster->m_properties.apply(radiant);

    std::vector<prop::Prop*> properties;

    if (skill >= SpellSkill::master) {
        prop::Prop* const prop = prop::make(prop::Id::blind);

        prop->set_duration(blind_duration_range(skill).roll());

        properties.push_back(prop);
    }

    if (skill == SpellSkill::transcendent) {
        prop::Prop* const prop = prop::make(prop::Id::burning);

        prop->set_duration(burning_duration_range().roll());

        properties.push_back(prop);
    }

    if (!properties.empty()) {
        explosion::run(
            caster->m_pos,
            ExplType::apply_prop,
            EmitExplSnd::no,
            -1,
            ExplExclCenter::yes,
            properties,
            colors::yellow());
    }
}

std::vector<std::string> SpellLight::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.light.descr_main",
            "Illuminates the area around the caster."));

    descr.push_back(spell_duration_descr(light_duration_range(skill).str()));

    if (skill >= SpellSkill::master) {
        descr.push_back(
            i18n::get(
                "spells.light.descr_blind_prefix",
                "On casting, causes a blinding flash centered on the "
                "caster (but not affecting the caster itself). "
                "The blinding effect lasts ") +
            blind_duration_range(skill).str() +
            i18n::get(
                "spells.light.descr_blind_suffix",
                " turns."));
    }

    if (skill == SpellSkill::transcendent) {
        descr.push_back(
            i18n::get(
                "spells.light.descr_burn_prefix",
                "The flash is so intense that any victim caught in it "
                "will also burn for ") +
            burning_duration_range().str() +
            i18n::get(
                "spells.light.descr_burn_suffix",
                " turns."));
    }

    return descr;
}

// -----------------------------------------------------------------------------
// Invisibility


// -----------------------------------------------------------------------------
// Spell Shield
// -----------------------------------------------------------------------------
int SpellSpellShield::mon_cooldown() const
{
    return 3;
}

std::string SpellSpellShield::name() const
{
    return i18n::get("spells.spell_shield.name", "Spell Shield");
}

SpellId SpellSpellShield::id() const
{
    return SpellId::spell_shield;
}

SpellDomain SpellSpellShield::domain() const
{
    return SpellDomain::warding;
}

SpellShock SpellSpellShield::shock_type() const
{
    return SpellShock::mild;
}

bool SpellSpellShield::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellSpellShield::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)caster;

    return 5 - (int)skill;
}

void SpellSpellShield::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)skill;
    (void)seen_targets;
    (void)player_aware;

    // TODO: What does it look like when casting from Manuscript and permanent spell shield is
    // already applied?

    prop::Prop* prop = prop::make(prop::Id::r_spell);

    prop->set_indefinite();

    caster->m_properties.apply(prop);
}

std::vector<std::string> SpellSpellShield::descr_specific(
    const SpellSkill skill) const
{
    (void)skill;

    std::vector<std::string> descr;

    descr.emplace_back(
        i18n::get(
            "spells.spell_shield.descr",
            "Grants protection against harmful spells. The effect lasts "
            "until a spell is blocked."));

    return descr;
}

bool SpellSpellShield::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)seen_targets;

    return !mon.m_properties.has(prop::Id::r_spell);
}

// -----------------------------------------------------------------------------
// Haste


// -----------------------------------------------------------------------------
// Heal
// -----------------------------------------------------------------------------
int SpellHeal::mon_cooldown() const
{
    return 6;
}

std::string SpellHeal::name() const
{
    return i18n::get("spells.healing.name", "Healing");
}

SpellId SpellHeal::id() const
{
    return SpellId::heal;
}

SpellDomain SpellHeal::domain() const
{
    return SpellDomain::warding;
}

SpellShock SpellHeal::shock_type() const
{
    return SpellShock::mild;
}

bool SpellHeal::is_noisy(const SpellSkill skill) const
{
    (void)skill;

    return true;
}

int SpellHeal::nr_hp_restored(SpellSkill skill) const
{
    return 8 + (int)skill * 4;
}

Range SpellHeal::regen_duration() const
{
    return {50, 100};
}

int SpellHeal::base_max_cost(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    (void)skill;
    (void)caster;

    return 6;
}

void SpellHeal::run_effect(
    actor::Actor* const caster,
    const SpellSkill skill,
    const std::vector<actor::Actor*>& seen_targets,
    PlayerAwareOfCast player_aware) const
{
    (void)seen_targets;
    (void)player_aware;

    // TODO: What does it look like when casting from manuscript and already at full HP and
    // there is nothing to cure?

    if ((int)skill >= (int)SpellSkill::expert) {
        caster->m_properties.end_prop(prop::Id::weakened);
        caster->m_properties.end_prop(prop::Id::poisoned);
    }

    if (skill >= SpellSkill::master) {
        caster->m_properties.end_prop(prop::Id::infected);
        caster->m_properties.end_prop(prop::Id::diseased);
        caster->m_properties.end_prop(prop::Id::blind);
        caster->m_properties.end_prop(prop::Id::deaf);
    }

    if (skill == SpellSkill::transcendent) {
        if (actor::is_player(caster)) {
            prop::Prop* const wound_prop =
                map::g_player->m_properties.prop(prop::Id::wound);

            if (wound_prop) {
                static_cast<prop::Wound*>(wound_prop)->heal_one_wound();
            }
        }

        auto* const prop = prop::make(prop::Id::regenerating);

        prop->set_duration(regen_duration().roll());

        caster->m_properties.apply(prop);
    }

    actor::restore_hp(*caster, nr_hp_restored(skill));
}

bool SpellHeal::allow_mon_cast_now(
    const actor::Actor& mon,
    const std::vector<actor::Actor*>& seen_targets) const
{
    (void)seen_targets;

    return mon.m_hp < actor::max_hp(mon);
}

std::vector<std::string> SpellHeal::descr_specific(
    const SpellSkill skill) const
{
    std::vector<std::string> descr;

    descr.push_back(
        i18n::get(
            "spells.healing.restore_prefix",
            "Restores ") +
        std::to_string(nr_hp_restored(skill)) +
        i18n::get(
            "spells.healing.restore_suffix",
            " hit points."));

    if (skill == SpellSkill::expert) {
        descr.emplace_back(
            i18n::get(
                "spells.healing.cures_basic",
                "Cures weakening and poisoning."));
    }
    else if (skill >= SpellSkill::master) {
        descr.emplace_back(
            i18n::get(
                "spells.healing.cures_master",
                "Cures weakening, poisoning, infections, disease, blindness and deafness."));
    }

    if (skill == SpellSkill::transcendent) {
        descr.emplace_back(
            i18n::get(
                "spells.healing.heals_wound",
                "Heals one wound."));

        descr.emplace_back(
            i18n::get(
                "spells.healing.regen_prefix",
                "+1 hit point regenerated per turn, for ") +
            regen_duration().str() +
            i18n::get(
                "spells.healing.regen_suffix",
                " turns."));
    }

    return descr;
}

// -----------------------------------------------------------------------------
// Mi-Go hypnosis

