#ifndef PROPERTY_DATA_HPP
#define PROPERTY_DATA_HPP

#include <unordered_map>

#include "rl_utils.hpp"

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
        // NOTE: The purpose of this is only to prevent blindness for "eyeless"
        // monsters (e.g. constructs such as animated weapons), and is only
        // intended as a natural property - not for e.g. gas masks.
        r_blind,
        r_para, // Mostly intended as a natural property for monsters
        r_spell,
        light_sensitive,
        blind,
        deaf,
        fainted,
        burning,
        radiant,
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
        divert_attacks,
        magic_searching,
        entangled,
        tele_ctrl,
        spell_reflect,
        conflict,
        vortex, // Vortex monsters pulling the player
        explodes_on_death,
        splits_on_death,
        corpse_eater,
        teleports,
        corrupts_env_color, // "Strange color" monster corrupting the area
        alters_env,
        regenerates,
        corpse_rises,
        spawns_zombie_parts_on_destroyed,
        breeds,
        confuses_adjacent, // "Strange color" confusing player when seen
        speaks_curses,
        aura_of_decay, // Damages adjacent hostile creatures

        // Properties describing the actors body and/or method of moving around
        flying,
        ethereal,
        ooze,
        burrowing,

        // Properties mostly used for AI control
        waiting, // Prevent acting - also used for player
        disabled_attack,
        disabled_melee,
        disabled_ranged,

        // Properties for supporting specific game mechanics (not intended to be
        // used in a general way)
        descend,
        zuul_possess_priest,
        possessed_by_zuul,
        major_clapham_summon,
        aiming,
        nailed,
        flared,
        wound,
        clockwork_hasted, // For the Arcane Clockwork artifact
        summoned,
        swimming,
        hp_sap,
        spi_sap,
        mind_sap,

        END
};

const std::unordered_map<std::string, PropId> str_to_prop_id_map = {
        {"alters_environment", PropId::alters_env},
        {"aura of decay", PropId::aura_of_decay},
        {"blessed", PropId::blessed},
        {"blind", PropId::blind},
        {"breeds", PropId::breeds},
        {"burning", PropId::burning},
        {"burrowing", PropId::burrowing},
        {"cloaked", PropId::cloaked},
        {"conflict", PropId::conflict},
        {"confused", PropId::confused},
        {"confuses_adjacent", PropId::confuses_adjacent},
        {"corpse_eater", PropId::corpse_eater},
        {"corpse_rises", PropId::corpse_rises},
        {"corrupts_environment_color", PropId::corrupts_env_color},
        {"cursed", PropId::cursed},
        {"darkvision", PropId::darkvision},
        {"deaf", PropId::deaf},
        {"diseased", PropId::diseased},
        {"divert_attacks", PropId::divert_attacks},
        {"entangled", PropId::entangled},
        {"ethereal", PropId::ethereal},
        {"explodes_on_death", PropId::explodes_on_death},
        {"fainted", PropId::fainted},
        {"flying", PropId::flying},
        {"frenzied", PropId::frenzied},
        {"hasted", PropId::hasted},
        {"infected", PropId::infected},
        {"invis", PropId::invis},
        {"light_sensitive", PropId::light_sensitive},
        {"magic_searching", PropId::magic_searching},
        {"major_clapham_summon", PropId::major_clapham_summon},
        {"ooze", PropId::ooze},
        {"paralyzed", PropId::paralyzed},
        {"poisoned", PropId::poisoned},
        {"r_acid", PropId::r_acid},
        {"r_blind", PropId::r_blind},
        {"r_breath", PropId::r_breath},
        {"r_conf", PropId::r_conf},
        {"r_disease", PropId::r_disease},
        {"r_elec", PropId::r_elec},
        {"r_fear", PropId::r_fear},
        {"r_fire", PropId::r_fire},
        {"r_para", PropId::r_para},
        {"r_phys", PropId::r_phys},
        {"r_poison", PropId::r_poison},
        {"r_sleep", PropId::r_sleep},
        {"r_slow", PropId::r_slow},
        {"r_spell", PropId::r_spell},
        {"radiant", PropId::radiant},
        {"recloaks", PropId::recloaks},
        {"regenerates", PropId::regenerates},
        {"see_invis", PropId::see_invis},
        {"slowed", PropId::slowed},
        {"spawns_zombie_parts_on_destroyed",
         PropId::spawns_zombie_parts_on_destroyed},
        {"speaks_curses", PropId::speaks_curses},
        {"spell_reflect", PropId::spell_reflect},
        {"splits_on_death", PropId::splits_on_death},
        {"stunned", PropId::stunned},
        {"tele_ctrl", PropId::tele_ctrl},
        {"teleports", PropId::teleports},
        {"terrified", PropId::terrified},
        {"vortex", PropId::vortex},
        {"weakened", PropId::weakened},
        {"zuul_possess_priest", PropId::zuul_possess_priest}
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
                is_making_mon_aware(false),
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
        bool is_making_mon_aware;
        bool allow_display_turns;
        bool update_vision_on_toggled;
        bool allow_test_on_bot;
        PropAlignment alignment;
};

namespace property_data
{

extern PropData data[(size_t)PropId::END];

void init();

} // prop_data

#endif // PROPERTY_DATA_HPP
