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
// Private
// -----------------------------------------------------------------------------
static const std::unordered_map<std::string, SpellId> s_str_to_spell_id_map = {
    {"SPELL_AURA_OF_DECAY", SpellId::aura_of_decay},
    {"SPELL_AZA_GAZE", SpellId::aza_gaze},
    {"SPELL_BLESS", SpellId::bless},
    {"SPELL_BLIND", SpellId::blind},
    {"SPELL_BURN", SpellId::burn},
    {"SPELL_CANCELLATION", SpellId::cancellation},
    {"SPELL_CATACLYSM", SpellId::cataclysm},
    {"SPELL_CLAIRVOYANCE", SpellId::clairvoyance},
    {"SPELL_CLEANSING_FIRE", SpellId::cleansing_fire},
    {"SPELL_CONTROL_OBJECT", SpellId::control_object},
    {"SPELL_CURSE", SpellId::curse},
    {"SPELL_DARKBOLT", SpellId::darkbolt},
    {"SPELL_DEAFEN", SpellId::deafen},
    {"SPELL_DISEASE", SpellId::disease},
    {"SPELL_ENFEEBLE", SpellId::enfeeble},
    {"SPELL_ERUDITION", SpellId::erudition},
    {"SPELL_EXPULSION", SpellId::expulsion},
    {"SPELL_FORCE_BOLT", SpellId::force_bolt},
    {"SPELL_FRENZY", SpellId::frenzy},
    {"SPELL_GNAWING_TORRENT", SpellId::gnawing_torrent},
    {"SPELL_HASTE", SpellId::haste},
    {"SPELL_HEAL", SpellId::heal},
    {"SPELL_HEAL_OTHERS", SpellId::heal_others},
    {"SPELL_IDENTIFY", SpellId::identify},
    {"SPELL_KNOCKBACK", SpellId::knockback},
    {"SPELL_LIGHT", SpellId::light},
    {"SPELL_MIRROR_IMAGE", SpellId::mirror_images},
    {"SPELL_MI_GO_HYPNO", SpellId::mi_go_hypno},
    {"SPELL_PESTILENCE", SpellId::pestilence},
    {"SPELL_POISON", SpellId::poison},
    {"SPELL_PREMONITION", SpellId::premonition},
    {"SPELL_PURGE", SpellId::purge},
    {"SPELL_SANCTUARY", SpellId::sanctuary},
    {"SPELL_SEE_INVIS", SpellId::see_invis},
    {"SPELL_SLOW", SpellId::slow},
    {"SPELL_PROJECTED_STRIKE", SpellId::projected_strike},
    {"SPELL_SPELL_SHIELD", SpellId::spell_shield},
    {"SPELL_SUMMON_RANDOM", SpellId::summon_random},
    {"SPELL_SUMMON_TENTACLES", SpellId::summon_tentacles},
    {"SPELL_SUMMON_WATER_CREATURE", SpellId::summon_water_creature},
    {"SPELL_TELEPORT", SpellId::teleport},
    {"SPELL_TEMPORAL_ECHO", SpellId::temporal_echo},
    {"SPELL_TERRIFY", SpellId::terrify},
    {"SPELL_THREAT_PROJECTION", SpellId::threat_projection},
    {"SPELL_TRANSMUT", SpellId::transmut},
};

static const std::unordered_map<std::string, SpellSkill> s_str_to_spell_skill_map = {
    {"SPELLSKILL_BASIC", SpellSkill::basic},
    {"SPELLSKILL_EXPERT", SpellSkill::expert},
    {"SPELLSKILL_MASTER", SpellSkill::master},
    {"SPELLSKILL_TRANSCENDENT", SpellSkill::transcendent},
};

static const std::unordered_map<SpellDomain, ShockSrc> s_spell_domain_to_shock_type_map = {
    {SpellDomain::blood, ShockSrc::cast_intr_spell_blood},
    {SpellDomain::channeling, ShockSrc::cast_intr_spell_channeling},
    {SpellDomain::corruption, ShockSrc::cast_intr_spell_corruption},
    {SpellDomain::illusion, ShockSrc::cast_intr_spell_illusion},
    {SpellDomain::mind, ShockSrc::cast_intr_spell_mind},
    {SpellDomain::time, ShockSrc::cast_intr_spell_time},
    {SpellDomain::warding, ShockSrc::cast_intr_spell_warding},
    // NOTE: Not all spells belong to a domain:
    {SpellDomain::END, ShockSrc::cast_intr_spell_general},
};

static std::string spell_resist_msg_player()
{
    return i18n::get("spells.resist_player", "I resist the spell!");
}

// This assumes the message starts with "Monster Name":
static std::string spell_resist_msg_mon_suffix()
{
    return i18n::get("spells.resists_suffix", " resists the spell!");
}

std::string spell_reflect_msg()
{
    return i18n::get("spells.reflected", "The spell is reflected!");
}

std::string not_alerting_mon_descr()
{
    return i18n::get(
        "spells.not_alerting_mon_descr",
        "Casting this spell does not alert the victim to the caster's presence.");
}

std::string spell_duration_descr(const std::string& duration)
{
    return
        i18n::get("spells.duration_prefix", "The spell lasts ") +
        duration +
        i18n::get("spells.duration_suffix", " turns.");
}

std::string spell_indefinite_duration_descr()
{
    return i18n::get(
        "spells.duration_indefinite",
        "The spell lasts indefinitely.");
}


namespace spell_side_effects
{
struct Context
{
    Context(actor::Actor& spell_caster, const std::vector<P>& caster_nearby_positions) :
        caster(spell_caster),
        nearby_positions(caster_nearby_positions) {}

    actor::Actor& caster;
    const std::vector<P>& nearby_positions;
};

static void print_side_effect_trigger_message()
{
    msg_log::add(i18n::get(
        "spells.unexpected_effect",
        "An unexpected effect was induced by the spell."));
}

static void side_effect_spawn_monsters(const Context& context)
{
    TRACE_FUNC_BEGIN;

    const P p = rnd::element(context.nearby_positions);

    const std::string id = "MON_TENTACLE_CLUSTER";

    actor::MonSpawnResult spawned = actor::spawn(p, {id});

    bool printed_msg = false;

    for (auto* const actor : spawned.monsters) {
        if (!printed_msg) {
            print_side_effect_trigger_message();
            printed_msg = true;
        }

        prop::Prop* const conflicted = prop::make(prop::Id::conflict);

        conflicted->set_indefinite();

        actor->m_properties.apply(
            conflicted,
            prop::PropSrc::intr,
            false,
            Verbose::no);

        prop::Prop* const waiting = prop::make(prop::Id::waiting);

        waiting->set_duration(2);

        actor->m_properties.apply(waiting);

        prop::Prop* const summoned = prop::make(prop::Id::summoned);

        summoned->set_duration(rnd::range(3, 20));

        actor->m_properties.apply(summoned);
    }

    map::update_vision();
    actor::make_player_aware_seen_monsters();

    TRACE_FUNC_END;
}

static void side_effect_swap_wall_floor(const Context& context)
{
    TRACE_FUNC_BEGIN;

    print_side_effect_trigger_message();

    Array2<bool> blocked(map::dims());

    map_parsers::BlocksWalking(ParseActors::no)
        .run(blocked, blocked.rect());

    const std::vector<terrain::Id> free_terrains = {
        terrain::Id::door,
    };

    for (const P& p : blocked.rect().positions()) {
        const bool is_free_terrain =
            map_parsers::IsAnyOfTerrains(free_terrains)
                .run(p);

        if (is_free_terrain) {
            blocked.at(p) = false;
        }
    }

    Array2<bool> has_actor(map::dims());

    for (auto* actor : game_time::g_actors) {
        if (actor->m_state != ActorState::destroyed) {
            has_actor.at(actor->m_pos) = true;
        }
    }

    for (const auto& p : context.nearby_positions) {
        if (!map::is_pos_inside_outer_walls(p) ||
            has_actor.at(p) ||
            map::g_items.at(p) ||
            !rnd::one_in(14)) {
            continue;
        }

        const auto terrain_id = map::g_terrain.at(p)->id();

        if (terrain_id == terrain::Id::wall) {
            blocked.at(p) = false;

            if (map_parsers::is_map_connected(blocked)) {
                map::update_terrain(terrain::make(terrain::Id::floor, p));
            }
            else {
                // Map would not be connected
                blocked.at(p) = true;
            }
        }
        else if (terrain_id == terrain::Id::floor) {
            blocked.at(p) = true;

            if (map_parsers::is_map_connected(blocked)) {
                map::update_terrain(terrain::make(terrain::Id::wall, p));
            }
            else {
                // Map would not be connected
                blocked.at(p) = false;
            }
        }
    }

    TRACE_FUNC_END;
}  // swap_wall_floor

static void side_effect_ignite_terrain(const Context& context)
{
    TRACE_FUNC_BEGIN;

    Array2<bool> has_actor(map::dims());

    for (auto* actor : game_time::g_actors) {
        if (actor->m_state != ActorState::destroyed) {
            has_actor.at(actor->m_pos) = true;
        }
    }

    bool printed_msg = false;
    for (const auto& p : context.nearby_positions) {
        if (has_actor.at(p)) {
            continue;
        }

        if (!rnd::one_in(14)) {
            continue;
        }

        if (!printed_msg) {
            print_side_effect_trigger_message();
            printed_msg = true;
        }

        terrain::Terrain* const terrain = map::g_terrain.at(p);

        terrain->hit(DmgType::fire, nullptr);
    }

    TRACE_FUNC_END;
}

static void side_effect_open_close_doors(const Context& context)
{
    TRACE_FUNC_BEGIN;

    // Open or close doors
    const bool should_open = (bool)rnd::coin_toss();

    Array2<bool> has_actor(map::dims());

    for (actor::Actor* actor : game_time::g_actors) {
        if (actor->m_state != ActorState::destroyed) {
            has_actor.at(actor->m_pos) = true;
        }
    }

    bool printed_msg = false;
    for (const P& pos : context.nearby_positions) {
        if (has_actor.at(pos) || map::g_items.at(pos)) {
            continue;
        }

        terrain::Terrain* const terrain = map::g_terrain.at(pos);

        if (terrain->id() != terrain::Id::door) {
            continue;
        }

        if (static_cast<terrain::Door*>(terrain)->is_warded()) {
            continue;
        }

        if (!printed_msg) {
            print_side_effect_trigger_message();
            printed_msg = true;
        }

        // NOTE: Warded doors are skipped, so it's OK to just run
        // open/close here.
        if (should_open) {
            terrain->open(nullptr);
        }
        else {
            terrain->close(nullptr);
        }
    }

    TRACE_FUNC_END;
}

static void side_effect_flay_human(const Context& context)
{
    TRACE_FUNC_BEGIN;

    std::vector<actor::Actor*> actors = actor::seen_actors(context.caster);

    actors.push_back(&context.caster);

    rnd::shuffle(actors);

    actor::Actor* target_actor = nullptr;

    for (actor::Actor* const actor : actors) {
        const actor::ActorData* const actor_data = actor->m_data;

        const prop::PropHandler& properties = actor->m_properties;

        // NOTE: The target of the spell side effect may be the caster itself, if caster is
        // a monster.
        if (!actor::is_player(actor) &&
            actor::is_alive(*actor) &&
            actor_data->is_humanoid &&
            actor_data->can_leave_corpse &&
            (actor_data->mon_shock_lvl <= MonShockLvl::frightening) &&
            !actor->m_properties.has(prop::Id::undead) &&
            !actor_data->is_unique &&
            !properties.has(prop::Id::ethereal) &&
            !properties.has(prop::Id::possessed_by_zuul) &&
            !properties.has(prop::Id::spawns_zombie_parts_on_destroyed)) {
            target_actor = actor;

            break;
        }
    }

    if (!target_actor) {
        return;
    }

    print_side_effect_trigger_message();

    if (actor::can_player_see_actor(*target_actor)) {
        const std::string name =
            text_format::first_to_upper(
                actor::name_the(*target_actor));

        msg_log::add(
            name +
            i18n::get(
                "spells.suddenly_flayed_alive_suffix",
                " is suddenly flayed alive!"));
    }

    actor::kill(*target_actor, IsDestroyed::yes, AllowGore::yes, AllowDropItems::yes);

    actor::spawn(target_actor->m_pos, {"MON_INTESTINAL_MASS"});

    TRACE_FUNC_END;
}

static void side_effect_create_water(const Context& context)
{
    TRACE_FUNC_BEGIN;

    bool printed_msg = false;

    for (const auto& p : context.nearby_positions) {
        if ((map::g_terrain.at(p)->id() != terrain::Id::floor) ||
            !rnd::one_in(8)) {
            continue;
        }

        if (!printed_msg) {
            print_side_effect_trigger_message();
            printed_msg = true;
        }

        auto* const liquid =
            static_cast<terrain::Liquid*>(
                terrain::make(
                    terrain::Id::liquid,
                    p));

        liquid->m_type = LiquidType::water;

        map::update_terrain(liquid);
    }

    TRACE_FUNC_END;
}

static void side_effect_alter_env(const Context& context)
{
    TRACE_FUNC_BEGIN;

    print_side_effect_trigger_message();

    prop::run_alter_env_effect(context.caster.m_pos);

    TRACE_FUNC_END;
}

static void side_effect_create_doors(const Context& context)
{
    TRACE_FUNC_BEGIN;

    const auto adj_door_checker = map_parsers::AnyAdjIsAnyOfTerrains(terrain::Id::door);
    const auto adj_floor_checker = map_parsers::AnyAdjIsAnyOfTerrains(terrain::Id::floor);

    bool printed_msg = false;
    for (const auto& p : context.nearby_positions) {
        const auto id = map::g_terrain.at(p)->id();

        if (!rnd::one_in(2) ||
            (id != terrain::Id::wall) ||
            adj_door_checker.run(p) ||
            !adj_floor_checker.run(p)) {
            continue;
        }

        if (!printed_msg) {
            print_side_effect_trigger_message();
            printed_msg = true;
        }

        auto* const mimic = terrain::make(terrain::Id::wall, p);

        auto* const door =
            static_cast<terrain::Door*>(
                terrain::make(
                    terrain::Id::door,
                    p));

        door->set_mimic_terrain(mimic);

        door->init_type_and_state(
            terrain::DoorType::wood,
            terrain::DoorSpawnState::closed);

        map::update_terrain(door);
    }

    TRACE_FUNC_END;
}

static void side_effect_create_dark_void(const Context& context)
{
    TRACE_FUNC_BEGIN;

    std::vector<P> sorted_positions = context.nearby_positions;

    std::sort(
        std::begin(sorted_positions),
        std::end(sorted_positions),
        [context](const auto& p1, const auto& p2) {
            const auto caster_p = context.caster.m_pos;

            const int d1 = king_dist(p1, caster_p);
            const int d2 = king_dist(p2, caster_p);

            return d1 < d2;
        });

    Array2<bool> blocked(map::dims());

    map_parsers::BlocksWalking(ParseActors::no)
        .run(blocked, blocked.rect());

    const std::vector<terrain::Id> free_terrains = {
        terrain::Id::door,
    };

    for (const P& p : blocked.rect().positions()) {
        const bool is_free_terrain = map_parsers::IsAnyOfTerrains(free_terrains).run(p);

        if (is_free_terrain) {
            blocked.at(p) = false;
        }
    }

    print_side_effect_trigger_message();

    for (const auto& p : sorted_positions) {
        if (!map::is_pos_inside_outer_walls(p)) {
            continue;
        }

        map::g_dark.at(p) = true;
        map::g_light.at(p) = false;

        if (map::g_terrain.at(p)->id() == terrain::Id::wall) {
            blocked.at(p) = false;

            if (map_parsers::is_map_connected(blocked)) {
                map::update_terrain(
                    terrain::make(terrain::Id::floor, p));
            }
            else {
                blocked.at(p) = true;
            }
        }
    }

    TRACE_FUNC_END;
}

static void side_effect_push_statue(const Context& context)
{
    TRACE_FUNC_BEGIN;

    for (const auto& p : context.nearby_positions) {
        auto* const terrain = map::g_terrain.at(p);

        if (terrain->id() != terrain::Id::statue) {
            continue;
        }

        print_side_effect_trigger_message();

        auto* const statue = static_cast<terrain::Statue*>(terrain);

        const auto direction = dir_utils::dir(rnd::element(dir_utils::g_dir_list));

        statue->topple(direction);

        break;
    }

    TRACE_FUNC_END;
}

using SpellSideEffect = std::function<void(const Context&)>;

WeightedItems<SpellSideEffect> s_spell_side_effects {
    {
        side_effect_create_dark_void,
        side_effect_create_doors,
        side_effect_alter_env,
        side_effect_create_water,
        side_effect_flay_human,
        side_effect_ignite_terrain,
        side_effect_open_close_doors,
        side_effect_push_statue,
        side_effect_spawn_monsters,
        side_effect_swap_wall_floor,
    },
    {
        10,  // create_dark_void
        15,  // create_doors
        30,  // create_trees
        30,  // create_water
        50,  // flay_human
        50,  // ignite_terrain
        90,  // open_close_doors
        90,  // push_statue
        50,  // spawn_monsters
        70,  // swap_wall_floor
    }};

static void run_random_side_effect(actor::Actor& caster)
{
    // Run a random side effect.
    const int d = 3;

    const R rect(
        {std::max(0, caster.m_pos.x - d),
         std::max(0, caster.m_pos.y - d)},
        {std::min(map::w() - 1, caster.m_pos.x + d),
         std::min(map::h() - 1, caster.m_pos.y + d)});

    std::vector<P> nearby_positions = rect.positions();

    rnd::shuffle(nearby_positions);

    const SpellSideEffect& side_effect_function = s_spell_side_effects.roll();

    TRACE << "Running spell side effect" << "\n";

    side_effect_function({caster, nearby_positions});
}

}  // namespace spell_side_effects

static std::string get_noise_descr(const bool is_noisy)
{
    std::string str =
        is_noisy
        ? i18n::get(
              "spells.cast_requires_sounds",
              "Casting this spell requires making sounds.")
        : i18n::get("spells.cast_silently", "The spell can be cast silently.");

    return str;
}

static std::string get_skill_descr(
    const SpellSkill skill,
    const SpellSrc source)
{
    std::string str =
        i18n::get("spells.skill_descr_prefix", "The spell can be cast at ") +
        spells::skill_to_str(skill) +
        i18n::get("spells.skill_descr_suffix", " level");

    std::vector<std::string> bon_words;

    const prop::PropHandler& properties = map::g_player->m_properties;

    if (source == SpellSrc::manuscript) {
        bon_words.emplace_back(
            i18n::get("spells.skill_bonus.manuscript", "manuscript"));
    }

    if (player_spells::is_getting_altar_bonus()) {
        bon_words.emplace_back(
            i18n::get("spells.skill_bonus.altar", "altar"));
    }

    if (properties.has(prop::Id::erudition)) {
        bon_words.emplace_back(
            i18n::get("spells.skill_bonus.erudition", "erudition"));
    }

    if (map::g_player->m_inv.has_item_in_backpack(item::Id::necronomicon)) {
        bon_words.emplace_back(
            i18n::get("spells.skill_bonus.necronomicon", "necronomicon"));
    }

    for (size_t i = 0; i < bon_words.size(); ++i) {
        if (i == 0) {
            str += " (";
        }

        str += bon_words[i];

        if (i < (bon_words.size() - 1)) {
            str += ", ";
        }
        else {
            str += ")";
        }
    }

    str += i18n::get("spells.period", ".");

    return str;
}

static void end_properties_for_casting_spell(
    actor::Actor& caster,
    const SpellId spell_id)
{
    // End cloaking (unless invisibility was cast now).
    if (spell_id != SpellId::invis) {
        caster.m_properties.end_prop(prop::Id::cloaked);
    }

    // End focused
    caster.m_properties.end_prop(prop::Id::meditative_focused);

    // End erudition (unless that was the spell that was cast now).
    if (spell_id != SpellId::erudition) {
        const auto* const prop =
            caster.m_properties.prop(prop::Id::erudition);

        const bool should_end =
            prop &&
            static_cast<const prop::Erudition*>(prop)->should_end_on_spell_cast();

        if (should_end) {
            caster.m_properties.end_prop(prop::Id::erudition);
        }
    }
}

static std::string generate_mon_cast_sound_msg(const actor::Actor& caster)
{
    std::string spell_msg = actor::localized_text(
        caster.m_data->spell_msg_sound_i18n_key,
        caster.m_data->spell_msg_sound);

    if (spell_msg.empty()) {
        return "";
    }

    const bool is_mon_seen = actor::can_player_see_actor(caster);

    const std::string mon_name =
        is_mon_seen
        ? text_format::first_to_upper(actor::name_the(caster))
        : (caster.m_data->is_humanoid
               ? i18n::get("spells.someone", "Someone")
               : i18n::get("spells.something", "Something"));

    spell_msg = mon_name + " " + spell_msg;

    return spell_msg;
}

static std::string generate_mon_cast_visual_msg(const actor::Actor& caster)
{
    // NOTE: This assumes that the monster is seen.

    std::string spell_msg = actor::localized_text(
        caster.m_data->spell_msg_visual_i18n_key,
        caster.m_data->spell_msg_visual);

    if (spell_msg.empty()) {
        return "";
    }

    const std::string mon_name = text_format::first_to_upper(actor::name_the(caster));

    spell_msg = mon_name + " " + spell_msg;

    return spell_msg;
}

static int absorb_sp_cost_with_exorcist_fervor(int sp_cost)
{
    const int missing_sp = (sp_cost - map::g_player->m_sp) + 1;

    if (missing_sp > 0) {
        const int cost_reduction =
            std::min(
                missing_sp,
                actor::player_state::g_exorcist_fervor);

        sp_cost -= cost_reduction;

        actor::player_state::g_exorcist_fervor -= cost_reduction;
    }

    return sp_cost;
}

static void apply_spell_cost(
    actor::Actor& caster,
    int cost,
    const SpellCostType cost_type)
{
    switch (cost_type) {
    case SpellCostType::spirit: {
        if (actor::is_player(&caster) && player_bon::is_bg(Bg::exorcist)) {
            cost = absorb_sp_cost_with_exorcist_fervor(cost);
        }

        if (cost > 0) {
            actor::hit_sp(caster, cost, nullptr, Verbose::no);
        }
    } break;

    case SpellCostType::hit_points: {
        actor::hit(caster, cost, DmgType::pure, nullptr, AllowWound::no);
    } break;
    }
}

static bool should_give_regen_from_flagellant_trait(
    const actor::Actor& caster,
    const int hp_before_casting,
    const SpellDomain spell_domain)
{
    return (
        actor::is_player(&caster) &&
        player_bon::has_trait(TraitId::galvanization) &&
        (spell_domain == SpellDomain::blood) &&
        (caster.m_hp < hp_before_casting));
}

static void apply_regen_from_flagellant_trait()
{
    prop::Prop* const regen = prop::make(prop::Id::regenerating);

    regen->set_duration(rnd::range(4, 6));

    map::g_player->m_properties.apply(regen);
}

static bool can_player_see_any_target(const std::vector<actor::Actor*>& targets)
{
    return (
        std::any_of(
            std::begin(targets),
            std::end(targets),
            [](const actor::Actor* const actor) {
                return actor::can_player_see_actor(*actor);
            }));
}

bool can_player_see_caster_and_any_target(
    const actor::Actor& caster,
    const std::vector<actor::Actor*>& targets)
{
    return actor::can_player_see_actor(caster) && can_player_see_any_target(targets);
}

void give_player_sp_for_resist_with_absorption_trait()
{
    actor::restore_sp(
        *map::g_player,
        rnd::range(1, 6),
        actor::AllowRestoreAboveMax::no,
        Verbose::yes);
}

// -----------------------------------------------------------------------------
// spells
// -----------------------------------------------------------------------------
namespace spells
{
Spell* make(const SpellId spell_id)
{
    switch (spell_id) {
    case SpellId::aura_of_decay:
        return new SpellAuraOfDecay();

    case SpellId::enfeeble:
        return new SpellEnfeeble();

    case SpellId::curse:
        return new SpellCurse();

    case SpellId::poison:
        return new SpellPoison();

    case SpellId::slow:
        return new SpellSlow();

    case SpellId::terrify:
        return new SpellTerrify();

    case SpellId::threat_projection:
        return new SpellThreatProjection();

    case SpellId::disease:
        return new SpellDisease();

    case SpellId::force_bolt:
        return new SpellBolt(new ForceBolt);

    case SpellId::darkbolt:
        return new SpellBolt(new Darkbolt);

    case SpellId::gnawing_torrent:
        return new SpellBolt(new GnawingTorrent);

    case SpellId::aza_gaze:
        return new SpellAzaGaze();

    case SpellId::summon_random:
        return new SpellSummon(new SummonRandom);

    case SpellId::summon_water_creature:
        return new SpellSummon(new SummonWaterCreature);

    case SpellId::summon_tentacles:
        return new SpellSummon(new SummonTentacles);

    case SpellId::heal:
        return new SpellHeal();

    case SpellId::knockback:
        return new SpellKnockBack();

    case SpellId::teleport:
        return new SpellTeleport();

    case SpellId::temporal_echo:
        return new SpellTemporalEcho();

    case SpellId::cataclysm:
        return new SpellCataclysm();

    case SpellId::pestilence:
        return new SpellPestilence();

    case SpellId::mirror_images:
        return new SpellMirrorImages();

    case SpellId::projected_strike:
        return new SpellProjectedStrike();

    case SpellId::control_object:
        return new SpellControlObject();

    case SpellId::cleansing_fire:
        return new SpellCleansingFire();

    case SpellId::sanctuary:
        return new SpellSanctuary();

    case SpellId::purge:
        return new SpellPurge();

    case SpellId::frenzy:
        return new SpellFrenzy();

    case SpellId::inscribe_boundary_sigil:
        return new SpellInscribeBoundarySigil();

    case SpellId::bless:
        return new SpellBless();

    case SpellId::cancellation:
        return new SpellCancellation();

    case SpellId::mi_go_hypno:
        return new SpellMiGoHypno();

    case SpellId::burn:
        return new SpellBurn();

    case SpellId::blind:
        return new SpellBlind();

    case SpellId::deafen:
        return new SpellDeafen();

    case SpellId::light:
        return new SpellLight();

    case SpellId::transmut:
        return new SpellTransmut();

    case SpellId::clairvoyance:
        return new SpellClairvoyance();

    case SpellId::invis:
        return new SpellInvis();

    case SpellId::see_invis:
        return new SpellSeeInvis();

    case SpellId::spell_shield:
        return new SpellSpellShield();

    case SpellId::haste:
        return new SpellHaste();

    case SpellId::premonition:
        return new SpellPremonition();

    case SpellId::erudition:
        return new SpellErudition();

    case SpellId::expulsion:
        return new SpellExpulsion();

    case SpellId::identify:
        return new SpellIdentify();

    case SpellId::blood_tempering:
        return new SpellBloodTempering();

    case SpellId::sacrifice_life:
        return new SpellSacrificeLife();

    case SpellId::shed_impurity:
        return new SpellShedImpurity();

    case SpellId::thorns:
        return new SpellThorns();

    case SpellId::crimson_passage:
        return new SpellCrimsonPassage();

    case SpellId::heal_others:
        return new SpellHealOthers();

    case SpellId::END:
        break;
    }

    ASSERT(false);

    return nullptr;
}

SpellId str_to_spell_id(const std::string& str)
{
    return s_str_to_spell_id_map.at(str);
}

SpellSkill str_to_spell_skill_id(const std::string& str)
{
    return s_str_to_spell_skill_map.at(str);
}

std::string spell_domain_title(const SpellDomain domain)
{
    switch (domain) {
    case SpellDomain::channeling:
        return i18n::get("spells.domain.channeling", "Channeling");

    case SpellDomain::corruption:
        return i18n::get("spells.domain.corruption", "Corruption");

    case SpellDomain::illusion:
        return i18n::get("spells.domain.illusion", "Illusion");

    case SpellDomain::mind:
        return i18n::get("spells.domain.mind", "Mind");

    case SpellDomain::time:
        return i18n::get("spells.domain.time", "Time");

    case SpellDomain::warding:
        return i18n::get("spells.domain.warding", "Warding");

    case SpellDomain::blood:
        return i18n::get("spells.domain.blood", "Blood");

    case SpellDomain::END:
        break;
    }

    ASSERT(false);

    return "";
}

std::string skill_to_str(const SpellSkill skill)
{
    switch (skill) {
    case SpellSkill::basic:
        return i18n::get("spells.skill.basic", "basic");

    case SpellSkill::expert:
        return i18n::get("spells.skill.expert", "expert");

    case SpellSkill::master:
        return i18n::get("spells.skill.master", "master");

    case SpellSkill::transcendent:
        return i18n::get("spells.skill.transcendent", "transcendent");
    }

    ASSERT(false);

    return "";
}

ShockSrc spell_domain_to_shock_type(const SpellDomain domain)
{
    return s_spell_domain_to_shock_type_map.at(domain);
}

terrain::DidOpen run_opening_spell_effect_at(
    const P& pos,
    const SpellSkill skill)
{
    (void)skill;

    terrain::Terrain* const terrain = map::g_terrain.at(pos);

    if (terrain->id() == terrain::Id::door) {
        auto* const door = static_cast<terrain::Door*>(terrain);

        if (door->is_open()) {
            return terrain::DidOpen::no;
        }
    }

    const auto did_open = terrain->open(nullptr);

    return did_open;
}

terrain::DidClose run_close_spell_effect_at(
    const P& pos,
    const SpellSkill skill)
{
    (void)skill;

    terrain::Terrain* const terrain = map::g_terrain.at(pos);

    if (terrain->id() == terrain::Id::door) {
        if (!static_cast<terrain::Door*>(terrain)->is_open()) {
            return terrain::DidClose::no;
        }
    }

    // TODO: Shouldn't the actor parameter be the caster here?
    const auto did_close = terrain->close(nullptr);

    return did_close;
}

void run_mi_go_hypno_effect(actor::Actor& target)
{
    prop::Prop* prop_fainted = prop::make(prop::Id::fainted);

    prop_fainted->set_duration(rnd::range(2, 10));

    target.m_properties.apply(prop_fainted);
}

}  // namespace spells

// -----------------------------------------------------------------------------
// Spell
// -----------------------------------------------------------------------------
Range Spell::cost_range(
    const SpellSkill skill,
    const actor::Actor* const caster) const
{
    const int cost_max = base_max_cost(skill, caster);
    const int cost_min = (cost_max + 1) / 2;

    Range range(cost_min, cost_max);

    if (actor::is_player(caster) &&
        caster->m_properties.has(prop::Id::meditative_focused)) {
        if (player_bon::has_trait(TraitId::sage)) {
            range.min = 0;
            range.max = 0;
        }
        else {
            --range.min;
            --range.max;
        }
    }

    range.min = std::max(0, range.min);
    range.max = std::max(0, range.max);

    return range;
}

void Spell::cast(
    actor::Actor* const caster,
    const SpellSkill skill,
    const SpellSrc spell_src,
    const std::vector<actor::Actor*>& seen_targets) const
{
    TRACE_FUNC_BEGIN;

    ASSERT(caster);

    prop::PropHandler& properties = caster->m_properties;

    // If this is an intrinsic cast, check properties which NEVER allows casting or speaking.
    //
    // NOTE: If this is a non-intrinsic cast (e.g. from a scroll), then we assume that the caller
    // has made all checks themselves.
    //
    if (spell_src == SpellSrc::learned) {
        if (!properties.allow_cast_intr_spell_absolute(Verbose::yes)) {
            return;
        }

        if (!properties.allow_speak(Verbose::yes)) {
            // TODO: Not all spells "require making noise", it seems insconsistent to outright
            // prevent casting when the caster cannot speak.
            return;
        }
    }

    // OK, we can try to cast

    PlayerAwareOfCast player_aware = PlayerAwareOfCast::no;

    if (actor::is_player(caster)) {
        TRACE << "Player casting spell" << "\n";

        player_aware = PlayerAwareOfCast::yes;

        const ShockSrc shock_src =
            (spell_src == SpellSrc::learned)
            ? s_spell_domain_to_shock_type_map.at(domain())
            : ShockSrc::use_strange_item;

        int shock = shock_value();

        if (map::g_player->m_inv.has_item_in_backpack(item::Id::necronomicon)) {
            shock *= 2;
        }

        if (shock > 0) {
            map::g_player->incr_shock((double)shock, shock_src);
        }

        // Make sound if noisy - casting from scrolls is always noisy.
        if (is_noisy(skill) || (spell_src == SpellSrc::manuscript)) {
            Snd snd(
                "",
                SndSpec{}
                    .sfx(audio::SfxId::END)
                    .ignore_msg_if_origin_seen(IgnoreMsgIfOriginSeen::yes)
                    .origin(caster->m_pos)
                    .actor(caster)
                    .vol(SndVol::low)
                    .alerts(AlertsMon::yes));

            snd.run();
        }
    }
    else {
        // Caster is monster
        TRACE << "Monster casting spell" << "\n";

        // If sound is noisy, print a sound message. Also if no sound was heard by the player and
        // the monster is seen, print a "visual" message.

        bool did_player_hear_sound = false;

        if (is_noisy(skill)) {
            Snd snd(
                generate_mon_cast_sound_msg(*caster),
                SndSpec{}
                    .sfx(audio::SfxId::END)
                    .ignore_msg_if_origin_seen(IgnoreMsgIfOriginSeen::no)
                    .origin(caster->m_pos)
                    .actor(caster)
                    .vol(SndVol::low)
                    .alerts(AlertsMon::no));

            snd.run();

            did_player_hear_sound = snd.did_player_hear_sound();

            if (did_player_hear_sound) {
                player_aware = PlayerAwareOfCast::yes;
            }
        }

        if (actor::can_player_see_actor(*caster)) {
            player_aware = PlayerAwareOfCast::yes;

            if (!did_player_hear_sound) {
                const std::string visual_msg = generate_mon_cast_visual_msg(*caster);

                if (!visual_msg.empty()) {
                    msg_log::add(generate_mon_cast_visual_msg(*caster));
                }
            }
        }
    }

    bool allow_cast = true;

    const int hp_before = caster->m_hp;

    if (spell_src == SpellSrc::learned) {
        const Range range = cost_range(skill, caster);

        if (range.min > 0) {
            TRACE
                << "Applying spell cost for spell "
                << "'" << name() << "', "
                << "caster "
                << "'" << actor::name_a(*caster) << "'"
                << "\n";

            apply_spell_cost(*caster, range.roll(), cost_type());
        }

        // Check properties which MAY allow casting.
        allow_cast = properties.allow_cast_intr_spell_chance(Verbose::yes);
    }

    const bool is_focused_player =
        actor::is_player(caster) &&
        caster->m_properties.has(prop::Id::meditative_focused);

    if (allow_cast && actor::is_alive(*caster)) {
        TRACE
            << "Running spell effect for spell "
            << "'" << name() << "', "
            << "caster "
            << "'" << actor::name_a(*caster) << "'"
            << "\n";

        // Here we run the actual casting of the spell itself:
        run_effect(caster, skill, seen_targets, player_aware);

        end_properties_for_casting_spell(*caster, id());

        if (should_give_regen_from_flagellant_trait(*caster, hp_before, domain())) {
            apply_regen_from_flagellant_trait();
        }

        // Disable tenebrous spell for the player?
        const bool should_forget_spell =
            actor::is_player(caster) &&
            is_tenebrous() &&
            (spell_src == SpellSrc::learned);

        if (should_forget_spell) {
            player_spells::forget_spell(id());
        }
    }

    const bool allow_side_effect =
        actor::is_player(caster) &&
        actor::is_alive(*caster) &&
        !player_bon::is_bg(Bg::exorcist) &&
        allow_cast &&
        (base_max_cost(skill, caster) > 0);

    if (allow_side_effect && rnd::one_in(7)) {
        spell_side_effects::run_random_side_effect(*caster);
    }

    const bool is_casting_from_item = (spell_src == SpellSrc::item);

    if (!is_casting_from_item && !is_focused_player) {
        game_time::tick();
    }

    TRACE_FUNC_END;
}

void Spell::on_resist(actor::Actor& target) const
{
    const bool is_player = actor::is_player(&target);

    const bool player_see_target = actor::can_player_see_actor(target);

    if (player_see_target) {
        std::string resist_msg;

        if (is_player) {
            // TODO: This should be "I resist a spell" instead of "the" spell, if the player is
            // unaware of the cast.
            resist_msg = spell_resist_msg_player();
        }
        else {
            const std::string mon_name =
                text_format::first_to_upper(
                    actor::name_the(target));

            // TODO: This should be "X resists a spell" instead of "the" spell, if the player is
            // unaware of the cast.
            resist_msg = mon_name + spell_resist_msg_mon_suffix();
        }

        msg_log::add(resist_msg);

        if (is_player) {
            audio::play(audio::SfxId::spell_shield_break);
        }

        draw_blast_at_cells({target.m_pos}, colors::white());
    }

    // End spell resistance if not a natural property.
    if (!target.m_data->natural_props[(size_t)prop::Id::r_spell]) {
        const bool is_ended = target.m_properties.end_prop(prop::Id::r_spell);

        if (is_ended && is_player && player_bon::has_trait(TraitId::absorption)) {
            give_player_sp_for_resist_with_absorption_trait();
        }
    }
}

std::vector<std::string> Spell::descr(
    const SpellSkill skill,
    const SpellSrc spell_src) const
{
    std::vector<std::string> lines = descr_specific(skill);

    if (spell_src != SpellSrc::manuscript) {
        lines.push_back(get_noise_descr(is_noisy(skill)));
    }

    if (spell_src == SpellSrc::learned) {
        const std::string forgotten_hint_str = i18n::get(
            "spells.forgotten_hint",
            "Forgotten spells can be recalled by "
            "studying inscribed objects "
            "or by casting them from a manuscript.");

        if (player_spells::is_spell_forgotten(id())) {
            lines.emplace_back(
                i18n::get(
                    "spells.forgotten_descr_prefix",
                    "Forgotten - this spell can no longer be "
                    "cast from memory. ") +
                forgotten_hint_str);
        }
        else if (is_tenebrous()) {
            lines.emplace_back(
                i18n::get(
                    "spells.tenebrous_descr_prefix",
                    "Tenebrous - this spell will be instantly "
                    "forgotten if cast from memory. ") +
                forgotten_hint_str);
        }
    }

    std::string str;

    if (can_be_improved_with_skill()) {
        str = get_skill_descr(skill, spell_src);
    }

    if (!player_bon::is_bg(Bg::exorcist)) {
        text_format::append_with_space(str, domain_descr());
    }

    if (!str.empty()) {
        lines.push_back(str);
    }

    return lines;
}

std::string Spell::domain_descr() const
{
    const SpellDomain my_domain = domain();

    if (my_domain == SpellDomain::END) {
        return "";
    }

    const std::string domain_title =
        text_format::first_to_upper(
            spells::spell_domain_title(domain()));

    return
        i18n::get("spells.domain_descr_prefix", "It belongs to the \"") +
        domain_title +
        i18n::get("spells.domain_descr_suffix", "\" domain.");
}

int Spell::shock_value() const
{
    const SpellShock type = shock_type();

    int value = 0;

    switch (type) {
    case SpellShock::none:
        value = 0;
        break;

    case SpellShock::mild:
        value = 4;
        break;

    case SpellShock::disturbing:
        value = 16;
        break;

    case SpellShock::severe:
        value = 24;
        break;
    }

    return value;
}

