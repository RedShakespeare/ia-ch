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
#include "property_handler.hpp"
#include "saving.hpp"
#include "spells.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static Bg current_bg = Bg::END;

static OccultistDomain current_occultist_domain = OccultistDomain::END;

static bool is_trait_blocked_for_bg(const Trait trait, const Bg bg)
{
        switch (trait)
        {
        case Trait::adept_melee:
        case Trait::expert_melee:
        case Trait::master_melee:
        case Trait::cool_headed:
        case Trait::courageous:
        case Trait::ravenous:
        case Trait::foul:
        case Trait::toxic:
        case Trait::indomitable_fury:
        case Trait::vigilant:
        case Trait::self_aware:
        case Trait::stout_spirit:
        case Trait::strong_spirit:
        case Trait::mighty_spirit:
        case Trait::stealthy:
        case Trait::imperceptible:
        case Trait::silent:
        case Trait::vicious:
        case Trait::treasure_hunter:
        case Trait::undead_bane:
        case Trait::absorb:
        case Trait::tough:
        case Trait::rugged:
        case Trait::thick_skinned:
        case Trait::resistant:
        case Trait::strong_backed:
        case Trait::dexterous:
        case Trait::lithe:
        case Trait::fearless:
        case Trait::steady_aimer:
                break;

        case Trait::adept_marksman:
        case Trait::expert_marksman:
        case Trait::master_marksman:
                return bg == Bg::ghoul;

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

        case Trait::END:
                break;
        }

        return false;
}

namespace player_bon
{

bool traits[(size_t)Trait::END];

void init()
{
        current_bg = Bg::END;

        current_occultist_domain = OccultistDomain::END;

        for (size_t i = 0; i < (size_t)Trait::END; ++i)
        {
                traits[i] = false;
        }
}

void save()
{
        saving::put_int((int)current_bg);

        saving::put_int((int)current_occultist_domain);

        for (size_t i = 0; i < (size_t)Trait::END; ++i)
        {
                saving::put_bool(traits[i]);
        }
}

void load()
{
        current_bg = (Bg)saving::get_int();

        current_occultist_domain = (OccultistDomain)saving::get_int();

        for (size_t i = 0; i < (size_t)Trait::END; ++i)
        {
                traits[i] = saving::get_bool();
        }
}

std::string bg_title(const Bg id)
{
        switch (id)
        {
        case Bg::occultist:
                return "Occultist";

        case Bg::ghoul:
                return "Ghoul";

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
        switch (domain)
        {
        case OccultistDomain::clairvoyant:
                return "Clairvoyance";

        case OccultistDomain::enchanter:
                return "Enchantment";

        case OccultistDomain::invoker:
                return "Invocation";

        // case OccultistDomain::summoner:
        //         return "Summoning";

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
        switch (domain)
        {
        case OccultistDomain::clairvoyant:
                return "Clairvoyant";

        case OccultistDomain::enchanter:
                return "Enchanter";

        case OccultistDomain::invoker:
                return "Invoker";

        // case OccultistDomain::summoner:
        //         return "Summoner";

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
        switch (id)
        {
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
                descr.push_back({str, colors::white()});
        };

        auto put_trait = [&descr](const Trait id) {
                descr.push_back({trait_title(id), colors::white()});
                descr.push_back({trait_descr(id), colors::gray()});
        };

        switch (id)
        {
        case Bg::occultist:
                put("Specializes in a spell domain (selected at character "
                    "creation). "
                    "At character levels 4 and 8, all spells belonging to the "
                    "chosen domain are cast with greater power. "
                    "This choice also determines starting spells.");
                put("");
                put("-50% shock taken from casting spells, and from carrying, "
                    "using or identifying strange items (e.g. drinking a "
                    "potion or carrying a disturbing artifact)");
                put("");
                put("Can dispel magic traps");
                put("");
                put("+2 Spirit Points (in addition to \"Stout Spirit\")");
                put("");
                put("-2 Hit Points");
                put("");
                put_trait(Trait::stout_spirit);
                break;

        case Bg::ghoul:
                put("Does not regenerate Hit Points and cannot use medical "
                    "equipment - heals by feeding on corpses (feeding is done "
                    "while waiting on a corpse)");
                put("");
                put("Can incite Frenzy at will, and does not become Weakened "
                    "when Frenzy ends");
                put("");
                put("+6 Hit Points");
                put("");
                put("Is immune to Disease and Infections");
                put("");
                put("Does not get sprains");
                put("");
                put("Can see in darkness");
                put("");
                put("-50% shock taken from seeing monsters");
                put("");
                put("-15% hit chance with firearms and thrown weapons");
                put("");
                put("All Ghouls are allied");
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
        switch (domain)
        {
        case OccultistDomain::clairvoyant:
                return
                        "Specialize in detection and learning.";

        case OccultistDomain::enchanter:
                return
                        "Specialize in aiding, debilitating, entrancing, and "
                        "beguiling.";

        case OccultistDomain::invoker:
                return
                        "Specialize in channeling destructive powers.";

        case OccultistDomain::transmuter:
                return
                        "Specialize in manipulating matter, energy, and time.";

        case OccultistDomain::END:
                ASSERT(false);
                break;
        }

        return "";
}

std::string trait_descr(const Trait id)
{
        switch (id)
        {
        case Trait::adept_melee:
        case Trait::expert_melee:
        case Trait::master_melee:
                return
                        "+10% hit chance and +1 damage with melee attacks";

        case Trait::adept_marksman:
        case Trait::expert_marksman:
        case Trait::master_marksman:
                return
                        "+10% hit chance with firearms and thrown weapons";

        case Trait::steady_aimer:
                return
                        "Standing still gives ranged attacks maximum damage "
                        "and +10% hit chance on the following turn, unless "
                        "damage is taken";

        case Trait::cool_headed:
        case Trait::courageous:
                return
                        "+20% shock resistance";

        case Trait::absorb:
                return
                        "1-6 Spirit Points are restored each time a spell is "
                        "resisted by Spell Resistance (granted by Spirit "
                        "traits, or the Spell Shield spell)";

        case Trait::tough:
        case Trait::rugged:
                return
                        "+4 Hit Points, less likely to sprain when kicking, "
                        "more likely to succeed with object interactions "
                        "requiring strength (e.g. bashing things open)";

        case Trait::thick_skinned:
                return
                        "+1 Armor Point (physical damage reduced by 1 point)";

        case Trait::resistant:
                return
                        "Halved damage from fire and electricity (at least 1 "
                        "damage taken)";

        case Trait::strong_backed:
                return
                        "+50% carry weight limit";

        case Trait::dexterous:
        case Trait::lithe:
                return
                        "+25% chance to evade attacks";

        case Trait::fearless:
                return
                        "You cannot become terrified, +10% shock resistance";

        case Trait::healer:
                return
                        "Using medical equipment requires only half the "
                        "normal time and resources";

        case Trait::vigilant:
                return
                        "You are always aware of creatures within three steps "
                        "away, even if they are invisible, around the corner, "
                        "or behind a door, etc";

        case Trait::rapid_recoverer:
                return
                        "You regenerate 1 Hit Point every second turn";

        case Trait::survivalist:
                return
                        "You cannot become diseased, wounds do not affect "
                        "your combat abilities, and their negative effect on "
                        "Hit Points and regeneration is halved";

        case Trait::self_aware:
                return
                        "You cannot become confused, the number of remaining "
                        "turns for status effects are displayed";

        case Trait::stout_spirit:
                return
                        "+2 Spirit Points, increased Spirit regeneration rate, "
                        "you can defy harmful spells (it takes 125-150 turns "
                        "to regain spell resistance after a spell is blocked)";

        case Trait::strong_spirit:
                return
                        "+2 Spirit Points, increased Spirit regeneration rate, "
                        "it takes 75-100 turns to regain spell resistance "
                        "after a spell is blocked";

        case Trait::mighty_spirit:
                return
                        "+2 Spirit Points, increased Spirit regeneration rate, "
                        "it takes 25-50 turns to regain spell resistance "
                        "after a spell is blocked";

        case Trait::stealthy:
                return
                        "You are more likely to avoid detection";

        case Trait::imperceptible:
                return
                        "You are more likely to avoid detection";

        case Trait::silent:
                return
                        "All your melee attacks are silent, and creatures are "
                        "not alerted when you open or close doors, wade, or "
                        "swim ";

        case Trait::vicious:
                return
                        "+150% backstab damage (in addition to the normal +50% "
                        "damage from stealth attacks)";

        case Trait::treasure_hunter:
                return
                        "You tend to find more items";

        case Trait::undead_bane:
                return
                        "+2 melee and ranged attack damage against undead "
                        "monsters. Attacks against ethereal undead monsters "
                        "(e.g. Ghosts) never pass through them (although you "
                        "can still miss in the ordinary way)";

        case Trait::elec_incl:
                return
                        "Rods recharge twice as fast, Strange Devices are less "
                        "likely to malfunction or break, Electric Lanterns "
                        "last twice as long, +1 damage with electricity "
                        "weapons";

        case Trait::ravenous:
                return
                        "You occasionally feed on living victims when "
                        "attacking with claws";

        case Trait::foul:
                return
                        "+1 claw damage, when attacking with claws, vicious "
                        "worms occasionally burst out from the corpses of your "
                        "victims to attack your enemies";

        case Trait::toxic:
                return
                        "+1 claw damage, you are immune to poison, and attacks "
                        "with your claws occasionally poisons your victims";

        case Trait::indomitable_fury:
                return
                        "While Frenzied, you are immune to Wounds, and your "
                        "attacks cause fear";

        case Trait::END:
                break;
        }

        ASSERT(false);

        return "";
}

void trait_prereqs(
        const Trait trait,
        const Bg bg,
        std::vector<Trait>& traits_out,
        Bg& bg_out,
        int& clvl_out)
{
        traits_out.clear();

        bg_out = Bg::END;

        clvl_out = -1;

        switch (trait)
        {
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
                traits_out.push_back(Trait::dexterous);
                traits_out.push_back(Trait::stealthy);
                break;

        case Trait::vicious:
                traits_out.push_back(Trait::stealthy);
                traits_out.push_back(Trait::dexterous);
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

        case Trait::END:
                break;
        }

        // Remove traits which are blocked for this background (prerequisites
        // are considered fulfilled)
        for (auto it = begin(traits_out); it != end(traits_out); )
        {
                const Trait trait = *it;

                if (is_trait_blocked_for_bg(trait, bg))
                {
                        it = traits_out.erase(it);
                }
                else // Not blocked
                {
                        ++it;
                }
        }

        // Sort lexicographically
        sort(traits_out.begin(),
             traits_out.end(),
             [](const Trait & t1, const Trait & t2)
             {
                     const std::string str1 = trait_title(t1);
                     const std::string str2 = trait_title(t2);
                     return str1 < str2;
             });
}

Bg bg()
{
        return current_bg;
}

OccultistDomain occultist_domain()
{
        return current_occultist_domain;
}

bool has_trait(const Trait id)
{
        return traits[(size_t)id];
}

std::vector<Bg> pickable_bgs()
{
        std::vector<Bg> ret;

        for (int i = 0; i < (int)Bg::END; ++i)
        {
                ret.push_back((Bg)i);
        }

        // Sort lexicographically
        sort(ret.begin(), ret.end(), [](const Bg bg1, const Bg bg2)
        {
                const std::string str1 = bg_title(bg1);
                const std::string str2 = bg_title(bg2);
                return str1 < str2;
        });

        return ret;
}

std::vector<OccultistDomain> pickable_occultist_domains()
{
        std::vector<OccultistDomain> ret;

        for (int i = 0; i < (int)OccultistDomain::END; ++i)
        {
                ret.push_back((OccultistDomain)i);
        }

        // Sort lexicographically
        sort(ret.begin(), ret.end(), [](
                const OccultistDomain domain_1,
                const OccultistDomain domain_2)
        {
                const std::string str1 = spell_domain_title(domain_1);
                const std::string str2 = spell_domain_title(domain_2);
                return str1 < str2;
        });

        return ret;
}

void unpicked_traits_for_bg(
        const Bg bg,
        std::vector<Trait>& traits_can_be_picked_out,
        std::vector<Trait>& traits_prereqs_not_met_out)
{
        for (size_t i = 0; i < (size_t)Trait::END; ++i)
        {
                // Already picked?
                if (traits[i])
                {
                        continue;
                }

                const auto trait = (Trait)i;

                // Check if trait is explicitly blocked for this background
                const bool is_blocked_for_bg =
                        is_trait_blocked_for_bg(trait, bg);

                if (is_blocked_for_bg)
                {
                        continue;
                }

                // Check trait prerequisites (traits and background)

                std::vector<Trait> trait_prereq_list;

                Bg bg_prereq = Bg::END;

                int clvl_prereq = -1;

                // NOTE: Traits blocked for the current background are not
                // considered prerequisites
                trait_prereqs(
                        trait,
                        bg,
                        trait_prereq_list,
                        bg_prereq,
                        clvl_prereq);

                const bool is_bg_ok =
                        (current_bg == bg_prereq) ||
                        (bg_prereq == Bg::END);

                if (!is_bg_ok)
                {
                        continue;
                }

                bool is_trait_prereqs_ok = true;

                for (const auto& prereq : trait_prereq_list)
                {
                        if (!traits[(size_t)prereq])
                        {
                                is_trait_prereqs_ok = false;

                                break;
                        }
                }

                is_trait_prereqs_ok =
                        is_trait_prereqs_ok &&
                        (game::clvl() >= clvl_prereq);

                if (is_trait_prereqs_ok)
                {
                        traits_can_be_picked_out.push_back(trait);
                }
                else // Prerequisites not met
                {
                        traits_prereqs_not_met_out.push_back(trait);
                }

        } // Trait loop

        // Sort lexicographically
        sort(traits_can_be_picked_out.begin(),
             traits_can_be_picked_out.end(),
             [](const Trait & t1, const Trait & t2)
             {
                     const std::string str1 = trait_title(t1);
                     const std::string str2 = trait_title(t2);
                     return str1 < str2;
             });

        sort(traits_prereqs_not_met_out.begin(),
             traits_prereqs_not_met_out.end(),
             [](const Trait & t1, const Trait & t2)
             {
                     const std::string str1 = trait_title(t1);
                     const std::string str2 = trait_title(t2);
                     return str1 < str2;
             });
}

void pick_bg(const Bg bg)
{
        ASSERT(bg != Bg::END);

        current_bg = bg;

        switch (current_bg)
        {
        case Bg::occultist:
        {
                pick_trait(Trait::stout_spirit);

                map::player->change_max_sp(2, Verbosity::silent);

                map::player->change_max_hp(-2, Verbosity::silent);
        }
        break;

        case Bg::ghoul:
        {
                auto prop_r_disease = new PropRDisease();

                prop_r_disease->set_indefinite();

                map::player->properties.apply(
                        prop_r_disease,
                        PropSrc::intr,
                        true,
                        Verbosity::silent);

                auto prop_darkvis = new PropDarkvis();

                prop_darkvis->set_indefinite();

                map::player->properties.apply(
                        prop_darkvis,
                        PropSrc::intr,
                        true,
                        Verbosity::silent);

                player_spells::learn_spell(SpellId::frenzy,
                                           Verbosity::silent);

                map::player->change_max_hp(6, Verbosity::silent);
        }
        break;

        case Bg::rogue:
        {
                pick_trait(Trait::stealthy);
        }
        break;

        case Bg::war_vet:
        {
                pick_trait(Trait::adept_marksman);
                pick_trait(Trait::adept_melee);
                pick_trait(Trait::tough);
                pick_trait(Trait::healer);
        }
        break;

        case Bg::END:
                break;
        }
}

void pick_occultist_domain(const OccultistDomain domain)
{
        ASSERT(domain != OccultistDomain::END);

        current_occultist_domain = domain;
}

void on_player_gained_lvl(const int new_lvl)
{
        if (current_bg == Bg::occultist)
        {
                const bool is_occultist_spell_incr_lvl =
                        (new_lvl == 4) ||
                        (new_lvl == 8);

                if (is_occultist_spell_incr_lvl)
                {
                        for (int spell_id = 0;
                             spell_id < (int)SpellId::END;
                             ++spell_id)
                        {
                                const std::unique_ptr<Spell> spell(
                                        spell_factory::make_spell_from_id(
                                                (SpellId)spell_id));

                                if (spell->player_can_learn() &&
                                    spell->domain() == current_occultist_domain)
                                {
                                        player_spells::incr_spell_skill(
                                                (SpellId)spell_id);
                                }
                        }
                }
        }
}

void set_all_traits_to_picked()
{
        for (int i = 0; i < (int)Trait::END; ++i)
        {
                traits[i] = true;
        }
}

void pick_trait(const Trait id)
{
        ASSERT(id != Trait::END);

        traits[(size_t)id] = true;

        switch (id)
        {
        case Trait::tough:
        case Trait::rugged:
        {
                const int hp_incr = 4;

                map::player->change_max_hp(
                        hp_incr,
                        Verbosity::silent);

                map::player->restore_hp(
                        hp_incr,
                        false, // Not allowed above max
                        Verbosity::silent);
        }
        break;

        case Trait::stout_spirit:
        {
                auto prop = new PropRSpell();

                prop->set_indefinite();

                map::player->properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbosity::silent);
        }
        // Fallthrough
        case Trait::strong_spirit:
        case Trait::mighty_spirit:
        {
                const int spi_incr = 2;

                map::player->change_max_sp(
                        spi_incr,
                        Verbosity::silent);

                map::player->restore_sp(
                        spi_incr,
                        false, // Not allowed above max
                        Verbosity::silent);
        }
        break;

        case Trait::self_aware:
        {
                auto prop = new PropRConf();

                prop->set_indefinite();

                map::player->properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbosity::silent);
        }
        break;

        case Trait::survivalist:
        {
                auto prop = new PropRDisease();

                prop->set_indefinite();

                map::player->properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbosity::silent);
        }
        break;

        case Trait::fearless:
        {
                auto prop = new PropRFear();

                prop->set_indefinite();

                map::player->properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbosity::silent);
        }
        break;

        case Trait::toxic:
        {
                auto prop = new PropRPoison();

                prop->set_indefinite();

                map::player->properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbosity::silent);
        }
        break;

        default:
                break;
        }
}

std::string all_picked_traits_titles_line()
{
        std::string out = "";

        for (int i = 0; i < (int)Trait::END; ++i)
        {
                if (traits[i])
                {
                        const std::string title = trait_title(Trait(i));

                        out += (out.empty() ? "" : ", ") + title;
                }
        }

        return out;
}

bool gets_undead_bane_bon(const ActorData& actor_data)
{
        return
                player_bon::traits[(size_t)Trait::undead_bane] &&
                actor_data.is_undead;
}

} // player_bon
