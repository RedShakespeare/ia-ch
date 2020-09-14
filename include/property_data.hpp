// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef PROPERTY_DATA_HPP
#define PROPERTY_DATA_HPP

#include "random.hpp"

// NOTE: When updating this, also update the two maps below
enum class PropId
{
        r_phys,
        r_fire,
        r_poison,
        r_elec,
        r_acid,
        r_sleep,
        r_fear,
        r_slow,
        r_conf,
        r_breath,
        r_disease,
        r_shock,
        // NOTE: The purpose of this is only to prevent blindness for "eyeless"
        // monsters (e.g. constructs such as animated weapons), and is only
        // intended as a natural property - not for e.g. gas masks.
        r_blind,
        r_para,  // Mostly intended as a natural property for monsters
        r_spell,
        light_sensitive,
        blind,
        deaf,
        fainted,
        burning,
        radiant_adjacent,
        radiant_fov,
        invis,
        cloaked,
        recloaks,
        see_invis,
        darkvision,
        poisoned,
        paralyzed,
        terrified,
        confused,
        stunned,
        slowed,
        hasted,
        infected,
        diseased,
        weakened,
        frenzied,
        blessed,
        cursed,
        premonition,
        magic_searching,
        entangled,
        tele_ctrl,
        spell_reflect,
        conflict,
        vortex,  // Vortex monsters pulling the player
        explodes_on_death,
        splits_on_death,
        corpse_eater,
        teleports,
        corrupts_env_color,  // "Strange color" monster corrupting the area
        alters_env,
        regenerates,
        corpse_rises,
        spawns_zombie_parts_on_destroyed,
        breeds,
        vomits_ooze,  // Gla'Suu
        confuses_adjacent,  // "Strange color" confusing player when seen
        speaks_curses,
        aura_of_decay,  // Damages adjacent hostile creatures
        reduced_pierce_dmg,  // E.g. worm masses
        short_hearing_range,

        // Properties describing the actors body and/or method of moving around
        // (typically affects which terrain types the actor can move through,
        // but may have other effects)
        flying,
        ethereal,
        ooze,
        small_crawling,
        burrowing,

        // Properties mostly used for AI control
        waiting,  // Prevent acting - also used for player
        disabled_attack,
        disabled_melee,
        disabled_ranged,

        // Properties for supporting specific game mechanics (not intended to be
        // used in a general way)
        descend,
        zuul_possess_priest,
        possessed_by_zuul,
        shapeshifts,  // For the Shapeshifter monster
        major_clapham_summon,
        aiming,
        nailed,
        flared,
        wound,
        clockwork_hasted,  // For the Arcane Clockwork artifact
        summoned,
        swimming,
        hp_sap,
        spi_sap,
        mind_sap,
        hit_chance_penalty_curse,
        increased_shock_curse,
        cannot_read_curse,
        light_sensitive_curse,  // This is just a copy of light_sensitive
        disabled_hp_regen,
        sanctuary,

        END
};

enum class PropAlignment
{
        good,
        bad,
        neutral
};

struct PropData
{
        PropData() :
                id(PropId::END),
                std_rnd_turns(Range(10, 10)),
                name(""),
                name_short(""),
                descr(""),
                msg_start_player(""),
                msg_start_mon(""),
                msg_end_player(""),
                msg_end_mon(""),
                msg_res_player(""),
                msg_res_mon(""),
                historic_msg_start_permanent(""),
                historic_msg_end_permanent(""),
                allow_display_turns(true),
                update_vision_on_toggled(false),
                allow_test_on_bot(false),
                alignment(PropAlignment::neutral) {}

        PropId id;
        Range std_rnd_turns;
        std::string name;
        std::string name_short;
        std::string descr;
        std::string msg_start_player;
        std::string msg_start_mon;
        std::string msg_end_player;
        std::string msg_end_mon;
        std::string msg_res_player;
        std::string msg_res_mon;
        std::string historic_msg_start_permanent;
        std::string historic_msg_end_permanent;
        bool allow_display_turns;
        bool update_vision_on_toggled;
        bool allow_test_on_bot;
        PropAlignment alignment;
};

namespace property_data
{
extern PropData g_data[(size_t)PropId::END];

void init();

PropId str_to_prop_id(const std::string& str);

}  // namespace property_data

#endif  // PROPERTY_DATA_HPP
