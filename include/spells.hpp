// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SPELLS_HPP
#define SPELLS_HPP

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "global.hpp"
#include "random.hpp"

class Spell;
struct P;

namespace prop
{
enum class Id;
};  // namespace prop

namespace actor
{
class Actor;
}  // namespace actor

namespace audio
{
enum class SfxId;
};  // namespace audio

namespace terrain
{
enum class DidOpen;
enum class DidClose;
}  // namespace terrain

namespace item
{
class Item;
}  // namespace item

enum class SpellId
{
    //
    // --- AVAILABLE FOR THE PLAYER (and possibly monsters)
    //

    // Domain: Channeling
    aza_gaze,
    cataclysm,
    darkbolt,
    gnawing_torrent,

    // Domain: Corruption
    aura_of_decay,
    curse,
    enfeeble,
    pestilence,
    poison,

    // Domain: Illusion
    invis,
    // NOTE: The Mirror Images spell is NOT supported for monsters, because the player could just
    // view the monster descriptions and see which ones are unusually hard to hit (the mirror images
    // have extremely high dodge), which would just be annoying and ruins the whole aspect of making
    // the images look like the caster.
    //
    // (A similar thing happens with hallucination, but in that case it's a different situation and
    // more OK.)
    //
    mirror_images,
    terrify,
    threat_projection,

    // Domain: Mind
    clairvoyance,
    control_object,
    erudition,
    identify,
    premonition,
    projected_strike,
    see_invis,
    transmut,

    // Domain: Time
    expulsion,
    haste,
    slow,
    teleport,
    temporal_echo,

    // Domain: Warding
    bless,
    cancellation,
    heal,
    inscribe_boundary_sigil,
    light,
    spell_shield,

    // Domain: Blood
    blood_tempering,
    crimson_passage,
    sacrifice_life,
    thorns,

    //
    // --- EXORCIST BACKGROUND ONLY ---
    //

    // (No domain)
    cleansing_fire,
    purge,
    sanctuary,

    //
    // --- GHOUL BACKGROUND ONLY ---
    //

    // (No domain)
    frenzy,

    //
    // --- FLAGELLANT BACKGROUND ONLY ---
    //

    // Domain: Blood
    shed_impurity,

    //
    // --- MONSTERS ONLY ---
    //

    // (Domain doesn't matter)
    blind,
    burn,
    deafen,
    disease,
    force_bolt,
    heal_others,
    knockback,
    mi_go_hypno,
    summon_random,
    summon_tentacles,
    summon_water_creature,

    END
};

enum class SpellDomain
{
    channeling,
    corruption,
    illusion,
    mind,
    time,
    warding,

    blood,

    END
};

enum class SpellSkill
{
    basic,
    expert,
    master,
    transcendent
};

enum class SpellSrc
{
    learned,
    manuscript,
    item
};

enum class SpellShock
{
    none,
    mild,
    disturbing,
    severe
};

// Player saw or heard the spell being cast.
enum class PlayerAwareOfCast
{
    no,
    yes,
};

// Does the spell cost spirit or hit points to cast?
enum class SpellCostType
{
    spirit,
    hit_points
};

namespace spells
{
Spell* make(SpellId spell_id);

SpellId str_to_spell_id(const std::string& str);

std::string spell_domain_title(SpellDomain domain);

SpellSkill str_to_spell_skill_id(const std::string& str);

std::string skill_to_str(SpellSkill skill);

ShockSrc spell_domain_to_shock_type(SpellDomain domain);

terrain::DidOpen run_opening_spell_effect_at(const P& pos, SpellSkill skill);

terrain::DidClose run_close_spell_effect_at(const P& pos, SpellSkill skill);

void run_mi_go_hypno_effect(actor::Actor& target);

}  // namespace spells

class Spell
{
public:
    Spell() = default;

    virtual ~Spell() = default;

    void cast(
        actor::Actor* caster,
        SpellSkill skill,
        SpellSrc spell_src,
        const std::vector<actor::Actor*>& seen_targets) const;

    virtual void run_effect(
        actor::Actor* caster,
        SpellSkill skill,
        const std::vector<actor::Actor*>& seen_targets,
        PlayerAwareOfCast player_aware) const = 0;

    virtual bool allow_mon_cast_now(
        const actor::Actor& mon,
        const std::vector<actor::Actor*>& seen_targets) const
    {
        (void)mon;
        (void)seen_targets;

        return false;
    }

    virtual int mon_cooldown() const
    {
        return 3;
    }

    virtual std::string name() const = 0;

    virtual SpellId id() const = 0;

    virtual SpellDomain domain() const = 0;

    // Casting a memorized tenebrous spell disables it (i.e. single use, until it it re-enabled).
    virtual bool is_tenebrous() const
    {
        return false;
    }

    virtual bool can_be_improved_with_skill() const
    {
        return true;
    }

    std::vector<std::string> descr(SpellSkill skill, SpellSrc spell_src) const;

    std::string domain_descr() const;

    virtual std::vector<std::string> descr_specific(SpellSkill skill) const = 0;

    Range cost_range(SpellSkill skill, const actor::Actor* caster = nullptr) const;

    virtual SpellCostType cost_type() const
    {
        return SpellCostType::spirit;
    }

    int shock_value() const;

    virtual SpellShock shock_type() const = 0;

protected:
    virtual int base_max_cost(SpellSkill skill, const actor::Actor* caster) const = 0;

    virtual bool is_noisy(SpellSkill skill) const = 0;

    void on_resist(actor::Actor& target) const;

    bool m_is_disabled {false};
};

#include "spells/background.hpp"
#include "spells/blood.hpp"
#include "spells/channeling.hpp"
#include "spells/corruption.hpp"
#include "spells/illusion.hpp"
#include "spells/mind.hpp"
#include "spells/monster.hpp"
#include "spells/time.hpp"
#include "spells/warding.hpp"

#endif  // SPELLS_HPP
