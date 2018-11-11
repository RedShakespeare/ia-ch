#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <cstdlib>

// -----------------------------------------------------------------------------
// Data
// -----------------------------------------------------------------------------
const size_t player_name_max_len = 14;

// NOTE:
// Early = dlvl 1  - 9
// Mid   = dlvl 10 - 19
// Late  = dlvl 20 - 30
const int dlvl_last_early_game = 9;
const int dlvl_first_mid_game = dlvl_last_early_game + 1;
const int dlvl_last_mid_game = 19;
const int dlvl_first_late_game = dlvl_last_mid_game + 1;
const int dlvl_last = 30;

const int dlvl_harder_traps = 6;

const size_t ms_delay_player_unable_act = 7;
const size_t min_ms_between_same_sfx = 60;

const int fov_radi_int = 6;
const int fov_w_int = (fov_radi_int * 2) + 1;
const double fov_radi_db = (double)fov_radi_int;

const int dynamite_fuse_turns = 5;
const int expl_std_radi = 2;

const int enc_immobile_lvl = 125;

const size_t nr_mg_projectiles = 5;

const int mi_go_gun_hp_drained = 3;

// NOTE: Number of rolls is reduced by one for each step away from the center
const int expl_dmg_rolls = 5;
const int expl_dmg_sides = 6;
const int expl_dmg_plus = 10;
const int expl_max_dmg = (expl_dmg_rolls * expl_dmg_sides) + expl_dmg_plus;

const int poison_dmg_n_turn = 4;

const int shock_from_obsession = 30;

const double shock_from_disturbing_items = 0.05;

// How many "units" of weight the player can carry, without trait modifiers etc
const int player_carry_weight_base = 500;

// Value used for limiting spawning over time and "breeder" monsters. The actual
// number of actors may sometimes go a bit above this number, e.g. due to a
// group of monsters spawning when the number of actors is near the limit.
// Summoning spells does not check this number at all (because their effects
// should not be arbitrarily limited by this) - so that may also push the number
// of actors above the limit. This number is treated as a soft limit.
const size_t max_nr_actors_on_map = 125;

// -----------------------------------------------------------------------------
// Types
// -----------------------------------------------------------------------------
// This is typically used for functions such as item identification and property
// applying to enable/disable printing to the message log, animating, or other
// such "side effects". For example when loading a saved game, we may want to do
// these things silently.
enum class Verbosity
{
        silent,
        verbose,
};

enum class UpdateScreen
{
        no,
        yes
};

enum class InvType
{
        slots,
        backpack
};

enum class AllowAction
{
        no,
        yes
};

enum class DidAction
{
        no,
        yes
};

enum class PassTime
{
        no,
        yes
};

enum class ConsumeItem
{
        no,
        yes
};

enum class ItemRefType
{
        plain,
        a,
        plural,
        END
};

enum class ItemRefInf
{
        none,
        yes
};

enum class ItemRefAttInf
{
        none,
        wpn_main_att_mode,
        melee,
        ranged,
        thrown
};

enum class ItemRefDmg
{
        average,
        average_and_melee_plus,
        dice,
};

enum class Article
{
        a,
        the
};

enum class Matl
{
        empty,
        stone,
        metal,
        plant,  // Grass, bushes, reeds, vines, fungi...
        wood,   // Trees, doors, benches...
        cloth,  // Carpet, silk (cocoons)...
        fluid
};

enum class LiquidType
{
        water,
        mud,
};

enum class Condition
{
        breaking,
        shoddy,
        fine
};

enum class DmgType
{
        physical,
        fire,
        acid,
        electric,
        spirit,
        light,
        pure,
        END
};

enum class DmgMethod
{
        piercing,
        slashing,
        blunt,
        kicking,
        explosion,
        shotgun,
        elemental,
        forced, // Guaranteed to detroy the feature (silently - no messages)
        END
};

enum class AttMode
{
        none,
        melee,
        thrown,
        ranged
};

enum class AllowWound
{
        no,
        yes
};

enum class ShockLvl
{
        none,
        unsettling,
        frightening,
        terrifying,
        mind_shattering,
        END
};

enum class MonRoamingAllowed
{
        no,
        yes
};

enum class GameEntryMode
{
        new_game,
        load_game
};

enum class IsWin
{
        no,
        yes
};

enum class SpawnRate
{
        never,
        extremely_rare,
        very_rare,
        rare,
        common,
        very_common
};

enum class VerDir
{
        up,
        down
};

enum class ActorState
{
        alive,
        corpse,
        destroyed
};

enum class ShouldCtrlTele
{
        if_tele_ctrl_prop,
        never,
        always
};

enum class Axis
{
        hor,
        ver
};

enum class IsSubRoom
{
        no,
        yes
};

enum class LgtSize
{
        none,
        small, // 3x3
        fov
};

enum class MorePromptOnMsg
{
        no,
        yes
};

enum class ItemType
{
        general,
        melee_wpn,
        ranged_wpn,
        throwing_wpn,
        ammo,
        ammo_mag,
        scroll,
        potion,
        device,
        rod,
        armor,
        head_wear,
        explosive,

        END_OF_EXTRINSIC_ITEMS,
        melee_wpn_intr,
        ranged_wpn_intr
};

#endif // GLOBAL_HPP
