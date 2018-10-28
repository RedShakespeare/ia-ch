#ifndef ABILITY_VALUES_HPP
#define ABILITY_VALUES_HPP

#include <cstddef>
#include <unordered_map>

class Actor;

enum class AbilityId
{
    melee,
    ranged,
    dodging,
    stealth,
    searching,
    END
};

const std::unordered_map<std::string, AbilityId> str_to_ability_id_map = {
    {"melee", AbilityId::melee},
    {"ranged", AbilityId::ranged},
    {"dodging", AbilityId::dodging},
    {"stealth", AbilityId::stealth},
    {"searching", AbilityId::searching},
};

const std::unordered_map<AbilityId, std::string> ability_id_to_str_map = {
    {AbilityId::melee, "melee"},
    {AbilityId::ranged, "ranged"},
    {AbilityId::dodging, "dodging"},
    {AbilityId::stealth, "stealth"},
    {AbilityId::searching, "searching"},
};

enum class ActionResult
{
    fail_critical,
    fail_big,
    fail,
    success,
    success_big,
    success_critical
};

// Each actor has an instance of this class
class AbilityValues
{
public:
    AbilityValues()
    {
        reset();
    }

    AbilityValues& operator=(const AbilityValues& other)
    {
        for (size_t i = 0; i < (size_t)AbilityId::END; ++i)
        {
            ability_list[i] = other.ability_list[i];
        }

        return *this;
    }

    void reset();

    int val(const AbilityId id,
            const bool is_affected_by_props,
            const Actor& actor) const;

    int raw_val(const AbilityId id) const
    {
        return ability_list[(size_t)id];
    }

    void set_val(const AbilityId id, const int val);

    void change_val(const AbilityId id, const int change);

private:
    int ability_list[(size_t)AbilityId::END];
};

namespace ability_roll
{

ActionResult roll(const int skill_value);

// Intended for presenting hit chance values to the player (when aiming etc).
// The parameter value (which could for example be > 100% due to bonuses) is
// clamped within the actual possible hit chance range, considering critical
// hits and misses.
int hit_chance_pct_actual(const int value);

} // ability_roll

#endif // ABILITY_VALUES_HPP
