// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "player_bon.hpp"

#include "actor_player.hpp"
#include "create_character.hpp"
#include "game.hpp"
#include "init.hpp"
#include "inventory.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "player_spells.hpp"
#include "property.hpp"
#include "property_factory.hpp"
#include "property_handler.hpp"
#include "saving.hpp"
#include "spells.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static bool s_traits[(size_t)Trait::END];

static std::vector<player_bon::TraitLogEntry> s_trait_log;

static auto s_current_bg = Bg::END;

static auto s_current_occultist_domain = OccultistDomain::END;

static const int s_exorcist_bon_trait_lvl_1 = 2;
static const int s_exorcist_bon_trait_lvl_2 = 4;
static const int s_exorcist_bon_trait_lvl_3 = 6;

static const int s_occultist_upgrade_lvl_1 = 4;
static const int s_occultist_upgrade_lvl_2 = 8;

static bool is_trait_blocked_for_bg(
        const Trait trait,
        const Bg bg,
        const OccultistDomain occultist_domain)
{
        switch (trait) {
        case Trait::vigilant: {
                return occultist_domain == OccultistDomain::clairvoyant;
        } break;

        case Trait::treasure_hunter: {
                switch (bg) {
                case Bg::exorcist:
                case Bg::ghoul:
                case Bg::war_vet:
                        return true;

                case Bg::occultist:
                        return (
                                occultist_domain !=
                                OccultistDomain::clairvoyant);

                case Bg::rogue:
                        return false;

                case Bg::END:
                        break;
                }

                ASSERT(false);
                return false;
        } break;

        case Trait::expert_melee:
                return bg == Bg::exorcist;

        case Trait::master_melee:
                return (
                        (bg == Bg::exorcist) ||
                        (bg == Bg::occultist));

        case Trait::adept_marksman:
                return bg == Bg::ghoul;

        case Trait::expert_marksman:
                return (
                        (bg == Bg::ghoul) ||
                        (bg == Bg::exorcist));

        case Trait::master_marksman:
                return (
                        (bg == Bg::ghoul) ||
                        (bg == Bg::exorcist) ||
                        (bg == Bg::occultist));

        case Trait::healer:
                // Cannot use Medial Bag
                return bg == Bg::ghoul;

        case Trait::rapid_recoverer:
                // Cannot regen hp passively
                return bg == Bg::ghoul;

        case Trait::survivalist:
                // Has RDISEASE already + a bit of theme mismatch
                return bg == Bg::ghoul;

        case Trait::elec_incl:
                return bg == Bg::ghoul;
                break;

        case Trait::self_aware:
        case Trait::stout_spirit:
        case Trait::strong_spirit:
        case Trait::mighty_spirit:
        case Trait::stealthy:
        case Trait::imperceptible:
        case Trait::silent:
        case Trait::vicious:
        case Trait::ruthless:
        case Trait::adept_melee:
        case Trait::cool_headed:
        case Trait::courageous:
        case Trait::ravenous:
        case Trait::foul:
        case Trait::toxic:
        case Trait::undead_bane:
        case Trait::absorb:
        case Trait::tough:
        case Trait::rugged:
        case Trait::thick_skinned:
        case Trait::resistant:
        case Trait::strong_backed:
        case Trait::dexterous:
        case Trait::lithe:
        case Trait::crippling_strikes:
        case Trait::fearless:
        case Trait::steady_aimer:
        case Trait::indomitable_fury:
        case Trait::cast_bless_i:
        case Trait::cast_bless_ii:
        case Trait::cast_cleansing_fire_i:
        case Trait::cast_cleansing_fire_ii:
        case Trait::cast_heal_i:
        case Trait::cast_heal_ii:
        case Trait::cast_light_i:
        case Trait::cast_light_ii:
        case Trait::cast_sanctuary_i:
        case Trait::cast_sanctuary_ii:
        case Trait::cast_see_invisible_i:
        case Trait::cast_see_invisible_ii:
        case Trait::cast_purge:
        case Trait::prolonged_life:
        case Trait::END: {
        } break;
        }

        return false;
}

static void incr_occultist_spells()
{
        for (int id = 0; id < (int)SpellId::END; ++id) {
                const std::unique_ptr<Spell> spell(spells::make((SpellId)id));

                const bool is_learnable =
                        spell->player_can_learn();

                const bool is_matching_domain =
                        (spell->domain() == s_current_occultist_domain);

                if (is_learnable && is_matching_domain) {
                        player_spells::incr_spell_skill((SpellId)id);
                }
        }
}

// -----------------------------------------------------------------------------
// player_bon
// -----------------------------------------------------------------------------
namespace player_bon {

void init()
{
        s_current_bg = Bg::END;

        s_current_occultist_domain = OccultistDomain::END;

        for (size_t i = 0; i < (size_t)Trait::END; ++i) {
                s_traits[i] = false;
        }

        s_trait_log.clear();
}

void save()
{
        saving::put_int((int)s_current_bg);

        saving::put_int((int)s_current_occultist_domain);

        for (size_t i = 0; i < (size_t)Trait::END; ++i) {
                saving::put_bool(s_traits[i]);
        }

        saving::put_int(s_trait_log.size());

        for (const auto& e : s_trait_log) {
                saving::put_int(e.clvl_picked);

                saving::put_int((int)e.trait_id);
        }
}

void load()
{
        s_current_bg = (Bg)saving::get_int();

        s_current_occultist_domain = (OccultistDomain)saving::get_int();

        for (size_t i = 0; i < (size_t)Trait::END; ++i) {
                s_traits[i] = saving::get_bool();
        }

        const int nr_trait_log_entries = saving::get_int();

        s_trait_log.resize(nr_trait_log_entries);

        for (auto& e : s_trait_log) {
                e.clvl_picked = saving::get_int();

                e.trait_id = (Trait)saving::get_int();
        }
}

std::string bg_title(const Bg id)
{
        switch (id) {
        case Bg::exorcist:
                return "Exorcist";

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

std::string spell_domain_title(const OccultistDomain domain)
{
        switch (domain) {
        case OccultistDomain::clairvoyant:
                return "Clairvoyance";

        case OccultistDomain::enchanter:
                return "Enchantment";

        case OccultistDomain::invoker:
                return "Invocation";

        case OccultistDomain::transmuter:
                return "Transmutation";

        case OccultistDomain::END:
                break;
        }

        ASSERT(false);

        return "";
}

std::string occultist_profession_title(const OccultistDomain domain)
{
        switch (domain) {
        case OccultistDomain::clairvoyant:
                return "Clairvoyant";

        case OccultistDomain::enchanter:
                return "Enchanter";

        case OccultistDomain::invoker:
                return "Invoker";

        case OccultistDomain::transmuter:
                return "Transmuter";

        case OccultistDomain::END:
                break;
        }

        ASSERT(false);

        return "";
}

std::string trait_title(const Trait id)
{
        switch (id) {
        case Trait::adept_melee:
                return "Adept Melee Fighter";

        case Trait::expert_melee:
                return "Expert Melee Fighter";

        case Trait::master_melee:
                return "Master Melee Fighter";

        case Trait::cool_headed:
                return "Cool-headed";

        case Trait::courageous:
                return "Courageous";

        case Trait::absorb:
                return "Absorption";

        case Trait::dexterous:
                return "Dexterous";

        case Trait::lithe:
                return "Lithe";

        case Trait::crippling_strikes:
                return "Crippling Strikes";

        case Trait::fearless:
                return "Fearless";

        case Trait::healer:
                return "Healer";

        case Trait::adept_marksman:
                return "Adept Marksman";

        case Trait::expert_marksman:
                return "Expert Marksman";

        case Trait::master_marksman:
                return "Master Marksman";

        case Trait::steady_aimer:
                return "Steady Aimer";

        case Trait::vigilant:
                return "Vigilant";

        case Trait::rapid_recoverer:
                return "Rapid Recoverer";

        case Trait::survivalist:
                return "Survivalist";

        case Trait::self_aware:
                return "Self-aware";

        case Trait::stout_spirit:
                return "Stout Spirit";

        case Trait::strong_spirit:
                return "Strong Spirit";

        case Trait::mighty_spirit:
                return "Mighty Spirit";

        case Trait::stealthy:
                return "Stealthy";

        case Trait::imperceptible:
                return "Imperceptible";

        case Trait::silent:
                return "Silent";

        case Trait::vicious:
                return "Vicious";

        case Trait::ruthless:
                return "Ruthless";

        case Trait::strong_backed:
                return "Strong-backed";

        case Trait::tough:
                return "Tough";

        case Trait::rugged:
                return "Rugged";

        case Trait::thick_skinned:
                return "Thick Skinned";

        case Trait::resistant:
                return "Resistant";

        case Trait::treasure_hunter:
                return "Treasure Hunter";

        case Trait::undead_bane:
                return "Bane of the Undead";

        case Trait::elec_incl:
                return "Electrically Inclined";

        case Trait::ravenous:
                return "Ravenous";

        case Trait::foul:
                return "Foul";

        case Trait::toxic:
                return "Toxic";

        case Trait::indomitable_fury:
                return "Indomitable Fury";

        case Trait::cast_bless_i:
                return "Cast Bless";

        case Trait::cast_bless_ii:
                return "Cast Bless II";

        case Trait::cast_cleansing_fire_i:
                return "Cast Cleansing Fire";

        case Trait::cast_cleansing_fire_ii:
                return "Cast Cleansing Fire II";

        case Trait::cast_heal_i:
                return "Cast Heal";

        case Trait::cast_heal_ii:
                return "Cast Heal II";

        case Trait::cast_light_i:
                return "Cast Light";

        case Trait::cast_light_ii:
                return "Cast Light II";

        case Trait::cast_sanctuary_i:
                return "Cast Sanctuary";

        case Trait::cast_sanctuary_ii:
                return "Cast Sanctuary II";

        case Trait::cast_see_invisible_i:
                return "Cast See Invisible";

        case Trait::cast_see_invisible_ii:
                return "Cast See Invisible II";

        case Trait::cast_purge:
                return "Cast Purge";

        case Trait::prolonged_life:
                return "Prolonged Life";

        case Trait::END:
                break;
        }

        ASSERT(false);

        return "";
}

std::vector<ColoredString> bg_descr(const Bg id)
{
        std::vector<ColoredString> descr;

        auto put = [&descr](const std::string& str) {
                descr.emplace_back(str, colors::white());
        };

        auto put_trait = [&descr](const Trait trait_id) {
                descr.emplace_back(trait_title(trait_id), colors::white());
                descr.emplace_back(trait_descr(trait_id), colors::gray());
        };

        switch (id) {
        case Bg::exorcist:
                put("Starts with a Holy Symbol, which can restore "
                    "spirit points and grant resistance against "
                    "shock and fear");
                put("");
                put("Cannot use manuscripts, altars, monoliths, or gongs, "
                    "but gains experience and spirit points for destroying "
                    "these (manuscripts are destroyed when picking them up)");
                put("");
                put("Spirit points gained above the maximum level can be kept "
                    "indefinitely until they are spent");
                put("");
                put("Gains a bonus trait at character levels " +
                    std::to_string(s_exorcist_bon_trait_lvl_1) +
                    ", " +
                    std::to_string(s_exorcist_bon_trait_lvl_2) +
                    ", and " +
                    std::to_string(s_exorcist_bon_trait_lvl_3));
                put("");
                put_trait(Trait::stout_spirit);
                put("");
                put_trait(Trait::undead_bane);
                break;

        case Bg::ghoul:
                put("Does not regenerate hit points and cannot use medical "
                    "equipment - heals by feeding on corpses (feeding is done "
                    "while waiting on a corpse)");
                put("");
                put("Can incite frenzy at will, and does not become weakened "
                    "when frenzy ends");
                put("");
                put("+6 hit points");
                put("");
                put("Is immune to disease and infections");
                put("");
                put("Does not get sprains");
                put("");
                put("Can see in darkness");
                put("");
                put("-50% shock taken from seeing monsters");
                put("");
                put("-15% hit chance with firearms and thrown weapons");
                put("");
                put("All ghouls are allied");
                break;

        case Bg::occultist:
                put("Specializes in a spell domain (selected at character "
                    "creation). At character levels " +
                    std::to_string(s_occultist_upgrade_lvl_1) +
                    " and " +
                    std::to_string(s_occultist_upgrade_lvl_2) +
                    ", all spells "
                    "belonging to the chosen domain are cast with greater "
                    "power. This choice also determines starting spells");
                put("");
                put("-50% shock taken from casting spells, and from carrying, "
                    "using or identifying strange items (e.g. drinking a "
                    "potion or carrying a disturbing artifact)");
                put("");
                put("Can dispel magic traps, doing so grants spirit points");
                put("");
                put("+3 spirit points (in addition to \"Stout Spirit\")");
                put("");
                put("-2 hit points");
                put("");
                put_trait(Trait::stout_spirit);
                break;

        case Bg::rogue:
                put("Shock received passively over time is reduced by 25%");
                put("");
                put("+10% chance to spot hidden monsters, doors, and traps");
                put("");
                put("Remains aware of the presence of other creatures longer");
                put("");
                put("Can sense the presence of unique monsters or powerful "
                    "artifacts");
                put("");
                put("Has acquired an artifact which can cloud the minds of all "
                    "enemies, causing them to forget the presence of the user");
                put("");
                put_trait(Trait::stealthy);
                break;

        case Bg::war_vet:
                put("Switches to prepared weapon instantly");
                put("");
                put("Starts with a Flak Jacket");
                put("");
                put("Maintains armor twice as long before it breaks");
                put("");
                put_trait(Trait::adept_marksman);
                put("");
                put_trait(Trait::adept_melee);
                put("");
                put_trait(Trait::tough);
                put("");
                put_trait(Trait::healer);
                break;

        case Bg::END:
                ASSERT(false);
                break;
        }

        return descr;
}

std::string occultist_domain_descr(const OccultistDomain domain)
{
        switch (domain) {
        case OccultistDomain::clairvoyant:
                return "Specializes in detection and learning. "
                       "Has an intrinsic ability to detect doors, traps, "
                       "stairs, and other locations of interest in the "
                       "surrounding area. At character level 4, this ability "
                       "also reveals items, and at level 8 it reveals "
                       "creatures";

        case OccultistDomain::enchanter:
                return "Specializes in aiding, debilitating, entrancing, and "
                       "beguiling";

        case OccultistDomain::invoker:
                return "Specializes in channeling destructive powers";

        case OccultistDomain::transmuter:
                return "Specializes in manipulating matter, energy, and time";

        case OccultistDomain::END:
                ASSERT(false);
                break;
        }

        return "";
}

std::string trait_descr(const Trait id)
{
        auto descr_for_spell_trait =
                [](const SpellId spell_id, const SpellSkill skill) {
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

                        return str;
                };

        switch (id) {
        case Trait::adept_melee:
        case Trait::expert_melee:
        case Trait::master_melee:
                return "+10% hit chance and +1 damage with melee attacks";

        case Trait::adept_marksman:
        case Trait::expert_marksman:
        case Trait::master_marksman:
                return "+10% hit chance with firearms and thrown weapons";

        case Trait::steady_aimer:
                return "Standing still gives ranged attacks maximum damage "
                       "and +10% hit chance on the following turn, unless "
                       "damage is taken";

        case Trait::cool_headed:
        case Trait::courageous:
                return "+20% shock resistance";

        case Trait::absorb:
                return "1-6 spirit points are restored each time a spell is "
                       "resisted by spell resistance (granted by spirit "
                       "traits, or the Spell Shield spell)";

        case Trait::tough:
        case Trait::rugged:
                return "+4 hit points, less likely to sprain when kicking, "
                       "more likely to succeed with object interactions "
                       "requiring strength (e.g. bashing things open)";

        case Trait::thick_skinned:
                return "+1 armor point (physical damage reduced by 1 point)";

        case Trait::resistant:
                return "Halved damage from fire and electricity (at least 1 "
                       "damage is taken)";

        case Trait::strong_backed:
                return "+50% carry weight limit";

        case Trait::dexterous:
        case Trait::lithe:
                return "+25% chance to evade attacks";

        case Trait::crippling_strikes:
                return "Your melee attacks have 60% chance to Weaken the "
                       "target creature for 2-3 turns (reducing their "
                       "melee damage by half)";

        case Trait::fearless:
                return "You cannot become terrified, +10% shock resistance";

        case Trait::healer:
                return "Using medical equipment requires only half the "
                       "normal time and resources";

        case Trait::vigilant:
                return "You are always aware of creatures within three steps "
                       "away, even if they are invisible, around the corner, "
                       "or behind a door, etc";

        case Trait::rapid_recoverer:
                return "You regenerate 1 hit point every second turn";

        case Trait::survivalist:
                return "You cannot become diseased, wounds do not affect "
                       "your combat abilities, and their negative effect on "
                       "hit points and regeneration is halved";

        case Trait::self_aware:
                return "You cannot become confused, the number of remaining "
                       "turns for status effects are displayed";

        case Trait::stout_spirit:
                return "+2 spirit points, increased spirit regeneration rate, "
                       "you can defy harmful spells (it takes 125-150 turns "
                       "to regain spell resistance after a spell is blocked)";

        case Trait::strong_spirit:
                return "+2 spirit points, increased spirit regeneration rate, "
                       "it takes 75-100 turns to regain spell resistance "
                       "after a spell is blocked";

        case Trait::mighty_spirit:
                return "+2 spirit points, increased spirit regeneration rate, "
                       "it takes 25-50 turns to regain spell resistance "
                       "after a spell is blocked";

        case Trait::stealthy:
                return "+45% chance to avoid detection by sight";

        case Trait::imperceptible:
                return "+45% chance to avoid detection by sight";

        case Trait::silent:
                return "All your melee attacks are silent, and creatures are "
                       "not alerted when you handle doors, wade, or swim";

        case Trait::vicious:
                return "+150% backstab damage (in addition to the normal +50%)";

        case Trait::ruthless:
                return "+150% backstab damage";

        case Trait::treasure_hunter:
                return "You tend to find more items";

        case Trait::undead_bane:
                return "+2 melee and ranged attack damage against all undead "
                       "monsters, +50% hit chance against ethereal undead "
                       "monsters (Ghosts)";

        case Trait::elec_incl:
                return "Rods recharge twice as fast, strange devices are less "
                       "likely to malfunction or break, electric lanterns "
                       "last twice as long, +1 damage with electricity weapons";

        case Trait::ravenous:
                return "You occasionally feed on living victims when "
                       "attacking with claws";

        case Trait::foul:
                return "+1 claw damage, when attacking with claws, vicious "
                       "worms occasionally burst out from the corpses of your "
                       "victims to attack your enemies";

        case Trait::toxic:
                return "+1 claw damage, you are immune to poison, and attacks "
                       "with your claws often poisons your victims";

        case Trait::indomitable_fury:
                return "While frenzied, you are immune to wounds, and your "
                       "attacks cause fear";

        case Trait::cast_bless_i:
                return (
                        descr_for_spell_trait(
                                SpellId::bless,
                                SpellSkill::basic));

        case Trait::cast_bless_ii:
                return (
                        descr_for_spell_trait(
                                SpellId::bless,
                                SpellSkill::expert));

        case Trait::cast_cleansing_fire_i:
                return (
                        descr_for_spell_trait(
                                SpellId::cleansing_fire,
                                SpellSkill::basic));

        case Trait::cast_cleansing_fire_ii:
                return (
                        descr_for_spell_trait(
                                SpellId::cleansing_fire,
                                SpellSkill::expert));

        case Trait::cast_heal_i:
                return (
                        descr_for_spell_trait(
                                SpellId::heal,
                                SpellSkill::basic));

        case Trait::cast_heal_ii:
                return (
                        descr_for_spell_trait(
                                SpellId::heal,
                                SpellSkill::expert));

        case Trait::cast_light_i:
                return (
                        descr_for_spell_trait(
                                SpellId::light,
                                SpellSkill::basic));

        case Trait::cast_light_ii:
                return (
                        descr_for_spell_trait(
                                SpellId::light,
                                SpellSkill::expert));

        case Trait::cast_sanctuary_i:
                return (
                        descr_for_spell_trait(
                                SpellId::sanctuary,
                                SpellSkill::basic));

        case Trait::cast_sanctuary_ii:
                return (
                        descr_for_spell_trait(
                                SpellId::sanctuary,
                                SpellSkill::expert));

        case Trait::cast_see_invisible_i:
                return (
                        descr_for_spell_trait(
                                SpellId::see_invis,
                                SpellSkill::basic));

        case Trait::cast_see_invisible_ii:
                return (
                        descr_for_spell_trait(
                                SpellId::see_invis,
                                SpellSkill::expert));

        case Trait::cast_purge:
                return (
                        descr_for_spell_trait(
                                SpellId::purge,
                                SpellSkill::basic));

        case Trait::prolonged_life:
                return "Any fatal damage received is instead drained fom your "
                       "spirit points";

        case Trait::END:
                break;
        }

        ASSERT(false);

        return "";
}

void trait_prereqs(
        const Trait trait,
        const Bg bg,
        const OccultistDomain occultist_domain,
        std::vector<Trait>& traits_out,
        Bg& bg_out)
{
        traits_out.clear();

        bg_out = Bg::END;

        switch (trait) {
        case Trait::adept_melee:
                break;

        case Trait::expert_melee:
                traits_out.push_back(Trait::adept_melee);
                break;

        case Trait::master_melee:
                traits_out.push_back(Trait::expert_melee);
                break;

        case Trait::adept_marksman:
                break;

        case Trait::expert_marksman:
                traits_out.push_back(Trait::adept_marksman);
                break;

        case Trait::master_marksman:
                traits_out.push_back(Trait::expert_marksman);
                break;

        case Trait::steady_aimer:
                bg_out = Bg::war_vet;
                break;

        case Trait::cool_headed:
                break;

        case Trait::courageous:
                traits_out.push_back(Trait::cool_headed);
                break;

        case Trait::absorb:
                traits_out.push_back(Trait::strong_spirit);
                break;

        case Trait::tough:
                break;

        case Trait::rugged:
                traits_out.push_back(Trait::tough);
                break;

        case Trait::resistant:
                traits_out.push_back(Trait::tough);
                break;

        case Trait::thick_skinned:
                traits_out.push_back(Trait::tough);
                break;

        case Trait::strong_backed:
                traits_out.push_back(Trait::tough);
                break;

        case Trait::dexterous:
                break;

        case Trait::lithe:
                traits_out.push_back(Trait::dexterous);
                break;

        case Trait::crippling_strikes:
                traits_out.push_back(Trait::dexterous);
                traits_out.push_back(Trait::adept_melee);
                bg_out = Bg::rogue;
                break;

        case Trait::fearless:
                traits_out.push_back(Trait::cool_headed);
                break;

        case Trait::healer:
                break;

        case Trait::vigilant:
                break;

        case Trait::rapid_recoverer:
                traits_out.push_back(Trait::tough);
                traits_out.push_back(Trait::healer);
                break;

        case Trait::survivalist:
                traits_out.push_back(Trait::healer);
                break;

        case Trait::self_aware:
                traits_out.push_back(Trait::stout_spirit);
                traits_out.push_back(Trait::cool_headed);
                break;

        case Trait::stout_spirit:
                break;

        case Trait::strong_spirit:
                traits_out.push_back(Trait::stout_spirit);
                break;

        case Trait::mighty_spirit:
                traits_out.push_back(Trait::strong_spirit);
                break;

        case Trait::stealthy:
                break;

        case Trait::imperceptible:
                traits_out.push_back(Trait::stealthy);
                bg_out = Bg::rogue;
                break;

        case Trait::silent:
                traits_out.push_back(Trait::stealthy);
                break;

        case Trait::vicious:
                traits_out.push_back(Trait::stealthy);
                traits_out.push_back(Trait::dexterous);
                bg_out = Bg::rogue;
                break;

        case Trait::ruthless:
                traits_out.push_back(Trait::vicious);
                bg_out = Bg::rogue;
                break;

        case Trait::treasure_hunter:
                break;

        case Trait::undead_bane:
                traits_out.push_back(Trait::tough);
                traits_out.push_back(Trait::fearless);
                traits_out.push_back(Trait::stout_spirit);
                break;

        case Trait::elec_incl:
                break;

        case Trait::ravenous:
                traits_out.push_back(Trait::adept_melee);
                bg_out = Bg::ghoul;
                break;

        case Trait::foul:
                bg_out = Bg::ghoul;
                break;

        case Trait::toxic:
                traits_out.push_back(Trait::foul);
                bg_out = Bg::ghoul;
                break;

        case Trait::indomitable_fury:
                traits_out.push_back(Trait::adept_melee);
                traits_out.push_back(Trait::tough);
                bg_out = Bg::ghoul;
                break;

        case Trait::cast_bless_i:
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_bless_ii:
                traits_out.push_back(Trait::cast_bless_i);
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_cleansing_fire_i:
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_cleansing_fire_ii:
                traits_out.push_back(Trait::cast_cleansing_fire_i);
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_heal_i:
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_heal_ii:
                traits_out.push_back(Trait::cast_heal_i);
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_light_i:
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_light_ii:
                traits_out.push_back(Trait::cast_light_i);
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_sanctuary_i:
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_sanctuary_ii:
                traits_out.push_back(Trait::cast_sanctuary_i);
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_see_invisible_i:
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_see_invisible_ii:
                traits_out.push_back(Trait::cast_see_invisible_i);
                bg_out = Bg::exorcist;
                break;

        case Trait::cast_purge:
                bg_out = Bg::exorcist;
                break;

        case Trait::prolonged_life:
                bg_out = Bg::exorcist;
                break;

        case Trait::END:
                break;
        }

        // Remove traits which are blocked for this background (prerequisites
        // are considered fulfilled)
        for (auto it = std::begin(traits_out); it != std::end(traits_out);) {
                if (is_trait_blocked_for_bg(*it, bg, occultist_domain)) {
                        it = traits_out.erase(it);
                } else {
                        // Not blocked
                        ++it;
                }
        }

        // Sort lexicographically
        std::sort(
                std::begin(traits_out),
                std::end(traits_out),
                [](const Trait& t1, const Trait& t2) {
                        const std::string str1 = trait_title(t1);
                        const std::string str2 = trait_title(t2);
                        return str1 < str2;
                });
}

Bg bg()
{
        return s_current_bg;
}

OccultistDomain occultist_domain()
{
        return s_current_occultist_domain;
}

bool is_bg(Bg bg)
{
        ASSERT(bg != Bg::END);

        return bg == s_current_bg;
}

bool has_trait(const Trait id)
{
        ASSERT(id != Trait::END);

        return s_traits[(size_t)id];
}

std::vector<Bg> pickable_bgs()
{
        std::vector<Bg> result;

        result.reserve((int)Bg::END);
        for (int i = 0; i < (int)Bg::END; ++i) {
                result.push_back((Bg)i);
        }

        // Sort lexicographically
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

        // Sort lexicographically
        std::sort(
                std::begin(result),
                std::end(result),
                [](
                        const OccultistDomain domain_1,
                        const OccultistDomain domain_2) {
                        const std::string str1 = spell_domain_title(domain_1);
                        const std::string str2 = spell_domain_title(domain_2);
                        return str1 < str2;
                });

        return result;
}

void unpicked_traits_for_bg(
        const Bg bg,
        const OccultistDomain occultist_domain,
        std::vector<Trait>& traits_can_be_picked_out,
        std::vector<Trait>& traits_prereqs_not_met_out)
{
        for (size_t i = 0; i < (size_t)Trait::END; ++i) {
                // Already picked?
                if (s_traits[i]) {
                        continue;
                }

                const auto trait = (Trait)i;

                // Check if trait is explicitly blocked for this background
                const bool is_blocked_for_bg =
                        is_trait_blocked_for_bg(
                                trait,
                                bg,
                                occultist_domain);

                if (is_blocked_for_bg) {
                        continue;
                }

                // Check trait prerequisites (traits and background)

                std::vector<Trait> trait_prereq_list;

                Bg bg_prereq = Bg::END;

                // NOTE: Traits blocked for the current background are not
                // considered prerequisites
                trait_prereqs(
                        trait,
                        bg,
                        occultist_domain,
                        trait_prereq_list,
                        bg_prereq);

                const bool is_bg_ok =
                        (s_current_bg == bg_prereq) ||
                        (bg_prereq == Bg::END);

                if (!is_bg_ok) {
                        continue;
                }

                bool is_trait_prereqs_ok = true;

                for (const auto& prereq : trait_prereq_list) {
                        if (!s_traits[(size_t)prereq]) {
                                is_trait_prereqs_ok = false;

                                break;
                        }
                }

                if (is_trait_prereqs_ok) {
                        traits_can_be_picked_out.push_back(trait);
                } else {
                        // Prerequisites not met
                        traits_prereqs_not_met_out.push_back(trait);
                }

        } // Trait loop

        // Sort lexicographically
        std::sort(
                std::begin(traits_can_be_picked_out),
                std::end(traits_can_be_picked_out),
                [](const Trait& t1, const Trait& t2) {
                        const std::string str1 = trait_title(t1);
                        const std::string str2 = trait_title(t2);
                        return str1 < str2;
                });

        std::sort(
                std::begin(traits_prereqs_not_met_out),
                std::end(traits_prereqs_not_met_out),
                [](const Trait& t1, const Trait& t2) {
                        const std::string str1 = trait_title(t1);
                        const std::string str2 = trait_title(t2);
                        return str1 < str2;
                });
}

void pick_bg(const Bg bg)
{
        ASSERT(bg != Bg::END);

        s_current_bg = bg;

        switch (s_current_bg) {
        case Bg::exorcist: {
                pick_trait(Trait::stout_spirit);
                pick_trait(Trait::undead_bane);

                // Mark all scrolls as found, so they do not yield XP
                for (auto& d : item::g_data) {
                        if (d.type == ItemType::scroll) {
                                d.is_found = true;
                        }
                }
        } break;

        case Bg::ghoul: {
                auto prop_r_disease = new PropRDisease();

                prop_r_disease->set_indefinite();

                map::g_player->m_properties.apply(
                        prop_r_disease,
                        PropSrc::intr,
                        true,
                        Verbose::no);

                auto prop_darkvis = property_factory::make(PropId::darkvision);

                prop_darkvis->set_indefinite();

                map::g_player->m_properties.apply(
                        prop_darkvis,
                        PropSrc::intr,
                        true,
                        Verbose::no);

                player_spells::learn_spell(SpellId::frenzy, Verbose::no);

                map::g_player->change_max_hp(6, Verbose::no);
        } break;

        case Bg::occultist: {
                pick_trait(Trait::stout_spirit);

                map::g_player->change_max_sp(3, Verbose::no);

                map::g_player->change_max_hp(-2, Verbose::no);
        } break;

        case Bg::rogue: {
                pick_trait(Trait::stealthy);
        } break;

        case Bg::war_vet: {
                pick_trait(Trait::adept_marksman);
                pick_trait(Trait::adept_melee);
                pick_trait(Trait::tough);
                pick_trait(Trait::healer);
        } break;

        case Bg::END:
                break;
        }
}

void pick_occultist_domain(const OccultistDomain domain)
{
        ASSERT(domain != OccultistDomain::END);

        s_current_occultist_domain = domain;

        switch (domain) {
        case OccultistDomain::clairvoyant: {
                auto prop =
                        static_cast<PropMagicSearching*>(
                                property_factory::make(
                                        PropId::magic_searching));

                prop->set_indefinite();

                prop->set_range(g_fov_radi_int);

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no);
        } break;

        case OccultistDomain::enchanter: {
        } break;

        case OccultistDomain::invoker: {
        } break;

        case OccultistDomain::transmuter: {
        } break;

        case OccultistDomain::END: {
                ASSERT(false);
        } break;
        }
}

void on_player_gained_lvl(const int new_lvl)
{
        switch (s_current_bg) {
        case Bg::exorcist: {
                const bool is_exorcist_extra_trait =
                        (new_lvl == s_exorcist_bon_trait_lvl_1) ||
                        (new_lvl == s_exorcist_bon_trait_lvl_2) ||
                        (new_lvl == s_exorcist_bon_trait_lvl_3);

                if (is_exorcist_extra_trait) {
                        states::push(
                                std::make_unique<PickTraitState>(
                                        "You gain an extra trait!"));
                }
        } break;

        case Bg::ghoul: {
        } break;

        case Bg::occultist: {
                const bool is_occultist_spell_incr_lvl =
                        (new_lvl == s_occultist_upgrade_lvl_1) ||
                        (new_lvl == s_occultist_upgrade_lvl_2);

                if (is_occultist_spell_incr_lvl) {
                        incr_occultist_spells();
                }

                switch (s_current_occultist_domain) {
                case OccultistDomain::clairvoyant: {
                        auto* const prop =
                                map::g_player->m_properties.prop(
                                        PropId::magic_searching);

                        ASSERT(prop);

                        auto* const searching =
                                static_cast<PropMagicSearching*>(prop);

                        if (new_lvl == s_occultist_upgrade_lvl_1) {
                                searching->set_allow_reveal_items();

                        } else if (new_lvl == s_occultist_upgrade_lvl_2) {
                                searching->set_allow_reveal_creatures();
                        }
                } break;

                case OccultistDomain::enchanter: {
                } break;

                case OccultistDomain::invoker: {
                } break;

                case OccultistDomain::transmuter: {
                } break;

                case OccultistDomain::END: {
                        ASSERT(false);
                } break;
                }
        } break;

        case Bg::rogue: {
        } break;

        case Bg::war_vet: {
        } break;

        case Bg::END: {
                ASSERT(false);
        } break;
        }
}

void set_all_traits_to_picked()
{
        for (int i = 0; i < (int)Trait::END; ++i) {
                s_traits[i] = true;
        }
}

void pick_trait(const Trait id)
{
        ASSERT(id != Trait::END);

        s_traits[(size_t)id] = true;

        {
                TraitLogEntry trait_log_entry;

                trait_log_entry.trait_id = id;
                trait_log_entry.clvl_picked = game::clvl();

                s_trait_log.push_back(trait_log_entry);
        }

        switch (id) {
        case Trait::tough:
        case Trait::rugged: {
                const int hp_incr = 4;

                map::g_player->change_max_hp(
                        hp_incr,
                        Verbose::no);

                map::g_player->restore_hp(
                        hp_incr,
                        false, // Not allowed above max
                        Verbose::no);
        } break;

        case Trait::stout_spirit: {
                auto prop = property_factory::make(PropId::r_spell);

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no);
        }
        // Fallthrough
        case Trait::strong_spirit:
        case Trait::mighty_spirit: {
                const int spi_incr = 2;

                map::g_player->change_max_sp(
                        spi_incr,
                        Verbose::no);

                map::g_player->restore_sp(
                        spi_incr,
                        false, // Not allowed above max
                        Verbose::no);
        } break;

        case Trait::self_aware: {
                auto prop = new PropRConf();

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no);
        } break;

        case Trait::survivalist: {
                auto prop = new PropRDisease();

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no);
        } break;

        case Trait::fearless: {
                auto prop = new PropRFear();

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no);
        } break;

        case Trait::toxic: {
                auto prop = new PropRPoison();

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no);
        } break;

        case Trait::cast_bless_i:
                player_spells::learn_spell(
                        SpellId::bless,
                        Verbose::no);
                break;

        case Trait::cast_bless_ii:
                player_spells::incr_spell_skill(
                        SpellId::bless);
                break;

        case Trait::cast_cleansing_fire_i:
                player_spells::learn_spell(
                        SpellId::cleansing_fire,
                        Verbose::no);
                break;

        case Trait::cast_cleansing_fire_ii:
                player_spells::incr_spell_skill(
                        SpellId::cleansing_fire);
                break;

        case Trait::cast_heal_i:
                player_spells::learn_spell(
                        SpellId::heal,
                        Verbose::no);
                break;

        case Trait::cast_heal_ii:
                player_spells::incr_spell_skill(
                        SpellId::heal);
                break;

        case Trait::cast_light_i:
                player_spells::learn_spell(
                        SpellId::light,
                        Verbose::no);
                break;

        case Trait::cast_light_ii:
                player_spells::incr_spell_skill(
                        SpellId::light);
                break;

        case Trait::cast_sanctuary_i:
                player_spells::learn_spell(
                        SpellId::sanctuary,
                        Verbose::no);
                break;

        case Trait::cast_sanctuary_ii:
                player_spells::incr_spell_skill(
                        SpellId::sanctuary);
                break;

        case Trait::cast_see_invisible_i:
                player_spells::learn_spell(
                        SpellId::see_invis,
                        Verbose::no);
                break;

        case Trait::cast_see_invisible_ii:
                player_spells::incr_spell_skill(
                        SpellId::see_invis);
                break;

        case Trait::cast_purge:
                player_spells::learn_spell(
                        SpellId::purge,
                        Verbose::no);
                break;

        default:
                break;
        }
}

std::vector<TraitLogEntry> trait_log()
{
        return s_trait_log;
}

bool gets_undead_bane_bon(const actor::ActorData& actor_data)
{
        return s_traits[(size_t)Trait::undead_bane] &&
                actor_data.is_undead;
}

} // namespace player_bon
