#ifndef PLAYER_BON_HPP
#define PLAYER_BON_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <math.h>

#include "global.hpp"

struct ActorData;
struct ColoredString;

enum class Trait
{
        // Common (except some traits can be blocked for certain backgrounds)
        adept_melee,
        expert_melee,
        master_melee,
        adept_marksman,
        expert_marksman,
        master_marksman,
        cool_headed,
        courageous,
        dexterous,
        fearless,
        stealthy,
        imperceptible,
        silent,
        lithe,
        vigilant,
        treasure_hunter,
        self_aware,
        healer,
        rapid_recoverer,
        survivalist,
        stout_spirit,
        strong_spirit,
        mighty_spirit,
        absorb,
        tough,
        rugged,
        thick_skinned,
        resistant,
        strong_backed,
        undead_bane,
        elec_incl,

        // Unique for Ghoul
        ravenous,
        foul,
        toxic,
        indomitable_fury,

        // Unique for Rogue
        vicious,

        // Unique for War veteran
        steady_aimer,

        END
};

enum class Bg
{
        ghoul,
        occultist,
        rogue,
        war_vet,

        END
};

enum class OccultistDomain
{
        clairvoyant,
        enchanter,
        invoker,
        // summoner,
        transmuter,

        END
};

namespace player_bon
{

extern bool traits[(size_t)Trait::END];

void init();

void save();

void load();

std::vector<Bg> pickable_bgs();

std::vector<OccultistDomain> pickable_occultist_domains();

void unpicked_traits_for_bg(
        const Bg bg,
        std::vector<Trait>& traits_can_be_picked_out,
        std::vector<Trait>& traits_prereqs_not_met_out);

void trait_prereqs(
        const Trait id,
        const Bg bg,
        std::vector<Trait>& traits_out,
        Bg& bg_out,
        int& clvl_out);

Bg bg();

OccultistDomain occultist_domain();

bool has_trait(const Trait id);

std::string trait_title(const Trait id);

std::string trait_descr(const Trait id);

std::string bg_title(const Bg id);

std::string spell_domain_title(const OccultistDomain domain);

std::string occultist_profession_title(const OccultistDomain domain);

// NOTE: The string vector returned is not formatted. Each line still needs to
// be formatted by the caller. The reason for using a vector instead of a string
// is to separate the text into paragraphs.
std::vector<ColoredString> bg_descr(const Bg id);

std::string occultist_domain_descr(const OccultistDomain domain);

std::string all_picked_traits_titles_line();

void pick_trait(const Trait id);

void pick_bg(const Bg bg);

void pick_occultist_domain(const OccultistDomain domain);

void on_player_gained_lvl(const int new_lvl);

void set_all_traits_to_picked();

bool gets_undead_bane_bon(const ActorData& actor_data);

} // player_bon

#endif // PLAYER_BON_HPP
