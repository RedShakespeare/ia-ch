// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "player_bon.hpp"

#include <functional>

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
struct TraitData
{
        Trait id { Trait::END };
        std::string title {};
        std::string descr {};
        std::function<void()> on_picked {};
        std::vector<Trait> trait_prereqs {};
        Bg bg_prereq { Bg::END };
        std::vector<Bg> blocked_for_bgs {};
        std::vector<OccultistDomain> blocked_for_occultist_domains {};
        bool is_picked { false };
};

static TraitData s_trait_data[ (size_t)Trait::END ];

static std::vector<player_bon::TraitLogEntry> s_trait_log;

static auto s_player_bg = Bg::END;
static auto s_player_occultist_domain = OccultistDomain::END;

static const int s_exorcist_bon_trait_lvl_1 = 2;
static const int s_exorcist_bon_trait_lvl_2 = 4;
static const int s_exorcist_bon_trait_lvl_3 = 6;

static const int s_occultist_upgrade_lvl_1 = 4;
static const int s_occultist_upgrade_lvl_2 = 8;

static std::string trait_descr_for_spell(
        const SpellId spell_id,
        const SpellSkill skill )
{
        std::unique_ptr<Spell> spell( spells::make( spell_id ) );

        std::string str =
                "Gain the ability to cast \"" +
                spell->name() +
                "\"";

        if ( spell->can_be_improved_with_skill() )
        {
                str +=
                        " at " +
                        spells::skill_to_str( skill ) +
                        " level";
        }

        str += " -";

        const auto descr = spell->descr_specific( skill );

        for ( const auto& line : descr )
        {
                str += " " + line;
        }

        return str;
}

static TraitData& trait_data( const Trait id )
{
        ASSERT( id != Trait::END );

        return s_trait_data[ (size_t)id ];
}

static void set_trait_data( TraitData& d )
{
        ASSERT( d.id != Trait::END );

        s_trait_data[ (size_t)d.id ] = d;

        d = {};
}

static void init_trait_data()
{
        for ( auto& d : s_trait_data )
        {
                d = {};
        }

        TraitData d;

        // --- Adept Melee Fighter ---
        d.id = Trait::adept_melee;
        d.title = "Adept Melee Fighter";
        d.descr = "+10% hit chance and +1 damage with melee attacks";
        set_trait_data( d );

        // --- Expert Melee Fighter ---
        d = trait_data( Trait::adept_melee );
        d.id = Trait::expert_melee;
        d.title = "Expert Melee Fighter";
        d.trait_prereqs = { Trait::adept_melee };
        d.blocked_for_bgs = { Bg::exorcist };
        set_trait_data( d );

        // --- Master Melee Fighter ---
        d = trait_data( Trait::adept_melee );
        d.id = Trait::master_melee;
        d.title = "Master Melee Fighter";
        d.trait_prereqs = { Trait::expert_melee };
        d.blocked_for_bgs = { Bg::exorcist, Bg::occultist };
        set_trait_data( d );

        // --- Adept Marksman ---
        d.id = Trait::adept_marksman;
        d.title = "Adept Marksman";
        d.descr = "+10% hit chance with firearms and thrown weapons";
        d.blocked_for_bgs = { Bg::ghoul };
        set_trait_data( d );

        // --- Expert Marksman ---
        d = trait_data( Trait::adept_marksman );
        d.id = Trait::expert_marksman;
        d.title = "Expert Marksman";
        d.trait_prereqs = { Trait::adept_marksman };
        d.blocked_for_bgs = { Bg::ghoul, Bg::exorcist };
        set_trait_data( d );

        // --- Master Marksman ---
        d = trait_data( Trait::adept_marksman );
        d.id = Trait::master_marksman;
        d.title = "Master Marksman";
        d.trait_prereqs = { Trait::expert_marksman };
        d.blocked_for_bgs = { Bg::ghoul, Bg::exorcist, Bg::occultist };
        set_trait_data( d );

        // --- Cool-headed ---
        d.id = Trait::cool_headed;
        d.title = "Cool-headed";
        d.descr = "+20% shock resistance";
        set_trait_data( d );

        // --- Courageous ---
        d = trait_data( Trait::cool_headed );
        d.id = Trait::courageous;
        d.title = "Courageous";
        d.trait_prereqs = { Trait::cool_headed };
        set_trait_data( d );

        // --- Dexterous ---
        d.id = Trait::dexterous;
        d.title = "Dexterous";
        d.descr = "+25% chance to evade attacks";
        set_trait_data( d );

        // --- Lithe ---
        d = trait_data( Trait::dexterous );
        d.id = Trait::lithe;
        d.title = "Lithe";
        d.trait_prereqs = { Trait::dexterous };
        set_trait_data( d );

        // --- Crippling Strikes ---
        d.id = Trait::crippling_strikes;
        d.title = "Crippling Strikes";
        d.descr =
                "Your melee attacks have 60% chance to weaken the target "
                "creature for 2-3 turns (reducing their melee damage by half)";
        d.trait_prereqs = { Trait::dexterous, Trait::adept_melee };
        d.bg_prereq = Bg::rogue;
        set_trait_data( d );

        // --- Fearless ---
        d.id = Trait::fearless;
        d.title = "Fearless";
        d.descr = "You cannot become terrified, +10% shock resistance";
        d.on_picked = []() {
                auto* prop = new PropRFear();

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no );
        };
        d.trait_prereqs = { Trait::cool_headed };
        set_trait_data( d );

        // --- Stealthy ---
        d.id = Trait::stealthy;
        d.title = "Stealthy";
        d.descr = "+45% chance to avoid detection by sight";
        set_trait_data( d );

        // --- Imperceptible ---
        d = trait_data( Trait::stealthy );
        d.id = Trait::imperceptible;
        d.title = "Imperceptible";
        d.trait_prereqs = { Trait::stealthy };
        d.bg_prereq = Bg::rogue;
        set_trait_data( d );

        // --- Silent ---
        d.id = Trait::silent;
        d.title = "Silent";
        d.descr =
                "All your melee attacks are silent (regardless of the weapon), "
                "and creatures are not alerted when you open or close doors, "
                "wade, or swim";
        d.trait_prereqs = { Trait::stealthy };
        set_trait_data( d );

        // --- Vigilant ---
        d.id = Trait::vigilant;
        d.title = "Vigilant";
        d.descr = "You are always aware of nearby creatures";
        d.blocked_for_occultist_domains = { OccultistDomain::clairvoyant };
        set_trait_data( d );

        // --- Treasure Hunter ---
        d.id = Trait::treasure_hunter;
        d.title = "Treasure Hunter";
        d.descr = "You tend to find more items";
        d.blocked_for_bgs = { Bg::exorcist, Bg::ghoul, Bg::war_vet };
        d.blocked_for_occultist_domains = { OccultistDomain::clairvoyant };
        set_trait_data( d );

        // --- Self-aware ---
        d.id = Trait::self_aware;
        d.title = "Self-aware";
        d.descr =
                "You cannot become confused, the number of remaining turns "
                "for status effects are displayed";
        d.on_picked = []() {
                auto* prop = new PropRConf();

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no );
        };
        d.trait_prereqs = { Trait::stout_spirit, Trait::cool_headed };
        set_trait_data( d );

        // --- Healer ---
        d.id = Trait::healer;
        d.title = "Healer";
        d.descr =
                "Using medical equipment requires only half the normal time "
                "and resources";
        d.blocked_for_bgs = { Bg::ghoul };
        set_trait_data( d );

        // --- Rapid Recoverer ---
        d.id = Trait::rapid_recoverer;
        d.title = "Rapid Recoverer";
        d.descr = "You regenerate 1 hit point every second turn";
        d.trait_prereqs = { Trait::tough, Trait::healer };
        d.blocked_for_bgs = { Bg::ghoul };
        set_trait_data( d );

        // --- Survivalist ---
        d.id = Trait::survivalist;
        d.title = "Survivalist";
        d.descr =
                "You cannot become diseased, wounds do not affect your combat "
                "abilities, and their negative effect on hit points and "
                "regeneration is halved";
        d.on_picked = []() {
                auto* prop = new PropRDisease();

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no );
        };
        d.trait_prereqs = { Trait::healer };
        d.blocked_for_bgs = { Bg::ghoul };
        set_trait_data( d );

        // --- Stout Spirit ---
        d.id = Trait::stout_spirit;
        d.title = "Stout Spirit";
        d.descr =
                "+2 spirit points, increased spirit regeneration rate, you "
                "can defy harmful spells (it takes 125-150 turns to regain "
                "spell resistance after a spell is blocked)";
        d.on_picked = []() {
                auto* prop = property_factory::make( PropId::r_spell );

                prop->set_indefinite();

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no );

                const int spi_incr = 2;

                map::g_player->change_max_sp(
                        spi_incr,
                        Verbose::no );

                map::g_player->restore_sp(
                        spi_incr,
                        false,  // Not allowed above max
                        Verbose::no );
        };
        set_trait_data( d );

        // --- Strong Spirit ---
        d = trait_data( Trait::stout_spirit );
        d.id = Trait::strong_spirit;
        d.title = "Strong Spirit";
        d.descr =
                "+2 spirit points, increased spirit regeneration rate, it "
                "takes 75-100 turns to regain spell resistance after a spell "
                "is blocked";
        d.trait_prereqs = { Trait::stout_spirit };
        set_trait_data( d );

        // --- Mighty Spirit ---
        d.id = Trait::mighty_spirit;
        d.title = "Mighty Spirit";
        d.descr =
                "+2 spirit points, increased spirit regeneration rate, it "
                "takes 25-50 turns to regain spell resistance after a spell "
                "is blocked";
        d.trait_prereqs = { Trait::strong_spirit };
        set_trait_data( d );

        // --- Absorption ---
        d.id = Trait::absorb;
        d.title = "Absorption";
        d.descr =
                "1-6 spirit points are restored each time a spell is resisted "
                "by spell resistance (granted by spirit traits, or the Spell "
                "Shield spell)";
        d.trait_prereqs = { Trait::strong_spirit };
        set_trait_data( d );

        // --- Tough ---
        d.id = Trait::tough;
        d.title = "Tough";
        d.descr =
                "+4 hit points, less likely to sprain when kicking, more "
                "likely to succeed with object interactions requiring "
                "strength (e.g. bashing things open)";
        d.on_picked = []() {
                const int hp_incr = 4;

                map::g_player->change_max_hp(
                        hp_incr,
                        Verbose::no );

                map::g_player->restore_hp(
                        hp_incr,
                        false,  // Not allowed above max
                        Verbose::no );
        };
        set_trait_data( d );

        // --- Rugged ---
        d = trait_data( Trait::tough );
        d.id = Trait::rugged;
        d.title = "Rugged";
        d.trait_prereqs = { Trait::tough };
        set_trait_data( d );

        // --- Thick Skinned ---
        d.id = Trait::thick_skinned;
        d.title = "Thick Skinned";
        d.descr = "+1 armor point (physical damage reduced by 1 point)";
        d.trait_prereqs = { Trait::tough };
        set_trait_data( d );

        // --- Resistant ---
        d.id = Trait::resistant;
        d.title = "Resistant";
        d.descr = "Halved damage from fire and electricity";
        d.trait_prereqs = { Trait::tough };
        set_trait_data( d );

        // --- Strong-backed ---
        d.id = Trait::strong_backed;
        d.title = "Strong-backed";
        d.descr = "+50% carry weight limit";
        d.trait_prereqs = { Trait::tough };
        set_trait_data( d );

        // --- Bane of the Undead ---
        d.id = Trait::undead_bane;
        d.title = "Bane of the Undead";
        d.descr =
                "+2 melee and ranged attack damage against all undead "
                "monsters, +50% hit chance against ethereal undead monsters";
        d.trait_prereqs = { Trait::tough, Trait::fearless, Trait::stout_spirit };
        set_trait_data( d );

        // --- Electrically Inclined ---
        d.id = Trait::elec_incl;
        d.title = "Electrically Inclined";
        d.descr =
                "Rods recharge twice as fast, strange devices are less likely "
                "to malfunction or break, electric lanterns last twice as "
                "long, +1 damage with electricity weapons";
        d.blocked_for_bgs = { Bg::ghoul };
        set_trait_data( d );

        // --- Cast Bless ---
        d.id = Trait::cast_bless_i;
        d.title = "Cast Bless";
        d.descr =
                trait_descr_for_spell(
                        SpellId::bless,
                        SpellSkill::basic );
        d.on_picked = []() {
                player_spells::learn_spell(
                        SpellId::bless,
                        Verbose::no );
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast Bless II ---
        d.id = Trait::cast_bless_ii;
        d.title = "Cast Bless II";
        d.descr =
                trait_descr_for_spell(
                        SpellId::bless,
                        SpellSkill::expert );
        d.on_picked = []() {
                player_spells::incr_spell_skill(
                        SpellId::bless );
        };
        d.trait_prereqs = { Trait::cast_bless_i };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast Cleansing Fire ---
        d.id = Trait::cast_cleansing_fire_i;
        d.title = "Cast Cleansing Fire";
        d.descr =
                trait_descr_for_spell(
                        SpellId::cleansing_fire,
                        SpellSkill::basic );
        d.on_picked = []() {
                player_spells::learn_spell(
                        SpellId::cleansing_fire,
                        Verbose::no );
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast Cleansing Fire II ---
        d.id = Trait::cast_cleansing_fire_ii;
        d.title = "Cast Cleansing Fire II";
        d.descr =
                trait_descr_for_spell(
                        SpellId::cleansing_fire,
                        SpellSkill::expert );
        d.on_picked = []() {
                player_spells::incr_spell_skill(
                        SpellId::cleansing_fire );
        };
        d.trait_prereqs = { Trait::cast_cleansing_fire_i };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast Heal ---
        d.id = Trait::cast_heal_i;
        d.title = "Cast Heal";
        d.descr =
                trait_descr_for_spell(
                        SpellId::heal,
                        SpellSkill::basic );
        d.on_picked = []() {
                player_spells::learn_spell(
                        SpellId::heal,
                        Verbose::no );
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast Heal II ---
        d.id = Trait::cast_heal_ii;
        d.title = "Cast Heal II";
        d.descr =
                trait_descr_for_spell(
                        SpellId::heal,
                        SpellSkill::expert );
        d.on_picked = []() {
                player_spells::incr_spell_skill(
                        SpellId::heal );
        };
        d.trait_prereqs = { Trait::cast_heal_i };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast Light ---
        d.id = Trait::cast_light_i;
        d.title = "Cast Light";
        d.descr =
                trait_descr_for_spell(
                        SpellId::light,
                        SpellSkill::basic );
        d.on_picked = []() {
                player_spells::learn_spell(
                        SpellId::light,
                        Verbose::no );
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast Light II ---
        d.id = Trait::cast_light_ii;
        d.title = "Cast Light II";
        d.descr =
                trait_descr_for_spell(
                        SpellId::light,
                        SpellSkill::expert );
        d.on_picked = []() {
                player_spells::incr_spell_skill(
                        SpellId::light );
        };
        d.trait_prereqs = { Trait::cast_light_i };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast Sanctuary ---
        d.id = Trait::cast_sanctuary_i;
        d.title = "Cast Sanctuary";
        d.descr =
                trait_descr_for_spell(
                        SpellId::sanctuary,
                        SpellSkill::basic );
        d.on_picked = []() {
                player_spells::learn_spell(
                        SpellId::sanctuary,
                        Verbose::no );
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast Sanctuary II ---
        d.id = Trait::cast_sanctuary_ii;
        d.title = "Cast Sanctuary II";
        d.descr =
                trait_descr_for_spell(
                        SpellId::sanctuary,
                        SpellSkill::expert );
        d.on_picked = []() {
                player_spells::incr_spell_skill(
                        SpellId::sanctuary );
        };
        d.trait_prereqs = { Trait::cast_sanctuary_i };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast See Invisible ---
        d.id = Trait::cast_see_invisible_i;
        d.title = "Cast See Invisible";
        d.descr =
                trait_descr_for_spell(
                        SpellId::see_invis,
                        SpellSkill::basic );
        d.on_picked = []() {
                player_spells::learn_spell(
                        SpellId::see_invis,
                        Verbose::no );
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast See Invisible II ---
        d.id = Trait::cast_see_invisible_ii;
        d.title = "Cast See Invisible II";
        d.descr =
                trait_descr_for_spell(
                        SpellId::see_invis,
                        SpellSkill::expert );
        d.on_picked = []() {
                player_spells::incr_spell_skill(
                        SpellId::see_invis );
        };
        d.trait_prereqs = { Trait::cast_see_invisible_i };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Cast Purge ---
        d.id = Trait::cast_purge;
        d.title = "Cast Purge";
        d.descr =
                trait_descr_for_spell(
                        SpellId::purge,
                        SpellSkill::basic );
        d.on_picked = []() {
                player_spells::learn_spell(
                        SpellId::purge,
                        Verbose::no );
        };
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Prolonged Life ---
        d.id = Trait::prolonged_life;
        d.title = "Prolonged Life";
        d.descr =
                "Any fatal damage received is instead drained fom your "
                "spirit points";
        d.bg_prereq = Bg::exorcist;
        set_trait_data( d );

        // --- Ravenous ---
        d.id = Trait::ravenous;
        d.title = "Ravenous";
        d.descr =
                "You occasionally feed on living victims when attacking "
                "with claws";
        d.trait_prereqs = { Trait::adept_melee };
        d.bg_prereq = Bg::ghoul;
        set_trait_data( d );

        // --- Foul ---
        d.id = Trait::foul;
        d.title = "Foul";
        d.descr =
                "+1 claw damage, when attacking with claws, vicious worms "
                "occasionally burst out from the corpses of your victims to "
                "attack your enemies";
        d.bg_prereq = Bg::ghoul;
        set_trait_data( d );

        // --- Toxic ---
        d.id = Trait::toxic;
        d.title = "Toxic";
        d.descr =
                "+1 claw damage, you are immune to poison, and attacks with "
                "your claws often poisons your victims";
        d.trait_prereqs = { Trait::foul };
        d.bg_prereq = Bg::ghoul;
        set_trait_data( d );

        // --- Indomitable Fury ---
        d.id = Trait::indomitable_fury;
        d.title = "Indomitable Fury";
        d.descr =
                "While frenzied, you are immune to wounds, and your attacks "
                "cause fear";
        d.trait_prereqs = { Trait::adept_melee, Trait::tough };
        d.bg_prereq = Bg::ghoul;
        set_trait_data( d );

        // --- Vicious ---
        d.id = Trait::vicious;
        d.title = "Vicious";
        d.descr = "+150% backstab damage (in addition to the normal +50%)";
        d.trait_prereqs = { Trait::stealthy, Trait::dexterous };
        d.bg_prereq = Bg::rogue;
        set_trait_data( d );

        // --- Ruthless ---
        d.id = Trait::ruthless;
        d.title = "Ruthless";
        d.descr = "+150% backstab damage";
        d.trait_prereqs = { Trait::vicious };
        d.bg_prereq = Bg::rogue;
        set_trait_data( d );

        // --- Steady Aimer ---
        d.id = Trait::steady_aimer;
        d.title = "Steady Aimer";
        d.descr =
                "Standing still gives ranged attacks maximum damage and +10% "
                "hit chance on the following turn, unless damage is taken";
        d.bg_prereq = Bg::war_vet;
        set_trait_data( d );
}

static bool is_trait_blocked_for_bg(
        const Trait trait,
        const Bg bg,
        const OccultistDomain occultist_domain )
{
        const auto d = trait_data( trait );

        const bool is_blocked_for_bg =
                std::find(
                        std::begin( d.blocked_for_bgs ),
                        std::end( d.blocked_for_bgs ),
                        bg ) != std::end( d.blocked_for_bgs );

        const bool is_blocked_for_domain =
                std::find(
                        std::begin( d.blocked_for_occultist_domains ),
                        std::end( d.blocked_for_occultist_domains ),
                        occultist_domain ) !=
                std::end( d.blocked_for_occultist_domains );

        return is_blocked_for_bg || is_blocked_for_domain;
}

static void incr_occultist_spells()
{
        for ( int id = 0; id < (int)SpellId::END; ++id )
        {
                const std::unique_ptr<Spell> spell( spells::make( (SpellId)id ) );

                const bool is_learnable =
                        spell->player_can_learn();

                const bool is_matching_domain =
                        ( spell->domain() == s_player_occultist_domain );

                if ( is_learnable && is_matching_domain )
                {
                        player_spells::incr_spell_skill( (SpellId)id );
                }
        }
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

        init_trait_data();

        s_trait_log.clear();
}

void save()
{
        saving::put_int( (int)s_player_bg );

        saving::put_int( (int)s_player_occultist_domain );

        for ( const auto& d : s_trait_data )
        {
                saving::put_bool( d.is_picked );
        }

        saving::put_int( s_trait_log.size() );

        for ( const auto& e : s_trait_log )
        {
                saving::put_int( e.clvl_picked );

                saving::put_int( (int)e.trait_id );
        }
}

void load()
{
        s_player_bg = (Bg)saving::get_int();

        s_player_occultist_domain = (OccultistDomain)saving::get_int();

        for ( auto& d : s_trait_data )
        {
                d.is_picked = saving::get_bool();
        }

        const int nr_trait_log_entries = saving::get_int();

        s_trait_log.resize( nr_trait_log_entries );

        for ( auto& e : s_trait_log )
        {
                e.clvl_picked = saving::get_int();

                e.trait_id = (Trait)saving::get_int();
        }
}

std::string bg_title( const Bg id )
{
        switch ( id )
        {
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

        ASSERT( false );

        return "";
}

std::string spell_domain_title( const OccultistDomain domain )
{
        switch ( domain )
        {
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

        ASSERT( false );

        return "";
}

std::string occultist_profession_title( const OccultistDomain domain )
{
        switch ( domain )
        {
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

        ASSERT( false );

        return "";
}

std::string trait_title( const Trait id )
{
        return trait_data( id ).title;
}

std::vector<ColoredString> bg_descr( const Bg id )
{
        std::vector<ColoredString> descr;

        auto put = [ &descr ]( const std::string& str ) {
                descr.emplace_back( str, colors::white() );
        };

        auto put_trait = [ &descr ]( const Trait trait_id ) {
                descr.emplace_back( trait_title( trait_id ), colors::white() );
                descr.emplace_back( trait_descr( trait_id ), colors::gray() );
        };

        switch ( id )
        {
        case Bg::exorcist:
                put( "Starts with a Holy Symbol, which can restore "
                     "spirit points and grant resistance against "
                     "shock and fear" );
                put( "" );
                put( "Cannot use manuscripts, altars, monoliths, or gongs, "
                     "but gains experience and spirit points for destroying "
                     "these (manuscripts are destroyed when picking them up)" );
                put( "" );
                put( "Spirit points gained above the maximum level can be kept "
                     "indefinitely until they are spent" );
                put( "" );
                put( "Gains a bonus trait at character levels " +
                     std::to_string( s_exorcist_bon_trait_lvl_1 ) +
                     ", " +
                     std::to_string( s_exorcist_bon_trait_lvl_2 ) +
                     ", and " +
                     std::to_string( s_exorcist_bon_trait_lvl_3 ) );
                put( "" );
                put_trait( Trait::stout_spirit );
                put( "" );
                put_trait( Trait::undead_bane );
                break;

        case Bg::ghoul:
                put( "Does not regenerate hit points and cannot use medical "
                     "equipment - heals by feeding on corpses (feeding is done "
                     "while waiting on a corpse)" );
                put( "" );
                put( "Can incite frenzy at will, and does not become weakened "
                     "when frenzy ends" );
                put( "" );
                put( "+6 hit points" );
                put( "" );
                put( "Is immune to disease and infections" );
                put( "" );
                put( "Does not get sprains" );
                put( "" );
                put( "Can see in darkness" );
                put( "" );
                put( "-50% shock taken from seeing monsters" );
                put( "" );
                put( "-15% hit chance with firearms and thrown weapons" );
                put( "" );
                put( "All ghouls are allied" );
                break;

        case Bg::occultist:
                put( "Specializes in a spell domain (selected at character "
                     "creation). At character levels " +
                     std::to_string( s_occultist_upgrade_lvl_1 ) +
                     " and " +
                     std::to_string( s_occultist_upgrade_lvl_2 ) +
                     ", all spells "
                     "belonging to the chosen domain are cast with greater "
                     "power. This choice also determines starting spells" );
                put( "" );
                put( "-50% shock taken from casting spells, and from carrying, "
                     "using or identifying strange items (e.g. drinking a "
                     "potion or carrying a disturbing artifact)" );
                put( "" );
                put( "Can dispel magic traps, doing so grants spirit points" );
                put( "" );
                put( "+3 spirit points (in addition to \"Stout Spirit\")" );
                put( "" );
                put( "-2 hit points" );
                put( "" );
                put_trait( Trait::stout_spirit );
                break;

        case Bg::rogue:
                put( "Shock received passively over time is reduced by 25%" );
                put( "" );
                put( "+10% chance to spot hidden monsters, doors, and traps" );
                put( "" );
                put( "Remains aware of the presence of other creatures longer" );
                put( "" );
                put( "Can sense the presence of unique monsters or powerful "
                     "artifacts" );
                put( "" );
                put( "Has acquired an artifact which can cloud the minds of all "
                     "enemies, causing them to forget the presence of the user" );
                put( "" );
                put_trait( Trait::stealthy );
                break;

        case Bg::war_vet:
                put( "Switches to prepared weapon instantly" );
                put( "" );
                put( "Starts with a Flak Jacket" );
                put( "" );
                put( "Maintains armor twice as long before it breaks" );
                put( "" );
                put_trait( Trait::adept_marksman );
                put( "" );
                put_trait( Trait::adept_melee );
                put( "" );
                put_trait( Trait::tough );
                put( "" );
                put_trait( Trait::healer );
                break;

        case Bg::END:
                ASSERT( false );
                break;
        }

        return descr;
}

std::string occultist_domain_descr( const OccultistDomain domain )
{
        switch ( domain )
        {
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
                ASSERT( false );
                break;
        }

        return "";
}

std::string trait_descr( const Trait id )
{
        return trait_data( id ).descr;
}

void trait_prereqs(
        const Trait trait,
        const Bg bg,
        const OccultistDomain occultist_domain,
        std::vector<Trait>& traits_out,
        Bg& bg_out )
{
        const auto& d = trait_data( trait );

        traits_out = d.trait_prereqs;
        bg_out = d.bg_prereq;

        // Remove traits which are blocked for this background (prerequisites
        // are considered fulfilled)
        for ( auto it = std::begin( traits_out ); it != std::end( traits_out ); )
        {
                if ( is_trait_blocked_for_bg( *it, bg, occultist_domain ) )
                {
                        it = traits_out.erase( it );
                }
                else
                {
                        // Not blocked
                        ++it;
                }
        }

        // Sort lexicographically
        std::sort(
                std::begin( traits_out ),
                std::end( traits_out ),
                []( const Trait& t1, const Trait& t2 ) {
                        const std::string str1 = trait_title( t1 );
                        const std::string str2 = trait_title( t2 );
                        return str1 < str2;
                } );
}

Bg bg()
{
        return s_player_bg;
}

OccultistDomain occultist_domain()
{
        return s_player_occultist_domain;
}

bool is_bg( Bg bg )
{
        ASSERT( bg != Bg::END );

        return bg == s_player_bg;
}

bool has_trait( const Trait id )
{
        return trait_data( id ).is_picked;
}

std::vector<Bg> pickable_bgs()
{
        std::vector<Bg> result;

        result.reserve( (int)Bg::END );

        for ( int i = 0; i < (int)Bg::END; ++i )
        {
                result.push_back( (Bg)i );
        }

        // Sort lexicographically
        std::sort(
                std::begin( result ),
                std::end( result ),
                []( const Bg bg1, const Bg bg2 ) {
                        const std::string str1 = bg_title( bg1 );
                        const std::string str2 = bg_title( bg2 );
                        return str1 < str2;
                } );

        return result;
}

std::vector<OccultistDomain> pickable_occultist_domains()
{
        std::vector<OccultistDomain> result;

        result.reserve( (int)OccultistDomain::END );

        for ( int i = 0; i < (int)OccultistDomain::END; ++i )
        {
                result.push_back( (OccultistDomain)i );
        }

        // Sort lexicographically
        std::sort(
                std::begin( result ),
                std::end( result ),
                [](
                        const OccultistDomain domain_1,
                        const OccultistDomain domain_2 ) {
                        const std::string str1 = spell_domain_title( domain_1 );
                        const std::string str2 = spell_domain_title( domain_2 );
                        return str1 < str2;
                } );

        return result;
}

void unpicked_traits_for_bg(
        const Bg bg,
        const OccultistDomain occultist_domain,
        std::vector<Trait>& traits_can_be_picked_out,
        std::vector<Trait>& traits_prereqs_not_met_out )
{
        for ( const auto& d : s_trait_data )
        {
                // Already picked?
                if ( d.is_picked )
                {
                        continue;
                }

                // Check if trait is explicitly blocked for this background
                const bool is_blocked_for_bg =
                        is_trait_blocked_for_bg(
                                d.id,
                                bg,
                                occultist_domain );

                if ( is_blocked_for_bg )
                {
                        continue;
                }

                // Check trait prerequisites (traits and background)

                std::vector<Trait> trait_prereq_list;

                auto bg_prereq = Bg::END;

                // NOTE: Traits blocked for the current background are not
                // considered prerequisites
                trait_prereqs(
                        d.id,
                        bg,
                        occultist_domain,
                        trait_prereq_list,
                        bg_prereq );

                const bool is_bg_ok =
                        ( s_player_bg == bg_prereq ) ||
                        ( bg_prereq == Bg::END );

                if ( ! is_bg_ok )
                {
                        continue;
                }

                bool is_trait_prereqs_ok = true;

                for ( const auto& prereq : trait_prereq_list )
                {
                        if ( ! trait_data( prereq ).is_picked )
                        {
                                is_trait_prereqs_ok = false;

                                break;
                        }
                }

                if ( is_trait_prereqs_ok )
                {
                        traits_can_be_picked_out.push_back( d.id );
                }
                else
                {
                        traits_prereqs_not_met_out.push_back( d.id );
                }

        }  // Trait loop

        // Sort lexicographically
        std::sort(
                std::begin( traits_can_be_picked_out ),
                std::end( traits_can_be_picked_out ),
                []( const Trait& t1, const Trait& t2 ) {
                        const std::string str1 = trait_title( t1 );
                        const std::string str2 = trait_title( t2 );
                        return str1 < str2;
                } );

        std::sort(
                std::begin( traits_prereqs_not_met_out ),
                std::end( traits_prereqs_not_met_out ),
                []( const Trait& t1, const Trait& t2 ) {
                        const std::string str1 = trait_title( t1 );
                        const std::string str2 = trait_title( t2 );
                        return str1 < str2;
                } );
}  // namespace player_bon

void pick_bg( const Bg bg )
{
        ASSERT( bg != Bg::END );

        s_player_bg = bg;

        switch ( s_player_bg )
        {
        case Bg::exorcist: {
                pick_trait( Trait::stout_spirit );
                pick_trait( Trait::undead_bane );

                // Mark all scrolls as found, so they do not yield XP
                for ( auto& d : item::g_data )
                {
                        if ( d.type == ItemType::scroll )
                        {
                                d.is_found = true;
                        }
                }
        }
        break;

        case Bg::ghoul: {
                auto* prop_r_disease = new PropRDisease();

                prop_r_disease->set_indefinite();

                map::g_player->m_properties.apply(
                        prop_r_disease,
                        PropSrc::intr,
                        true,
                        Verbose::no );

                auto* prop_darkvis = property_factory::make( PropId::darkvision );

                prop_darkvis->set_indefinite();

                map::g_player->m_properties.apply(
                        prop_darkvis,
                        PropSrc::intr,
                        true,
                        Verbose::no );

                player_spells::learn_spell( SpellId::frenzy, Verbose::no );

                map::g_player->change_max_hp( 6, Verbose::no );
        }
        break;

        case Bg::occultist: {
                pick_trait( Trait::stout_spirit );

                map::g_player->change_max_sp( 3, Verbose::no );

                map::g_player->change_max_hp( -2, Verbose::no );
        }
        break;

        case Bg::rogue: {
                pick_trait( Trait::stealthy );
        }
        break;

        case Bg::war_vet: {
                pick_trait( Trait::adept_marksman );
                pick_trait( Trait::adept_melee );
                pick_trait( Trait::tough );
                pick_trait( Trait::healer );
        }
        break;

        case Bg::END:
                break;
        }
}

void pick_occultist_domain( const OccultistDomain domain )
{
        ASSERT( domain != OccultistDomain::END );

        s_player_occultist_domain = domain;

        switch ( domain )
        {
        case OccultistDomain::clairvoyant: {
                auto* prop =
                        static_cast<PropMagicSearching*>(
                                property_factory::make(
                                        PropId::magic_searching ) );

                prop->set_indefinite();

                prop->set_range( g_fov_radi_int );

                map::g_player->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbose::no );
        }
        break;

        case OccultistDomain::enchanter: {
        }
        break;

        case OccultistDomain::invoker: {
        }
        break;

        case OccultistDomain::transmuter: {
        }
        break;

        case OccultistDomain::END: {
                ASSERT( false );
        }
        break;
        }
}

void on_player_gained_lvl( const int new_lvl )
{
        switch ( s_player_bg )
        {
        case Bg::exorcist: {
                const bool is_exorcist_extra_trait =
                        ( new_lvl == s_exorcist_bon_trait_lvl_1 ) ||
                        ( new_lvl == s_exorcist_bon_trait_lvl_2 ) ||
                        ( new_lvl == s_exorcist_bon_trait_lvl_3 );

                if ( is_exorcist_extra_trait )
                {
                        states::push(
                                std::make_unique<PickTraitState>(
                                        "You gain an extra trait!" ) );
                }
        }
        break;

        case Bg::ghoul: {
        }
        break;

        case Bg::occultist: {
                const bool is_occultist_spell_incr_lvl =
                        ( new_lvl == s_occultist_upgrade_lvl_1 ) ||
                        ( new_lvl == s_occultist_upgrade_lvl_2 );

                if ( is_occultist_spell_incr_lvl )
                {
                        incr_occultist_spells();
                }

                switch ( s_player_occultist_domain )
                {
                case OccultistDomain::clairvoyant: {
                        auto* const prop =
                                map::g_player->m_properties.prop(
                                        PropId::magic_searching );

                        ASSERT( prop );

                        auto* const searching =
                                static_cast<PropMagicSearching*>( prop );

                        if ( new_lvl == s_occultist_upgrade_lvl_1 )
                        {
                                searching->set_allow_reveal_items();
                        }
                        else if ( new_lvl == s_occultist_upgrade_lvl_2 )
                        {
                                searching->set_allow_reveal_creatures();
                        }
                }
                break;

                case OccultistDomain::enchanter: {
                }
                break;

                case OccultistDomain::invoker: {
                }
                break;

                case OccultistDomain::transmuter: {
                }
                break;

                case OccultistDomain::END: {
                        ASSERT( false );
                }
                break;
                }
        }
        break;

        case Bg::rogue: {
        }
        break;

        case Bg::war_vet: {
        }
        break;

        case Bg::END: {
                ASSERT( false );
        }
        break;
        }
}

void set_all_traits_to_picked()
{
        for ( auto& d : s_trait_data )
        {
                d.is_picked = true;
        }
}

void pick_trait( const Trait id )
{
        ASSERT( id != Trait::END );

        trait_data( id ).is_picked = true;

        TraitLogEntry trait_log_entry;

        trait_log_entry.trait_id = id;
        trait_log_entry.clvl_picked = game::clvl();

        s_trait_log.push_back( trait_log_entry );

        const auto& d = trait_data( id );

        if ( d.on_picked )
        {
                // Has trait pick function
                trait_data( id ).on_picked();
        }
}

std::vector<TraitLogEntry> trait_log()
{
        return s_trait_log;
}

bool gets_undead_bane_bon( const actor::ActorData& actor_data )
{
        return (
                has_trait( Trait::undead_bane ) &&
                actor_data.is_undead );
}

}  // namespace player_bon
