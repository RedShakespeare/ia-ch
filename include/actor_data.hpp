// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ACTOR_DATA_HPP
#define ACTOR_DATA_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ability_values.hpp"
#include "colors.hpp"
#include "item_att_property.hpp"
#include "item_data.hpp"
#include "property_data.hpp"
#include "random.hpp"
#include "spells.hpp"

enum class MonShockLvl;

namespace audio
{
enum class SfxId;
}  // namespace audio

namespace gfx
{
enum class TileId;
}  // namespace gfx

namespace room
{
enum class RoomType;
}  // namespace room

namespace actor
{
enum class MonGroupSize
{
    alone,
    few,
    pack,
    swarm
};

// Each actor data entry has a list of this struct, this is used for choosing
// group sizes when spawning monsters. The size of the group spawned is
// determined by a weighted random choice (so that a certain monster could for
// example usually spawn alone, but on some rare occasions spawn in big groups).
struct MonGroupSpawnRule
{
    MonGroupSize group_size {MonGroupSize::alone};
    int weight {1};
    int required_dlvl {0};
};

struct ActorItemSetData
{
    item::ItemSetId item_set_id {(item::ItemSetId)0};
    int pct_chance_to_spawn {100};
    Range nr_spawned_range {1, 1};
};

struct IntrAttData
{
    IntrAttData() = default;

    ~IntrAttData() = default;

    item::Id item_id {item::Id::END};
    int dmg {0};
    ItemAttackProp prop_applied {};
};

struct ActorSpellData
{
    SpellId spell_id {SpellId::END};
    SpellSkill spell_skill {SpellSkill::basic};
    int pct_chance_to_know {100};
};

struct StartingAllyEntry
{
    std::string id {};
    Range nr {1, 1};
};

enum class Speed
{
    slow,
    normal,
    fast,
    very_fast,
};

enum class Size
{
    floor,
    humanoid,
    giant
};

enum class AiId
{
    looks,
    avoids_blocking_friend,
    attacks,
    paths_to_target_when_aware,
    moves_to_target_when_los,
    moves_to_lair,
    moves_to_leader,
    moves_randomly_when_unaware,
    END
};

struct ActorData
{
    // Default member initializers express the reset state. To reset an
    // existing instance, assign a fresh default-constructed value:
    //   data = ActorData{};
    // (the constructor sets the one non-trivial default below).
    ActorData()
    {
        ai[(size_t)AiId::moves_randomly_when_unaware] = true;
    }

    // Reset to default-constructed state (kept for in-place reset call sites).
    void reset();

    std::string id {};
    std::string name_a {};
    std::string name_a_i18n_key {};
    std::string name_the {};
    std::string name_the_i18n_key {};
    std::string corpse_name_a {};
    std::string corpse_name_a_i18n_key {};
    std::string corpse_name_the {};
    std::string corpse_name_the_i18n_key {};
    gfx::TileId tile {gfx::TileId::END};
    char character {'X'};
    Color color {colors::yellow()};
    std::vector<MonGroupSpawnRule> group_sizes {};
    int hp {0};
    int spi {0};
    std::vector<ActorItemSetData> item_sets {};
    std::vector<std::shared_ptr<IntrAttData>> intr_attacks {};
    std::vector<ActorSpellData> spells {};
    Speed speed {Speed::normal};
    AbilityValues ability_values {};
    bool natural_props[(size_t)prop::Id::END] {};
    bool ai[(size_t)AiId::END] {};
    int nr_turns_aware {0};
    int ranged_cooldown_turns {0};
    bool is_pausing_on_player_seen {false};
    int spawn_min_dlvl {-1};
    int spawn_max_dlvl {-1};
    int spawn_weight {100};
    Size actor_size {Size::humanoid};
    bool allow_wielded_wpn_descr {false};
    bool allow_speed_descr {false};
    int nr_kills {0};
    bool has_player_seen {false};
    bool can_open_doors {false};
    bool can_bash_doors {false};
    // NOTE: Knockback may also be prevented by other soucres, e.g. if the
    // monster is ethereal
    bool prevent_knockback {false};
    int nr_left_allowed_to_spawn {-1};
    bool is_unique {false};
    bool is_auto_spawn_allowed {true};
    std::string descr {};
    std::string descr_i18n_key {};
    std::string smell_msg {};
    std::string smell_msg_i18n_key {};
    std::string wary_msg {};
    std::string wary_msg_i18n_key {};
    std::string aware_msg_mon_seen {};
    std::string aware_msg_mon_seen_i18n_key {};
    std::string aware_msg_mon_hidden {};
    std::string aware_msg_mon_hidden_i18n_key {};
    bool use_cultist_aware_msg_mon_seen {false};
    bool use_cultist_aware_msg_mon_hidden {false};
    audio::SfxId aware_sfx_mon_seen {audio::SfxId::END};
    audio::SfxId aware_sfx_mon_hidden {audio::SfxId::END};
    std::string spell_msg_sound {};
    std::string spell_msg_sound_i18n_key {};
    std::string spell_msg_visual {};
    std::string spell_msg_visual_i18n_key {};
    std::string death_msg_override {};
    std::string death_msg_override_i18n_key {};
    int erratic_move_pct {0};
    MonShockLvl mon_shock_lvl {MonShockLvl::none};
    bool is_humanoid {false};
    bool is_rat {false};
    bool is_canine {false};
    bool is_spider {false};
    bool is_ghost {false};
    bool is_ghoul {false};
    bool is_snake {false};
    bool is_reptile {false};
    bool is_amphibian {false};
    bool can_be_summoned_by_mon {false};
    bool can_spawn_from_tomb {false};
    bool can_be_shapeshifted_into {false};
    bool can_bleed {true};
    bool can_leave_corpse {true};
    bool prio_corpse_bash {false};
    std::vector<room::RoomType> native_rooms {};
    std::vector<StartingAllyEntry> starting_allies {};
};

extern std::unordered_map<std::string, ActorData> g_data;

void init();

std::string localized_text(
    const std::string& key,
    const std::string& fallback);

void save();
void load();

}  // namespace actor

#endif  // ACTOR_DATA_HPP
