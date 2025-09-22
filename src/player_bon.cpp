// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "player_bon.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>

#include "actor.hpp"
#include "actor_data.hpp"
#include "actor_player_state.hpp"
#include "colors.hpp"
#include "create_character.hpp"
#include "debug.hpp"
#include "game.hpp"
#include "global.hpp"
#include "item_data.hpp"
#include "map.hpp"
#include "player_spells.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "property_handler.hpp"
#include "random.hpp"
#include "saving.hpp"
#include "spells.hpp"
#include "state.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
struct TraitData
{
        TraitId id {TraitId::END};
        std::string title {};
        std::string descr {};
        std::string extra_descr_when_picking {};
        std::function<void()> on_picked {};
        std::function<void()> on_removed {};
        std::vector<TraitId> trait_prereqs {};
        Bg bg_prereq {Bg::END};
        int clvl_prereq {0};
        std::vector<Bg> blocked_for_bgs {};
};

static TraitData s_trait_data[(size_t)TraitId::END];

// NOTE: This is stored separately from the trait data since we sometimes need
// to update the trait data (e.g. to update trait descriptions containing
// information on the player's current spirit for spell traits). Bundling the
// picked state with the other trait data would be confusing and inconvenient.
static bool s_traits_picked[(size_t)TraitId::END];

static std::vector<player_bon::TraitLogEntry> s_trait_log;

static auto s_player_bg = Bg::END;
static auto s_player_occultist_domain = OccultistDomain::END;

static const int s_occultist_spell_upgrade_lvl_1 = 4;
static const int s_occultist_spell_upgrade_lvl_2 = 8;

static const int s_exorcist_bon_trait_lvl_1 = 2;
static const int s_exorcist_bon_trait_lvl_2 = 4;
static const int s_exorcist_bon_trait_lvl_3 = 6;

static const int s_flagellant_spell_upgrade_lvl_1 = 4;
static const int s_flagellant_spell_upgrade_lvl_2 = 8;

static std::string trait_descr_for_spell(
        const SpellId spell_id,
        const SpellSkill skill)
{
        std::unique_ptr<Spell> spell(spells::make(spell_id));

        std::string str =
                "Gain the ability to cast \"" +
                spell->name() +
                "\"";

        if (spell->can_be_improved_with_skill()) {
                str +=
                        " at " +
                        spells::skill_to_str(skill) +
                        " level";
        }

        str += " -";

        const auto descr = spell->descr_specific(skill);

        for (const auto& line : descr) {
                str += " " + line;
        }

        // Assert that the player character has been initialized, as it is used
        // below, and also spell costs might be affected by whether the caster
        // is the player or not.
        ASSERT(actor::is_player(map::g_player));

        const auto cost_str = spell->cost_range(skill, map::g_player).str();

        str += (" This spell costs " +
                cost_str +
                " spirit to cast.");

        return str;
}

static std::string get_player_available_sp_str()
{
        const std::string sp_str = std::to_string(map::g_player->m_sp);
        const std::string max_sp_str = std::to_string(actor::max_sp(*map::g_player));

        std::string descr = "You currently have " + sp_str + "/" + max_sp_str + " spirit";

        if (player_bon::is_bg(Bg::exorcist)) {
                const std::string fp_str = std::to_string(actor::player_state::g_exorcist_fervor);
                const std::string max_fp_str = std::to_string(actor::player_exorcist_max_fervor());

                descr += " and " + fp_str + "/" + max_fp_str + " fervor";
        }

        descr += ".";

        return descr;
}

static TraitData& trait_data(const TraitId id)
{
        ASSERT(id != TraitId::END);

        return s_trait_data[(size_t)id];
}

static void set_trait_data(TraitData& d)
{
        ASSERT(d.id != TraitId::END);

        s_trait_data[(size_t)d.id] = d;

        d = {};
}

static void update_trait_data()
{
        for (auto& d : s_trait_data) {
                d = {};
        }

        TraitData d;

        // --- Adept Melee Fighter ---
        d.id = TraitId::adept_melee;
        d.title = "Adept Melee Fighter";
        d.descr = "+10% hit chance and +1 damage with melee attacks";
        set_trait_data(d);

        // --- Expert Melee Fighter ---
        d = trait_data(TraitId::adept_melee);
        d.id = TraitId::expert_melee;
        d.title = "Expert Melee Fighter";
        d.trait_prereqs = {TraitId::adept_melee};
        d.blocked_for_bgs = {Bg::exorcist};
        set_trait_data(d);

        // --- Master Melee Fighter ---
        d = trait_data(TraitId::adept_melee);
        d.id = TraitId::master_melee;
        d.title = "Master Melee Fighter";
        d.trait_prereqs = {TraitId::expert_melee};
        d.blocked_for_bgs = {Bg::exorcist, Bg::occultist};
        set_trait_data(d);

        // --- Adept Marksman ---
        d.id = TraitId::adept_marksman;
        d.title = "Adept Marksman";
        d.descr = (
                "+10% hit chance and +1 minimum damage with firearms and thrown weapons "
                "(cannot raise maximum damage)");
        d.blocked_for_bgs = {Bg::ghoul};
        set_trait_data(d);

        // --- Expert Marksman ---
        d = trait_data(TraitId::adept_marksman);
        d.id = TraitId::expert_marksman;
        d.title = "Expert Marksman";
        d.trait_prereqs = {TraitId::adept_marksman};
        d.blocked_for_bgs = {Bg::ghoul, Bg::exorcist};
        set_trait_data(d);

        // --- Master Marksman ---
        d = trait_data(TraitId::adept_marksman);
        d.id = TraitId::master_marksman;
        d.title = "Master Marksman";
        d.trait_prereqs = {TraitId::expert_marksman};
        d.blocked_for_bgs = {Bg::ghoul, Bg::exorcist, Bg::occultist, Bg::flagellant};
        set_trait_data(d);

        // --- Cool-headed ---
        d.id = TraitId::cool_headed;
        d.title = "Cool-headed";
        d.descr = "+20% mental shock resistance";
        set_trait_data(d);

        // --- Courageous ---
        d = trait_data(TraitId::cool_headed);
        d.id = TraitId::courageous;
        d.title = "Courageous";
        d.trait_prereqs = {TraitId::cool_headed};
        set_trait_data(d);

        // --- Dexterous ---
        d.id = TraitId::dexterous;
        d.title = "Dexterous";
        d.descr = "+25% chance to evade attacks";
        set_trait_data(d);

        // --- Lithe ---
        d = trait_data(TraitId::dexterous);
        d.id = TraitId::lithe;
        d.title = "Lithe";
        d.trait_prereqs = {TraitId::dexterous};
        set_trait_data(d);

        // --- Crippling Strikes ---
        d.id = TraitId::crippling_strikes;
        d.title = "Crippling Strikes";
        d.descr =
                "Your melee attacks have 60% chance to weaken the target "
                "creature for 2-3 turns (reducing their melee damage by half)";
        d.trait_prereqs = {TraitId::dexterous, TraitId::adept_melee};
        d.bg_prereq = Bg::rogue;
        set_trait_data(d);

        // --- Fearless ---
        d.id = TraitId::fearless;
        d.title = "Fearless";
        d.descr = "You cannot become terrified, +10% mental shock resistance";
        d.on_picked = []() {
                prop::Prop* prop = prop::make(prop::Id::r_fear);

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        prop::PropSrc::intr,
                        true,
                        Verbose::no);
        };
        d.on_removed = []() {
                map::g_player->m_properties.end_prop(prop::Id::r_fear);
        };
        d.trait_prereqs = {TraitId::cool_headed};
        set_trait_data(d);

        // --- Stealthy ---
        d.id = TraitId::stealthy;
        d.title = "Stealthy";
        d.descr = "+45% chance to avoid detection by sight";
        set_trait_data(d);

        // --- Imperceptible ---
        d = trait_data(TraitId::stealthy);
        d.id = TraitId::imperceptible;
        d.title = "Imperceptible";
        d.trait_prereqs = {TraitId::stealthy};
        d.bg_prereq = Bg::rogue;
        set_trait_data(d);

        // --- Silent ---
        d.id = TraitId::silent;
        d.title = "Silent";
        d.descr =
                "All your melee attacks are silent (regardless of the weapon), "
                "and creatures are not alerted when you open or close doors, "
                "or wade through water";
        d.trait_prereqs = {TraitId::stealthy};
        set_trait_data(d);

        // --- Vigilant ---
        d.id = TraitId::vigilant;
        d.title = "Vigilant";
        d.descr = "You are always aware of nearby creatures";
        // Blocked for Occultists, since they have access to Clairvoyance (a strictly much better
        // version of Vigilant when fully upgraded).
        d.blocked_for_bgs = {Bg::occultist};
        set_trait_data(d);

        // --- Treasure Hunter ---
        d.id = TraitId::treasure_hunter;
        d.title = "Treasure Hunter";
        d.descr = "You tend to find more items";
        d.blocked_for_bgs = {Bg::exorcist, Bg::ghoul, Bg::war_vet, Bg::flagellant};
        set_trait_data(d);

        // --- Self-aware ---
        d.id = TraitId::self_aware;
        d.title = "Self-aware";
        d.descr =
                "You cannot become confused, the number of remaining turns "
                "for status effects are displayed";
        d.on_picked = []() {
                prop::Prop* prop = prop::make(prop::Id::r_conf);

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        prop::PropSrc::intr,
                        true,
                        Verbose::no);
        };
        d.on_removed = []() {
                map::g_player->m_properties.end_prop(prop::Id::r_conf);
        };
        d.trait_prereqs = {TraitId::stout_spirit, TraitId::cool_headed};
        set_trait_data(d);

        // --- Healer ---
        d.id = TraitId::healer;
        d.title = "Healer";
        d.descr =
                "Using medical equipment requires only half the normal time "
                "and resources";
        d.blocked_for_bgs = {Bg::ghoul};
        set_trait_data(d);

        // --- Rapid Recoverer ---
        d.id = TraitId::rapid_recoverer;
        d.title = "Rapid Recoverer";
        d.descr = "You regenerate 1 hit point every third turn";
        d.trait_prereqs = {TraitId::tough, TraitId::healer};
        d.blocked_for_bgs = {Bg::ghoul};
        set_trait_data(d);

        // --- Survivalist ---
        d.id = TraitId::survivalist;
        d.title = "Survivalist";
        d.descr =
                "You cannot become diseased, "
                "only half your wounds count, "
                "rounded down "
                "(i.e. number of wounds are halved when calculating "
                "combat, hit point and regeneration penalties, "
                "slower walking speed happens at 6 wounds instead of 3, "
                "and you die from 10 wounds instead of 5)";
        d.on_picked = []() {
                prop::Prop* prop = prop::make(prop::Id::r_disease);

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        prop::PropSrc::intr,
                        true,
                        Verbose::no);
        };
        d.on_removed = []() {
                map::g_player->m_properties.end_prop(prop::Id::r_disease);
        };
        d.blocked_for_bgs = {Bg::ghoul, Bg::flagellant};
        set_trait_data(d);

        // --- Stout Spirit ---
        d.id = TraitId::stout_spirit;
        d.title = "Stout Spirit";
        d.descr =
                "+2 spirit points, increased spirit regeneration rate, you "
                "can defy harmful spells (it takes 125-150 turns to regain "
                "spell resistance after a spell is blocked)";
        d.on_picked = []() {
                prop::Prop* prop = prop::make(prop::Id::r_spell);

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        prop::PropSrc::intr,
                        true,
                        Verbose::no);

                const int spi_incr = 2;

                actor::change_max_sp(
                        *map::g_player,
                        spi_incr,
                        Verbose::no);

                actor::restore_sp(
                        *map::g_player,
                        spi_incr,
                        actor::AllowRestoreAboveMax::no,
                        Verbose::no);
        };
        d.on_removed = []() {
                actor::change_max_sp(*map::g_player, -2, Verbose::no);
        };
        set_trait_data(d);

        // --- Strong Spirit ---
        d = trait_data(TraitId::stout_spirit);
        d.id = TraitId::strong_spirit;
        d.title = "Strong Spirit";
        d.descr =
                "+2 spirit points, increased spirit regeneration rate, it "
                "takes 75-100 turns to regain spell resistance after a spell "
                "is blocked";
        d.trait_prereqs = {TraitId::stout_spirit};
        set_trait_data(d);

        // --- Mighty Spirit ---
        d = trait_data(TraitId::stout_spirit);
        d.id = TraitId::mighty_spirit;
        d.title = "Mighty Spirit";
        d.descr =
                "+2 spirit points, increased spirit regeneration rate, it "
                "takes 25-50 turns to regain spell resistance after a spell "
                "is blocked";
        d.trait_prereqs = {TraitId::strong_spirit};
        set_trait_data(d);

        // --- Meditative ---
        d.id = TraitId::meditative;
        d.title = "Meditative";
        d.descr =
                "Applies a focused state which allows the next spell to be "
                "cast without spending a turn, and with the casting cost "
                "reduced by 1 point - it takes 125-150 turns to regain this "
                "state after a spell is cast";
        d.on_picked = []() {
                prop::Prop* prop = prop::make(prop::Id::meditative_focused);

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        prop::PropSrc::intr,
                        true,
                        Verbose::no);
        };
        d.trait_prereqs = {TraitId::strong_spirit, TraitId::cool_headed};
        d.blocked_for_bgs = {Bg::ghoul, Bg::war_vet, Bg::rogue};
        set_trait_data(d);

        // --- Sage ---
        d.id = TraitId::sage;
        d.title = "Sage";
        d.descr =
                "When focused, spells are also cast at a higher skill level, "
                "and the duration to regain the focused state is reduced to "
                "75-100 turns.";
        d.trait_prereqs = {TraitId::meditative};
        d.blocked_for_bgs = trait_data(TraitId::meditative).blocked_for_bgs;
        // TODO: Consider allowing it for Exorcists (and have third level spells for them, probably
        // also traits for the third level spells).
        d.blocked_for_bgs.push_back(Bg::exorcist);
        d.blocked_for_bgs.push_back(Bg::flagellant);
        set_trait_data(d);

        // --- Absorption ---
        d.id = TraitId::absorbtion;
        d.title = "Absorption";
        d.descr =
                "1-6 spirit points are restored each time a spell is resisted "
                "by spell resistance (granted by spirit traits, or the Spell "
                "Shield spell)";
        d.trait_prereqs = {TraitId::strong_spirit};
        set_trait_data(d);

        // --- Tough ---
        d.id = TraitId::tough;
        d.title = "Tough";
        d.descr =
                "+6 hit points, "
                "+10% chance to resist burning, poisoning and paralysis, "
                "less likely to sprain when kicking, more likely to "
                "succeed with object interactions requiring strength (e.g. "
                "bashing things open)";
        d.on_picked = []() {
                const int hp_incr = 6;

                actor::change_max_hp(*map::g_player, hp_incr, Verbose::no);

                actor::restore_hp(
                        *map::g_player,
                        hp_incr,
                        actor::AllowRestoreAboveMax::no,
                        Verbose::no);
        };
        d.on_removed = []() {
                actor::change_max_hp(*map::g_player, -6, Verbose::no);
        };
        set_trait_data(d);

        // --- Rugged ---
        d = trait_data(TraitId::tough);
        d.id = TraitId::rugged;
        d.title = "Rugged";
        d.trait_prereqs = {TraitId::tough};
        set_trait_data(d);

        // --- Unbreakable ---
        d = trait_data(TraitId::rugged);
        d.id = TraitId::unbreakable;
        d.title = "Unbreakable";
        d.bg_prereq = Bg::flagellant;
        d.trait_prereqs = {TraitId::rugged};
        set_trait_data(d);

        // --- Thick Skinned ---
        d.id = TraitId::thick_skinned;
        d.title = "Thick Skinned";
        d.descr = "+1 armor point (physical damage reduced by 1 point)";
        d.trait_prereqs = {TraitId::tough};
        set_trait_data(d);

        // --- Callous ---
        d = trait_data(TraitId::thick_skinned);
        d.id = TraitId::callous;
        d.title = "Callous";
        d.bg_prereq = Bg::flagellant;
        d.trait_prereqs = {TraitId::thick_skinned};
        set_trait_data(d);

        // --- Resistant ---
        d.id = TraitId::resistant;
        d.title = "Resistant";
        d.descr =
                "+25% chance to resist burning, poisoning and paralysis - "
                "and the duration of those effects is halved";
        d.trait_prereqs = {TraitId::tough};
        set_trait_data(d);

        // --- Strong-backed ---
        d.id = TraitId::strong_backed;
        d.title = "Strong-backed";
        d.descr = "+50% carry weight limit";
        d.trait_prereqs = {TraitId::tough};
        set_trait_data(d);

        // --- Bane of the Undead ---
        d.id = TraitId::undead_bane;
        d.title = "Bane of the Undead";
        d.descr =
                "+2 melee and ranged attack damage against all undead "
                "monsters, +50% hit chance against ethereal undead monsters";
        d.trait_prereqs = {TraitId::tough, TraitId::fearless, TraitId::stout_spirit};
        set_trait_data(d);

        // --- Electrically Inclined ---
        d.id = TraitId::elec_incl;
        d.title = "Electrically Inclined";
        d.descr =
                "Rods recharge twice as fast, strange devices are less likely "
                "to malfunction or break, electric lanterns last twice as "
                "long, +1 damage with electricity weapons";
        d.blocked_for_bgs = {Bg::ghoul};
        set_trait_data(d);

        // --- Lesser Clairvoyance ---
        d.id = TraitId::lesser_clairvoyance;
        d.title = "Lesser Clairvoyance";
        d.descr =
                "Specialize in detection and learning. "
                "Clairvoyance spells are cast at a higher skill level. "
                "Provides an intrinsic ability to detect "
                "doors, traps, stairs, "
                "and other locations of interest in the surrounding area";
        d.bg_prereq = Bg::occultist;
        d.clvl_prereq = s_occultist_spell_upgrade_lvl_1;
        d.on_picked = []() {
                auto* searching =
                        static_cast<prop::MagicSearching*>(
                                prop::make(
                                        prop::Id::magic_searching));

                searching->set_indefinite();

                searching->set_range(g_fov_radi_int);

                map::g_player->m_properties.apply(
                        searching,
                        prop::PropSrc::intr,
                        true,
                        Verbose::no);
        };
        set_trait_data(d);

        // --- Greater Clairvoyance ---
        d = trait_data(TraitId::lesser_clairvoyance);
        d.id = TraitId::greater_clairvoyance;
        d.title = "Greater Clairvoyance";
        d.descr =
                "Specialize in detection and learning. "
                "Clairvoyance spells are cast at a higher skill level. "
                "Creatures and items are detected.";
        d.bg_prereq = Bg::occultist;
        d.trait_prereqs = {TraitId::lesser_clairvoyance};
        d.clvl_prereq = s_occultist_spell_upgrade_lvl_2;
        d.on_picked = []() {
                prop::Prop* const prop =
                        map::g_player->m_properties.prop(
                                prop::Id::magic_searching);

                ASSERT(prop);

                auto* const searching = static_cast<prop::MagicSearching*>(prop);

                searching->set_allow_reveal_items();
                searching->set_allow_reveal_creatures();
        };
        set_trait_data(d);

        // --- Lesser Enchantment ---
        d.id = TraitId::lesser_enchantment;
        d.title = "Lesser Enchantment";
        d.descr =
                "Specialize in aiding, debilitating, entrancing, and beguiling. "
                "Enchantment spells are cast at a higher skill level.";
        d.bg_prereq = Bg::occultist;
        d.clvl_prereq = s_occultist_spell_upgrade_lvl_1;
        set_trait_data(d);

        // --- Greater Enchantment ---
        d = trait_data(TraitId::lesser_enchantment);
        d.id = TraitId::greater_enchantment;
        d.title = "Greater Enchantment";
        d.bg_prereq = Bg::occultist;
        d.trait_prereqs = {TraitId::lesser_enchantment};
        d.clvl_prereq = s_occultist_spell_upgrade_lvl_2;
        set_trait_data(d);

        // --- Lesser Invocation ---
        d.id = TraitId::lesser_invocation;
        d.title = "Lesser Invocation";
        d.descr =
                "Specialize in channeling destructive powers. "
                "Invocation spells are cast at a higher skill level.";
        d.bg_prereq = Bg::occultist;
        d.clvl_prereq = s_occultist_spell_upgrade_lvl_1;
        set_trait_data(d);

        // --- Greater Invocation ---
        d = trait_data(TraitId::lesser_invocation);
        d.id = TraitId::greater_invocation;
        d.title = "Greater Invocation";
        d.bg_prereq = Bg::occultist;
        d.trait_prereqs = {TraitId::lesser_invocation};
        d.clvl_prereq = s_occultist_spell_upgrade_lvl_2;
        set_trait_data(d);

        // --- Lesser Transmutation ---
        d.id = TraitId::lesser_transmutation;
        d.title = "Lesser Transmutation";
        d.descr =
                "Specialize in manipulating matter, energy, and time. "
                "Transmutation spells are cast at a higher skill level.";
        d.bg_prereq = Bg::occultist;
        d.clvl_prereq = s_occultist_spell_upgrade_lvl_1;
        set_trait_data(d);

        // --- Greater Transmutation ---
        d = trait_data(TraitId::lesser_transmutation);
        d.id = TraitId::greater_transmutation;
        d.title = "Greater Transmutation";
        d.bg_prereq = Bg::occultist;
        d.trait_prereqs = {TraitId::lesser_transmutation};
        d.clvl_prereq = s_occultist_spell_upgrade_lvl_2;
        set_trait_data(d);

        // --- Cast Bless ---
        d.id = TraitId::cast_bless_i;
        d.title = "Cast Bless";
        d.descr = trait_descr_for_spell(SpellId::bless, SpellSkill::basic);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::learn_spell(SpellId::bless, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::remove_learned_spell(SpellId::bless);
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast Bless II ---
        d.id = TraitId::cast_bless_ii;
        d.title = "Cast Bless II";
        d.descr = trait_descr_for_spell(SpellId::bless, SpellSkill::expert);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::incr_spell_skill(SpellId::bless, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::set_spell_skill(SpellId::bless, SpellSkill::basic);
        };
        d.trait_prereqs = {TraitId::cast_bless_i};
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast Cleansing Fire ---
        d.id = TraitId::cast_cleansing_fire_i;
        d.title = "Cast Cleansing Fire";
        d.descr = trait_descr_for_spell(SpellId::cleansing_fire, SpellSkill::basic);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::learn_spell(SpellId::cleansing_fire, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::remove_learned_spell(SpellId::cleansing_fire);
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast Cleansing Fire II ---
        d.id = TraitId::cast_cleansing_fire_ii;
        d.title = "Cast Cleansing Fire II";
        d.descr = trait_descr_for_spell(SpellId::cleansing_fire, SpellSkill::expert);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::incr_spell_skill(SpellId::cleansing_fire, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::set_spell_skill(SpellId::cleansing_fire, SpellSkill::basic);
        };
        d.trait_prereqs = {TraitId::cast_cleansing_fire_i};
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast Heal ---
        d.id = TraitId::cast_heal_i;
        d.title = "Cast Heal";
        d.descr = trait_descr_for_spell(SpellId::heal, SpellSkill::basic);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::learn_spell(SpellId::heal, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::remove_learned_spell(SpellId::heal);
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast Heal II ---
        d.id = TraitId::cast_heal_ii;
        d.title = "Cast Heal II";
        d.descr = trait_descr_for_spell(SpellId::heal, SpellSkill::expert);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::incr_spell_skill(SpellId::heal, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::set_spell_skill(SpellId::heal, SpellSkill::basic);
        };
        d.trait_prereqs = {TraitId::cast_heal_i};
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast Light ---
        d.id = TraitId::cast_light_i;
        d.title = "Cast Light";
        d.descr = trait_descr_for_spell(SpellId::light, SpellSkill::basic);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::learn_spell(SpellId::light, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::remove_learned_spell(SpellId::light);
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast Light II ---
        d.id = TraitId::cast_light_ii;
        d.title = "Cast Light II";
        d.descr = trait_descr_for_spell(SpellId::light, SpellSkill::expert);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::incr_spell_skill(SpellId::light, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::set_spell_skill(SpellId::light, SpellSkill::basic);
        };
        d.trait_prereqs = {TraitId::cast_light_i};
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast Sanctuary ---
        d.id = TraitId::cast_sanctuary_i;
        d.title = "Cast Sanctuary";
        d.descr = trait_descr_for_spell(SpellId::sanctuary, SpellSkill::basic);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::learn_spell(SpellId::sanctuary, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::remove_learned_spell(SpellId::sanctuary);
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast Sanctuary II ---
        d.id = TraitId::cast_sanctuary_ii;
        d.title = "Cast Sanctuary II";
        d.descr = trait_descr_for_spell(SpellId::sanctuary, SpellSkill::expert);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::incr_spell_skill(SpellId::sanctuary, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::set_spell_skill(SpellId::sanctuary, SpellSkill::basic);
        };
        d.trait_prereqs = {TraitId::cast_sanctuary_i};
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast See Invisible ---
        d.id = TraitId::cast_see_invisible_i;
        d.title = "Cast See Invisible";
        d.descr = trait_descr_for_spell(SpellId::see_invis, SpellSkill::basic);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::learn_spell(SpellId::see_invis, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::remove_learned_spell(SpellId::see_invis);
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Cast See Invisible II ---
        d.id = TraitId::cast_see_invisible_ii;
        d.title = "Cast See Invisible II";
        d.descr = trait_descr_for_spell(SpellId::see_invis, SpellSkill::expert);
        d.extra_descr_when_picking = get_player_available_sp_str();
        d.on_picked = []() {
                player_spells::incr_spell_skill(SpellId::see_invis, Verbose::no);
        };
        d.on_removed = []() {
                player_spells::set_spell_skill(SpellId::see_invis, SpellSkill::basic);
        };
        d.trait_prereqs = {TraitId::cast_see_invisible_i};
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Prolonged Life ---
        d.id = TraitId::prolonged_life;
        d.title = "Prolonged Life";
        d.descr =
                "Any fatal damage received is instead drained fom your "
                "fervor points";
        d.bg_prereq = Bg::exorcist;
        set_trait_data(d);

        // --- Ravenous ---
        d.id = TraitId::ravenous;
        d.title = "Ravenous";
        d.descr =
                "You occasionally feed on living victims when attacking "
                "with claws";
        d.trait_prereqs = {TraitId::adept_melee};
        d.bg_prereq = Bg::ghoul;
        set_trait_data(d);

        // --- Foul ---
        d.id = TraitId::foul;
        d.title = "Foul";
        d.descr =
                "+1 claw damage, when attacking with claws, vicious worms "
                "occasionally burst out from the corpses of your victims to "
                "attack your enemies";
        d.bg_prereq = Bg::ghoul;
        set_trait_data(d);

        // --- Toxic ---
        d.id = TraitId::toxic;
        d.title = "Toxic";
        d.descr =
                "+1 claw damage, you are immune to poison, and attacks with "
                "your claws often poisons your victims";
        d.on_picked = []() {
                prop::Prop* prop = prop::make(prop::Id::r_poison);

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        prop::PropSrc::intr,
                        true,
                        Verbose::no);
        };
        d.on_removed = []() {
                map::g_player->m_properties.end_prop(prop::Id::r_poison);
        };
        d.trait_prereqs = {TraitId::foul};
        d.bg_prereq = Bg::ghoul;
        set_trait_data(d);

        // --- Indomitable Fury ---
        d.id = TraitId::indomitable_fury;
        d.title = "Indomitable Fury";
        d.descr =
                "While frenzied, you are immune to wounds, and your claw "
                "attacks cause fear";
        d.trait_prereqs = {TraitId::adept_melee, TraitId::tough};
        d.bg_prereq = Bg::ghoul;
        set_trait_data(d);

        // --- Elusive ---
        d.id = TraitId::elusive;
        d.title = "Elusive";
        d.descr =
                "Creatures only remember you for half the normal duration "
                "(rounded up).";
        d.bg_prereq = Bg::rogue;
        set_trait_data(d);

        // --- Vicious ---
        d.id = TraitId::vicious;
        d.title = "Vicious";
        d.descr = "+100% backstab damage (in addition to the normal +50%)";
        d.trait_prereqs = {TraitId::stealthy, TraitId::dexterous};
        d.bg_prereq = Bg::rogue;
        set_trait_data(d);

        // --- Ruthless ---
        d.id = TraitId::ruthless;
        d.title = "Ruthless";
        d.descr = "+100% backstab damage";
        d.trait_prereqs = {TraitId::vicious};
        d.bg_prereq = Bg::rogue;
        set_trait_data(d);

        // --- Steady Aimer ---
        d.id = TraitId::steady_aimer;
        d.title = "Steady Aimer";
        d.descr =
                "Standing still gives ranged attacks maximum damage and +10% "
                "hit chance on the following turn, unless damage is taken";
        d.bg_prereq = Bg::war_vet;
        set_trait_data(d);

        // --- Galvanization ---
        d.id = TraitId::galvanization;
        d.title = "Galvanization";
        d.descr =
                "Casting any spell from the Blood domain grants "
                "Regeneration for 4-6 turns "
                "(+1 extra hit point regenerated per turn), if "
                "hit points are lost from casting the spell";
        d.bg_prereq = Bg::flagellant;
        set_trait_data(d);

        d.id = TraitId::enthusiasm;
        d.title = "Enthusiasm";
        d.descr =
                "Doubles all bonuses for the moribund effect";
        d.bg_prereq = Bg::flagellant;
        set_trait_data(d);

        // --- Memento Mori ---
        d.id = TraitId::memento_mori;
        d.title = "Memento Mori";
        d.descr =
                "Raises the threshold of the moribund status to 8 hit points, "
                "and increases the duration of the effect by 50% (rounded down)";
        d.bg_prereq = Bg::flagellant;
        set_trait_data(d);
}

static bool is_trait_blocked_for_bg(const TraitId trait, const Bg bg)
{
        const auto d = trait_data(trait);

        const bool is_blocked_for_bg =
                std::find(
                        std::begin(d.blocked_for_bgs),
                        std::end(d.blocked_for_bgs),
                        bg) != std::end(d.blocked_for_bgs);

        return is_blocked_for_bg;
}

static void incr_spell_skills(const SpellDomain spell_domain)
{
        for (int i = 0; i < (int)SpellId::END; ++i) {
                const auto id = (SpellId)i;

                const std::unique_ptr<Spell> spell(spells::make(id));

                if (spell->player_can_learn() &&
                    (spell->domain() == spell_domain)) {
                        player_spells::incr_spell_skill(id, Verbose::yes);
                }
        }
}

static bool is_flagellant_spell_upgrade_clvl(const int clvl)
{
        return (
                (clvl == s_flagellant_spell_upgrade_lvl_1) ||
                (clvl == s_flagellant_spell_upgrade_lvl_2));
}

// -----------------------------------------------------------------------------
// player_bon
// -----------------------------------------------------------------------------
namespace player_bon
{
void init()
{
        s_player_bg = Bg::END;

        s_player_occultist_domain = OccultistDomain::END;

        for (size_t i = 0; i < (size_t)TraitId::END; ++i) {
                s_traits_picked[i] = false;
        }

        update_trait_data();

        s_trait_log.clear();
}

void save()
{
        saving::put_int((int)s_player_bg);

        saving::put_int((int)s_player_occultist_domain);

        for (size_t i = 0; i < (size_t)TraitId::END; ++i) {
                saving::put_bool(s_traits_picked[i]);
        }

        saving::put_int((int)s_trait_log.size());

        for (const TraitLogEntry& e : s_trait_log) {
                saving::put_int(e.clvl);

                saving::put_int((int)e.trait_id);

                saving::put_bool(e.is_removal);
        }
}

void load()
{
        s_player_bg = (Bg)saving::get_int();

        s_player_occultist_domain = (OccultistDomain)saving::get_int();

        for (size_t i = 0; i < (size_t)TraitId::END; ++i) {
                s_traits_picked[i] = saving::get_bool();
        }

        const int nr_trait_log_entries = saving::get_int();

        s_trait_log.resize(nr_trait_log_entries);

        for (player_bon::TraitLogEntry& e : s_trait_log) {
                e.clvl = saving::get_int();

                e.trait_id = (TraitId)saving::get_int();

                e.is_removal = saving::get_bool();
        }
}

std::string bg_title(const Bg id)
{
        switch (id) {
        case Bg::exorcist:
                return "Exorcist";

        case Bg::flagellant:
                return "Flagellant";

        case Bg::ghoul:
                return "Ghoul";

        case Bg::occultist:
                return "Occultist";

        case Bg::rogue:
                return "Rogue";

        case Bg::war_vet:
                return "War Veteran";

        case Bg::END:
                break;
        }

        ASSERT(false);

        return "";
}

SpellDomain occultist_domain_to_spell_domain(const OccultistDomain occultist_domain)
{
        switch (occultist_domain) {
        case OccultistDomain::clairvoyant:
                return SpellDomain::clairvoyance;
                break;

        case OccultistDomain::enchanter:
                return SpellDomain::enchantment;
                break;

        case OccultistDomain::invoker:
                return SpellDomain::invocation;
                break;

        case OccultistDomain::transmuter:
                return SpellDomain::transmutation;
                break;

        case OccultistDomain::END:
                break;
        }

        return SpellDomain::END;
}

std::string trait_title(const TraitId id)
{
        return trait_data(id).title;
}

std::vector<ColoredString> bg_descr(const Bg id)
{
        std::vector<ColoredString> descr;

        auto put = [&descr](const std::string& str) {
                descr.emplace_back(str, colors::text());
        };

        auto put_trait = [&descr](const TraitId trait_id) {
                const auto t = trait_title(trait_id);
                const auto d = trait_descr(trait_id);

                descr.emplace_back("{COLOR_WHITE}" + t + "{color_reset}: " + d, colors::gray());
        };

        switch (id) {
        case Bg::exorcist:
                put("Cannot use manuscripts, altars, monoliths, or gongs, "
                    "but instead gains experience and fervor for destroying "
                    "these (manuscripts are destroyed when picking them up). "
                    "Fervor can be used for casting spells - these points "
                    "are used automatically when there is not enough "
                    "spirit points to cast from.");
                put("");
                put("Starts with a Holy Symbol, which can restore "
                    "spirit points and grant resistance against "
                    "mental shock and fear.");
                put("");
                put("Gains a bonus trait at character levels " +
                    std::to_string(s_exorcist_bon_trait_lvl_1) +
                    ", " +
                    std::to_string(s_exorcist_bon_trait_lvl_2) +
                    ", and " +
                    std::to_string(s_exorcist_bon_trait_lvl_3) +
                    ".");
                put("");
                put_trait(TraitId::stout_spirit);
                put("");
                put_trait(TraitId::undead_bane);
                break;

        case Bg::flagellant:
                put("No mental shock received for taking damage.");
                put("");
                put("If health is reduced to 6 hit points or below when taking damage, "
                    "the moribund status is applied for 5-7 turns "
                    "(+3 melee damage, +30% melee hit chance, +3 armor points).");
                put("");
                put("Wears a torture collar which cannot be taken off; "
                    "walking requires extra turns, and stealth and evasion "
                    "are reduced by 20%. However, wearing the collar hardens "
                    "the Flagellant against physical suffering, armor is "
                    "increased by 3 points.");
                put("");
                put("Specializes in spells belonging to the Blood domain. "
                    "At character levels " +
                    std::to_string(s_flagellant_spell_upgrade_lvl_1) +
                    " and " +
                    std::to_string(s_flagellant_spell_upgrade_lvl_2) +
                    ", all spells belonging to this domain are cast at "
                    "a higher skill level.");
                put("");
                put("-25% mental shock taken from casting memorized spells "
                    "from the Blood domain.");
                put("");
                put_trait(TraitId::self_aware);
                put("");
                put_trait(TraitId::tough);
                break;

        case Bg::ghoul:
                put("-50% mental shock taken from seeing monsters and "
                    "standing in darkness - "
                    "but also only gains halved shock reduction from light.");
                put("");
                put("Does not regenerate hit points and cannot use medical equipment - "
                    "instead heals by feeding on corpses "
                    "(feeding is done by waiting on a corpse).");
                put("");
                put("Can incite frenzy at will, and does not become weakened "
                    "when frenzy ends.");
                put("");
                put("+8 hit points.");
                put("");
                put("Is immune to disease and infections.");
                put("");
                put("Does not get sprains.");
                put("");
                put("Can see in darkness.");
                put("");
                put("-15% hit chance with firearms and thrown weapons.");
                put("");
                put("All ghouls are allied.");
                break;

        case Bg::occultist:
                put("-50% mental shock taken from casting memorized spells "
                    "and from using or identifying strange items such as "
                    "potions or manuscripts "
                    "(in addition to \"Cool-headed\").");
                put("");
                put("Can gain traits to increase skill level in various spell domains.");
                put("");
                put("Chooses background in a specific spell domain at character creation, "
                    "which determines starting spells.");
                put("");
                put("+3 spirit points (in addition to \"Stout Spirit\").");
                put("");
                put("Starts with several Bone Charms, that can be used for "
                    "gaining spell resistance or dispelling magic traps.");

                put("");
                put_trait(TraitId::stout_spirit);
                put("");
                put_trait(TraitId::cool_headed);
                break;

        case Bg::rogue:
                put("Mental shock received passively over time is reduced by 25%.");
                put("");
                put("+10% chance to spot hidden monsters, doors, and traps.");
                put("");
                put("Remains aware of the presence of other creatures longer.");
                put("");
                put("Can sense the presence of unique monsters or powerful "
                    "artifacts.");
                put("");
                put("Has acquired an artifact which can cloud the minds of all "
                    "enemies, causing them to forget the presence of the "
                    "user.");
                put("");
                put_trait(TraitId::stealthy);
                break;

        case Bg::war_vet:
                put("Switches to prepared weapon instantly.");
                put("");
                put("Starts with a Flak Jacket.");
                put("");
                put("Maintains armor twice as long before it breaks.");
                put("");
                put_trait(TraitId::adept_marksman);
                put("");
                put_trait(TraitId::adept_melee);
                put("");
                put_trait(TraitId::tough);
                put("");
                put_trait(TraitId::healer);
                break;

        case Bg::END:
                ASSERT(false);
                break;
        }

        return descr;
}

std::string occultist_domain_descr(const OccultistDomain domain)
{
        // TODO: Do not write spell names here, get them from the spell classes.

        switch (domain) {
        case OccultistDomain::clairvoyant:
                return (
                        "You have previously dabbled in the casting of "
                        "clairvoyance spells, "
                        "and have basic knowledge of "
                        "Premonition (large evasion bonus) and "
                        "Identify (learn the true nature of items).");

        case OccultistDomain::enchanter:
                return (
                        "You have previously dabbled in the casting of "
                        "enchantment spells, "
                        "and have basic knowledge of "
                        "Terrify and Heal.");

        case OccultistDomain::invoker:
                return (
                        "You have previously dabbled in the casting of "
                        "invocation spells, "
                        "and have basic knowledge of "
                        "Darkbolt (fires bolts that damage and paralyze) and "
                        "Aura of Decay (damages nearby creatures over time).");

        case OccultistDomain::transmuter:
                return (
                        "You have previously dabbled in the casting of "
                        "transmutation spells, "
                        "and have basic knowledge of "
                        "Haste (all actions are faster) and "
                        "Transmute (convert items into other items).");

        case OccultistDomain::END:
                ASSERT(false);
                break;
        }

        return "";
}

std::string trait_descr(const TraitId id)
{
        return trait_data(id).descr;
}

std::string trait_descr_extra_when_picking(const TraitId id)
{
        return trait_data(id).extra_descr_when_picking;
}

TraitPrereqData trait_prereqs(const TraitId trait, const Bg bg)
{
        const auto& d = trait_data(trait);

        TraitPrereqData result;

        result.clvl = d.clvl_prereq;
        result.traits = d.trait_prereqs;
        result.bg = d.bg_prereq;

        // Remove traits which are blocked for this background (prerequisites are considered
        // fulfilled).
        for (auto it = std::begin(result.traits);
             it != std::end(result.traits);) {
                if (is_trait_blocked_for_bg(*it, bg)) {
                        it = result.traits.erase(it);
                }
                else {
                        // Not blocked
                        ++it;
                }
        }

        // Sort traits lexicographically.
        std::sort(
                std::begin(result.traits),
                std::end(result.traits),
                [](const TraitId& t1, const TraitId& t2) {
                        const std::string str1 = trait_title(t1);
                        const std::string str2 = trait_title(t2);
                        return str1 < str2;
                });

        return result;
}

Bg bg()
{
        return s_player_bg;
}

OccultistDomain occultist_starting_domain()
{
        return s_player_occultist_domain;
}

bool is_bg(Bg bg)
{
        ASSERT(bg != Bg::END);

        return bg == s_player_bg;
}

bool has_trait(const TraitId id)
{
        return s_traits_picked[(size_t)id];
}

std::vector<Bg> pickable_bgs()
{
        std::vector<Bg> result;

        result.reserve((int)Bg::END);

        for (int i = 0; i < (int)Bg::END; ++i) {
                result.push_back((Bg)i);
        }

        // Sort lexicographically.
        std::sort(
                std::begin(result),
                std::end(result),
                [](const Bg bg1, const Bg bg2) {
                        const std::string str1 = bg_title(bg1);
                        const std::string str2 = bg_title(bg2);
                        return str1 < str2;
                });

        return result;
}

std::vector<OccultistDomain> pickable_occultist_domains()
{
        std::vector<OccultistDomain> result;

        result.reserve((int)OccultistDomain::END);

        for (int i = 0; i < (int)OccultistDomain::END; ++i) {
                result.push_back((OccultistDomain)i);
        }

        // Sort lexicographically.
        std::sort(
                std::begin(result),
                std::end(result),
                [](
                        const OccultistDomain domain_1,
                        const OccultistDomain domain_2) {
                        const SpellDomain spell_domain_1 =
                                occultist_domain_to_spell_domain(domain_1);

                        const SpellDomain spell_domain_2 =
                                occultist_domain_to_spell_domain(domain_2);

                        const std::string str1 =
                                spells::spell_domain_title(spell_domain_1);

                        const std::string str2 =
                                spells::spell_domain_title(spell_domain_2);

                        return str1 < str2;
                });

        return result;
}

UnpickedTraitsData unpicked_traits(const Bg bg)
{
        update_trait_data();

        UnpickedTraitsData result;

        for (const TraitData& d : s_trait_data) {
                if (s_traits_picked[(size_t)d.id]) {
                        continue;
                }

                // Check if trait is explicitly blocked for this background.
                const bool is_blocked_for_bg = is_trait_blocked_for_bg(d.id, bg);

                if (is_blocked_for_bg) {
                        continue;
                }

                // Check trait prerequisites (character level, traits and background).

                // NOTE: Traits blocked for the current background are not considered prerequisites.
                const auto prereq_data = trait_prereqs(d.id, bg);

                const bool is_bg_ok =
                        (s_player_bg == prereq_data.bg) ||
                        (prereq_data.bg == Bg::END);

                if (!is_bg_ok) {
                        // Trait not available for this background - don't include it as an
                        // "unpicked trait" (it will never become available).
                        continue;
                }

                // OK the trait *could* be picked eventually.

                const bool is_clvl_ok = game::clvl() >= prereq_data.clvl;

                bool is_trait_prereqs_ok = true;

                for (const TraitId& prereq : prereq_data.traits) {
                        if (!s_traits_picked[(size_t)prereq]) {
                                is_trait_prereqs_ok = false;
                                break;
                        }
                }

                if (is_clvl_ok && is_trait_prereqs_ok) {
                        result.traits_can_be_picked.push_back(d.id);
                }
                else {
                        result.traits_prereqs_not_met.push_back(d.id);
                }

        }  // Trait loop

        // Sort lexicographically
        std::sort(
                std::begin(result.traits_can_be_picked),
                std::end(result.traits_can_be_picked),
                [](const TraitId& t1, const TraitId& t2) {
                        const std::string str1 = trait_title(t1);
                        const std::string str2 = trait_title(t2);
                        return str1 < str2;
                });

        std::sort(
                std::begin(result.traits_prereqs_not_met),
                std::end(result.traits_prereqs_not_met),
                [](const TraitId& t1, const TraitId& t2) {
                        const std::string str1 = trait_title(t1);
                        const std::string str2 = trait_title(t2);
                        return str1 < str2;
                });

        return result;
}  // unpicked_traits

std::vector<TraitId> traits_can_be_removed()
{
        update_trait_data();

        std::vector<TraitId> result;

        for (const auto& d : s_trait_data) {
                if (!s_traits_picked[(size_t)d.id]) {
                        continue;
                }

                bool is_prereq_for_other_trait = false;

                for (const auto& d_other : s_trait_data) {
                        if (!s_traits_picked[(size_t)d_other.id]) {
                                continue;
                        }

                        const auto match =
                                std::find(
                                        std::begin(d_other.trait_prereqs),
                                        std::end(d_other.trait_prereqs),
                                        d.id);

                        if (match != std::end(d_other.trait_prereqs)) {
                                is_prereq_for_other_trait = true;
                                break;
                        }
                }

                if (is_prereq_for_other_trait) {
                        continue;
                }

                result.push_back(d.id);
        }

        return result;
}

void pick_bg(const Bg bg)
{
        TRACE_FUNC_BEGIN;

        ASSERT(bg != Bg::END);

        s_player_bg = bg;

        switch (s_player_bg) {
        case Bg::exorcist: {
                pick_trait(TraitId::stout_spirit);
                pick_trait(TraitId::undead_bane);

                // Mark all scrolls as found, so that they do not yield XP.
                for (auto& d : item::g_data) {
                        if (d.type == ItemType::scroll) {
                                d.is_found = true;
                        }
                }
        } break;

        case Bg::flagellant: {
                pick_trait(TraitId::self_aware);
                pick_trait(TraitId::tough);

                prop::Prop* flagellant_prop = prop::make(prop::Id::flagellant);

                flagellant_prop->set_indefinite();

                map::g_player->m_properties.apply(
                        flagellant_prop,
                        prop::PropSrc::intr,
                        true,
                        Verbose::no);
        } break;

        case Bg::ghoul: {
                prop::Prop* r_disease = prop::make(prop::Id::r_disease);

                r_disease->set_indefinite();

                map::g_player->m_properties.apply(
                        r_disease,
                        prop::PropSrc::intr,
                        true,
                        Verbose::no);

                prop::Prop* darkvis = prop::make(prop::Id::darkvision);

                darkvis->set_indefinite();

                map::g_player->m_properties.apply(
                        darkvis,
                        prop::PropSrc::intr,
                        true,
                        Verbose::no);

                player_spells::learn_spell(SpellId::frenzy, Verbose::no);

                actor::change_max_hp(*map::g_player, 8, Verbose::no);
        } break;

        case Bg::occultist: {
                pick_trait(TraitId::stout_spirit);
                pick_trait(TraitId::cool_headed);

                actor::change_max_sp(*map::g_player, 3, Verbose::no);
        } break;

        case Bg::rogue: {
                pick_trait(TraitId::stealthy);
        } break;

        case Bg::war_vet: {
                pick_trait(TraitId::adept_marksman);
                pick_trait(TraitId::adept_melee);
                pick_trait(TraitId::tough);
                pick_trait(TraitId::healer);
        } break;

        case Bg::END:
                break;
        }

        TRACE_FUNC_END;
}

void pick_occultist_domain(const OccultistDomain domain)
{
        ASSERT(domain != OccultistDomain::END);

        s_player_occultist_domain = domain;

        switch (domain) {
        case OccultistDomain::clairvoyant:
        case OccultistDomain::enchanter:
        case OccultistDomain::invoker:
        case OccultistDomain::transmuter: {
        } break;

        case OccultistDomain::END: {
                ASSERT(false);
        } break;
        }
}

void on_player_gained_lvl(const int new_lvl)
{
        TRACE_FUNC_BEGIN;

        switch (s_player_bg) {
        case Bg::exorcist: {
                const bool is_exorcist_extra_trait =
                        (new_lvl == s_exorcist_bon_trait_lvl_1) ||
                        (new_lvl == s_exorcist_bon_trait_lvl_2) ||
                        (new_lvl == s_exorcist_bon_trait_lvl_3);

                if (is_exorcist_extra_trait) {
                        states::push(
                                std::make_unique<PickTraitState>(
                                        "You gain an extra trait!",
                                        IsCharacterCreationTraitPick::no));
                }
        } break;

        case Bg::flagellant: {
                if (is_flagellant_spell_upgrade_clvl(new_lvl)) {
                        incr_spell_skills(SpellDomain::blood);
                }
        } break;

        case Bg::ghoul:
        case Bg::occultist:
        case Bg::rogue:
        case Bg::war_vet: {
        } break;

        case Bg::END: {
                ASSERT(false);
        } break;
        }

        TRACE_FUNC_END;
}

void set_all_traits_to_picked()
{
        for (size_t i = 0; i < (size_t)TraitId::END; ++i) {
                s_traits_picked[i] = true;
        }
}

void pick_trait(const TraitId id)
{
        TRACE_FUNC_BEGIN;

        ASSERT(id != TraitId::END);

        s_traits_picked[(size_t)id] = true;

        TraitLogEntry trait_log_entry;

        trait_log_entry.trait_id = id;
        trait_log_entry.clvl = game::clvl();
        trait_log_entry.is_removal = false;

        s_trait_log.push_back(trait_log_entry);

        const TraitData& d = trait_data(id);

        if (d.on_picked) {
                // Has trait pick function
                trait_data(id).on_picked();
        }

        TRACE_FUNC_END;
}

void remove_trait(const TraitId id)
{
        TRACE_FUNC_BEGIN;

        ASSERT(id != TraitId::END);

        s_traits_picked[(size_t)id] = false;

        TraitLogEntry trait_log_entry;

        trait_log_entry.trait_id = id;
        trait_log_entry.clvl = game::clvl();
        trait_log_entry.is_removal = true;

        s_trait_log.push_back(trait_log_entry);

        const TraitData& d = trait_data(id);

        // If the trait applies effects when picked, it must also revert those
        ASSERT(!(d.on_picked && !d.on_removed));

        if (d.on_removed) {
                // Has trait removal function
                trait_data(id).on_removed();
        }

        TRACE_FUNC_END;
}

std::vector<TraitLogEntry> trait_log()
{
        return s_trait_log;
}

}  // namespace player_bon
