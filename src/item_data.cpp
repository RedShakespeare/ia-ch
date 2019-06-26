// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "item_data.hpp"

#include <iostream>
#include <climits>

#include "init.hpp"
#include "colors.hpp"
#include "item.hpp"
#include "item_att_property.hpp"
#include "actor_data.hpp"
#include "actor_player.hpp"
#include "sound.hpp"
#include "item_device.hpp"
#include "map.hpp"
#include "saving.hpp"
#include "game_time.hpp"
#include "property.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static void mod_spawn_chance(item::ItemData& data, const double factor)
{
        data.chance_to_incl_in_spawn_list =
                (int)((double)data.chance_to_incl_in_spawn_list * factor);
}

// Item archetypes (defaults)
static void reset_data(item::ItemData& d, ItemType const item_type)
{
        switch (item_type)
        {
        case ItemType::general:
                d = item::ItemData();
                break;

        case ItemType::melee_wpn:
                reset_data(d, ItemType::general);
                d.type = ItemType::melee_wpn;
                d.is_stackable = false;
                d.weight = item::Weight::medium;
                d.character = ')';
                d.color = colors::white();
                d.main_att_mode = AttMode::melee;
                d.melee.is_melee_wpn = true;
                d.melee.miss_sfx = SfxId::miss_medium;
                d.melee.hit_small_sfx = SfxId::hit_small;
                d.melee.hit_medium_sfx = SfxId::hit_medium;
                d.melee.hit_hard_sfx = SfxId::hit_hard;
                d.ranged.is_throwable_wpn = true;
                d.land_on_hard_snd_msg = "I hear a clanking sound.";
                d.land_on_hard_sfx = SfxId::metal_clank;
                break;

        case ItemType::melee_wpn_intr:
                reset_data(d, ItemType::melee_wpn);
                d.type = ItemType::melee_wpn_intr;
                d.is_intr = true;
                d.spawn_std_range = Range(-1, -1);
                d.chance_to_incl_in_spawn_list = 0;
                d.allow_spawn = false;
                d.melee.hit_small_sfx = SfxId::hit_small;
                d.melee.hit_medium_sfx = SfxId::hit_medium;
                d.melee.hit_hard_sfx = SfxId::hit_hard;
                d.melee.miss_sfx = SfxId::END;
                d.ranged.is_throwable_wpn = false;
                break;

        case ItemType::ranged_wpn:
                reset_data(d, ItemType::general);
                d.type = ItemType::ranged_wpn;
                d.is_stackable = false;
                d.weight = item::Weight::medium;
                d.character = '}';
                d.color = colors::white();
                d.melee.is_melee_wpn = true;
                d.melee.dmg = DmgRange(1, 3);
                d.main_att_mode = AttMode::ranged;
                d.ranged.is_ranged_wpn = true;
                d.ranged.projectile_character = '/';
                d.ranged.projectile_color = colors::white();
                d.spawn_std_range.max = g_dlvl_last_mid_game;
                d.melee.hit_small_sfx = SfxId::hit_small;
                d.melee.hit_medium_sfx = SfxId::hit_medium;
                d.melee.hit_hard_sfx = SfxId::hit_hard;
                d.melee.miss_sfx = SfxId::miss_medium;
                d.ranged.snd_vol = SndVol::high;
                break;

        case ItemType::ranged_wpn_intr:
                reset_data(d, ItemType::ranged_wpn);
                d.type = ItemType::ranged_wpn_intr;
                d.is_intr = true;
                d.ranged.has_infinite_ammo = true;
                d.spawn_std_range = Range(-1, -1);
                d.chance_to_incl_in_spawn_list = 0;
                d.allow_spawn = false;
                d.melee.is_melee_wpn = false;
                d.ranged.projectile_character = '*';
                d.ranged.snd_vol = SndVol::low;
                break;

        case ItemType::throwing_wpn:
                reset_data(d, ItemType::general);
                d.type = ItemType::throwing_wpn;
                d.weight = item::Weight::extra_light;
                d.is_stackable = true;
                d.spawn_std_range.max = g_dlvl_last_mid_game;
                d.ranged.snd_vol = SndVol::low;
                d.ranged.is_throwable_wpn = true;
                break;

        case ItemType::ammo:
                reset_data(d, ItemType::general);
                d.type = ItemType::ammo;
                d.weight = item::Weight::extra_light;
                d.character = '{';
                d.color = colors::white();
                d.tile = TileId::ammo;
                d.spawn_std_range.max = g_dlvl_last_mid_game;
                break;

        case ItemType::ammo_mag:
                reset_data(d, ItemType::ammo);
                d.type = ItemType::ammo_mag;
                d.weight = item::Weight::light;
                d.is_stackable = false;
                d.spawn_std_range.max = g_dlvl_last_mid_game;
                break;

        case ItemType::scroll:
                // NOTE: Scroll spawning chances are set elsewhere
                reset_data(d, ItemType::general);
                d.type = ItemType::scroll;
                d.has_std_activate = true;
                d.base_descr = {
                        "A short transcription of an eldritch incantation. "
                        "There is a strange aura about it, as if some power "
                        "was imbued in the paper itself.",
                        "It should be possible to pronounce it correctly, but "
                        "the purpose is unclear."
                };
                d.value = item::Value::minor_treasure;
                d.weight = item::Weight::none;
                d.is_identified = false;
                d.is_spell_domain_known = false;
                d.xp_on_found = 8;
                d.character = '?';
                d.color = colors::white();
                d.tile = TileId::scroll;
                d.max_stack_at_spawn = 1;
                d.land_on_hard_snd_msg = "";
                d.native_containers.push_back(terrain::Id::chest);
                d.native_containers.push_back(terrain::Id::tomb);
                d.native_containers.push_back(terrain::Id::cabinet);
                d.native_containers.push_back(terrain::Id::bookshelf);
                d.native_containers.push_back(terrain::Id::cocoon);
                break;

        case ItemType::potion:
                reset_data(d, ItemType::general);
                d.type = ItemType::potion;
                d.has_std_activate = true;
                d.base_descr = {
                        "A small glass bottle containing a mysterious "
                        "concoction."
                };
                d.value = item::Value::minor_treasure;
                d.chance_to_incl_in_spawn_list = 60;
                d.weight = item::Weight::light;
                d.is_identified = false;
                d.is_alignment_known = false;
                d.xp_on_found = 8;
                d.character = '!';
                d.tile = TileId::potion;
                d.ranged.throw_hit_chance_mod = 15;
                d.ranged.dmg = DmgRange(1, 3);
                d.ranged.always_break_on_throw = true;
                d.max_stack_at_spawn = 1;
                d.land_on_hard_snd_msg = "";
                d.ranged.is_throwable_wpn = true;
                d.native_containers.push_back(terrain::Id::chest);
                d.native_containers.push_back(terrain::Id::tomb);
                d.native_containers.push_back(terrain::Id::cabinet);
                d.native_containers.push_back(terrain::Id::alchemist_bench);
                d.native_containers.push_back(terrain::Id::cocoon);
                break;

        case ItemType::device:
                reset_data(d, ItemType::general);
                d.type = ItemType::device;
                d.value = item::Value::rare_treasure;
                d.has_std_activate = true;
                d.base_name_un_id = {
                        "Strange Device",
                        "Strange Devices",
                        "a Strange Device"
                };
                d.base_descr = {
                        "A small piece of machinery. It could not possibly "
                        "have been designed by a human mind. Even for its "
                        "small size, it seems incredibly complex. There is no "
                        "hope of understanding the purpose or function of it "
                        "through normal means."
                };
                d.weight = item::Weight::light;
                d.is_identified = false;
                d.character = '%';
                d.tile = TileId::device1;
                d.is_stackable = false;
                d.land_on_hard_snd_msg = "I hear a clanking sound.";
                d.land_on_hard_sfx = SfxId::metal_clank;
                d.chance_to_incl_in_spawn_list = 5;
                d.native_containers.push_back(terrain::Id::chest);
                d.native_containers.push_back(terrain::Id::cocoon);
                break;

        case ItemType::rod:
                reset_data(d, ItemType::general);
                d.type = ItemType::rod;
                d.value = item::Value::rare_treasure;
                d.has_std_activate = true;
                d.base_descr = {
                        "A peculiar metallic device of cylindrical shape. The "
                        "only details are a single button on the side, and a "
                        "small display."
                };
                d.weight = item::Weight::light;
                d.is_identified = false;
                d.xp_on_found = 15;
                d.character = '%';
                d.tile = TileId::rod;
                d.is_stackable = false;
                d.land_on_hard_snd_msg = "I hear a clanking sound.";
                d.land_on_hard_sfx = SfxId::metal_clank;
                d.chance_to_incl_in_spawn_list = 5;
                d.native_containers.push_back(terrain::Id::chest);
                d.native_containers.push_back(terrain::Id::cocoon);
                break;

        case ItemType::armor:
                reset_data(d, ItemType::general);
                d.type = ItemType::armor;
                d.weight = item::Weight::heavy;
                d.character = '[';
                d.tile = TileId::armor;
                d.is_stackable = false;
                break;

        case ItemType::head_wear:
                reset_data(d, ItemType::general);
                d.type = ItemType::head_wear;
                d.character = '[';
                d.is_stackable = false;
                break;

        case ItemType::explosive:
                reset_data(d, ItemType::general);
                d.type = ItemType::explosive;
                d.has_std_activate = true;
                d.weight = item::Weight::light;
                d.character = '-';
                d.max_stack_at_spawn = 2;
                d.land_on_hard_snd_msg = "";
                break;

        default:
                break;
        }
}


// -----------------------------------------------------------------------------
// item
// -----------------------------------------------------------------------------
namespace item
{

ItemData g_data[(size_t)Id::END];


void init()
{
        TRACE_FUNC_BEGIN;

        ItemData d;

        reset_data(d, ItemType::general);
        d.id = Id::trapez;;
        d.base_name = {
                "Shining Trapezohedron",
                "Shining Trapezohedrons",
                "The Shining Trapezohedron"
        };
        d.spawn_std_range = Range(-1, -1);
        d.chance_to_incl_in_spawn_list = 0;
        d.allow_spawn = false;
        d.is_stackable = false;
        d.character = '*';
        d.color = colors::light_red();
        d.tile = TileId::trapez;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d.id = Id::sawed_off;
        d.base_name = {
                "Sawed-off Shotgun", "Sawed-off shotguns", "a Sawed-off Shotgun"
        };
        d.base_descr = {
                "Compared to a standard shotgun, the sawed-off has a shorter "
                "effective range - however, at close range it is more "
                "devastating. It holds two barrels, and needs to be reloaded "
                "after both are discharged."
        };
        d.weight = Weight::medium;
        d.tile = TileId::shotgun;
        d.ranged.is_shotgun = true;
        d.melee.att_msgs = {"strike", "strikes me with a shotgun"};
        d.ranged.max_ammo = 2;
        d.ranged.dmg = DmgRange(8, 24);
        d.ranged.hit_chance_mod = 0;
        d.ranged.effective_range = 3;
        d.ranged.ammo_item_id = Id::shotgun_shell;
        d.ranged.att_msgs = {"fire", "fires a shotgun"};
        d.ranged.snd_msg = "I hear a shotgun blast.";
        d.ranged.att_sfx = SfxId::shotgun_sawed_off_fire;
        d.ranged.makes_ricochet_snd = true;
        d.ranged.reload_sfx = SfxId::shotgun_reload;
        d.spawn_std_range.min = 2;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d.id = Id::pump_shotgun;
        d.base_name = {"Pump Shotgun", "Pump shotguns", "a Pump Shotgun"};
        d.base_descr = {
                "A pump-action shotgun has a handgrip that can be pumped back "
                "and forth in order to eject a spent round of ammunition and "
                "to chamber a fresh one. It has a single barrel above a tube "
                "magazine into which shells are inserted. The magazine has a "
                "capacity of 8 shells."
        };
        d.weight = Weight::medium;
        d.tile = TileId::shotgun;
        d.ranged.is_shotgun = true;
        d.melee.att_msgs = {"strike", "strikes me with a shotgun"};
        d.ranged.max_ammo = 8;
        d.ranged.dmg = DmgRange(6, 18);
        d.ranged.hit_chance_mod = 0;
        d.ranged.effective_range = 5;
        d.ranged.ammo_item_id = Id::shotgun_shell;
        d.ranged.att_msgs = {"fire", "fires a shotgun"};
        d.ranged.snd_msg = "I hear a shotgun blast.";
        d.ranged.att_sfx = SfxId::shotgun_pump_fire ;
        d.ranged.makes_ricochet_snd = true;
        d.ranged.reload_sfx = SfxId::shotgun_reload;
        d.spawn_std_range.min = 2;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ammo);
        d.id = Id::shotgun_shell;
        d.base_name = {"Shotgun shell", "Shotgun shells", "a shotgun shell"};
        d.base_descr = {
                "A cartridge designed to be fired from a shotgun."
        };
        d.max_stack_at_spawn = 10;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d.id = Id::incinerator;
        d.base_name = {"Incinerator", "Incinerators", "an Incinerator"};
        d.base_descr = {
                "This hellish, experimental weapon launches an explosive "
                "fireball. Best used with extreme caution."
        };
        d.weight = (Weight::medium + Weight::heavy) / 2;
        d.tile = TileId::incinerator;
        d.melee.att_msgs = {"strike", "strikes me with an Incinerator"};
        d.ranged.max_ammo = 5;
        d.ranged.dmg = DmgRange(1, 3);
        d.ranged.effective_range = 8;
        d.allow_display_dmg = false;
        d.ranged.ammo_item_id = Id::incinerator_ammo;
        d.ranged.att_msgs = {"fire", "fires an incinerator"};
        d.ranged.snd_msg = "I hear the blast of a launched missile.";
        d.ranged.projectile_character = '*';
        d.ranged.projectile_color = colors::light_red();
        d.ranged.reload_sfx = SfxId::machine_gun_reload;
        d.spawn_std_range.min = g_dlvl_first_mid_game;
        d.chance_to_incl_in_spawn_list = 35;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ammo_mag);
        d.id = Id::incinerator_ammo;
        d.base_name = {
                "Incinerator Cartridge",
                "Incinerator Cartridges",
                "an Incinerator Cartridge"
        };
        d.base_descr = {
                "Ammunition designed for Incinerators."
        };
        d.weight = Weight::light;
        d.spawn_std_range.min = 5;
        d.max_stack_at_spawn = 1;
        d.ranged.max_ammo = g_data[(size_t)Id::incinerator].ranged.max_ammo;
        d.chance_to_incl_in_spawn_list = 15;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d.id = Id::machine_gun;
        d.base_name = {"Tommy Gun", "Tommy Guns", "a Tommy Gun"};
        d.base_descr = {
                "\"Tommy Gun\" is a nickname for the Thompson submachine gun - "
                "an automatic firearm with a drum magazine and vertical "
                "foregrip. It fires .45 ACP ammunition. The drum magazine has "
                "a capacity of 50 rounds."
        };
        d.weight = Weight::medium;
        d.tile = TileId::tommy_gun;
        d.melee.att_msgs = {"strike", "strikes me with a Tommy Gun"};
        d.ranged.is_machine_gun = true;
        d.ranged.max_ammo = 50;
        d.ranged.dmg = DmgRange(2, 4, 2);
        d.ranged.hit_chance_mod = -10;
        d.ranged.effective_range = 8;
        d.ranged.ammo_item_id = Id::drum_of_bullets;
        d.ranged.att_msgs = {"fire", "fires a Tommy Gun"};
        d.ranged.snd_msg = "I hear the burst of a machine gun.";
        d.ranged.att_sfx = SfxId::machine_gun_fire;
        d.ranged.makes_ricochet_snd = true;
        d.ranged.reload_sfx = SfxId::machine_gun_reload;
        d.spawn_std_range.min = 2;
        d.chance_to_incl_in_spawn_list = 75;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ammo_mag);
        d.id = Id::drum_of_bullets;
        d.base_name = {
                "Drum of .45 ACP",
                "Drums of .45 ACP",
                "a Drum of .45 ACP"
        };
        d.base_descr = {
                "Ammunition used by Tommy Guns."
        };
        d.ranged.max_ammo = g_data[(size_t)Id::machine_gun].ranged.max_ammo;
        d.chance_to_incl_in_spawn_list = 50;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d.id = Id::pistol;
        d.base_name = {"M1911 Colt", "M1911 Colt", "an M1911 Colt"};
        d.base_descr = {
                "A semi-automatic, magazine-fed pistol chambered for the .45 "
                "ACP cartridge."
        };
        d.weight = (Weight::light + Weight::medium) / 2;
        d.tile = TileId::pistol;
        d.ranged.max_ammo = 7;
        d.ranged.dmg = DmgRange(1, 8, 4);
        d.ranged.effective_range = 6;
        d.ranged.ammo_item_id = Id::pistol_mag;
        d.melee.att_msgs = {"strike", "strikes me with a pistol"};
        d.ranged.att_msgs = {"fire", "fires a pistol"};
        d.ranged.snd_msg = "I hear a pistol being fired.";
        d.ranged.att_sfx = SfxId::pistol_fire;
        d.ranged.makes_ricochet_snd = true;
        d.ranged.reload_sfx = SfxId::pistol_reload;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ammo_mag);
        d.id = Id::pistol_mag;
        d.base_name = {
                ".45ACP Colt cartridge", ".45ACP Colt cartridges",
                "a .45ACP Colt cartridge"
        };
        d.base_descr = {
                "Ammunition used by Colt pistols."
        };
        d.ranged.max_ammo = g_data[(size_t)Id::pistol].ranged.max_ammo;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d.id = Id::flare_gun;
        d.base_name = {"Flare Gun", "Flare Gun", "a Flare Gun"};
        d.base_descr = {
                "Launches flares. Not designed to function as a weapon."
        };
        d.weight = (Weight::light + Weight::medium) / 2;
        d.tile = TileId::flare_gun;
        d.ranged.max_ammo = 1;
        d.ranged.dmg = DmgRange(1, 3, 0);
        d.ranged.effective_range = 3;
        d.allow_display_dmg = false;
        d.ranged.ammo_item_id = Id::flare;
        d.melee.att_msgs = {"strike", "strikes me with a flare gun"};
        d.ranged.att_msgs = {"fire", "fires a flare gun"};
        d.ranged.snd_msg = "I hear a flare gun being fired.";
        d.ranged.prop_applied = ItemAttProp(new PropFlared());
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d.id = Id::spike_gun;
        d.base_name = {"Spike Gun", "Spike Guns", "a Spike Gun"};
        d.base_descr = {
                "A very strange and crude weapon capable of launching iron "
                "spikes with enough force to pierce flesh (or even rock). It "
                "seems almost to be deliberately designed for cruelty, rather "
                "than pure stopping power."
        };
        d.weight = Weight::medium;
        d.tile = TileId::tommy_gun;
        d.color = colors::dark_brown();
        d.melee.att_msgs = {"strike", "strikes me with a Spike Gun"};
        d.ranged.max_ammo = 12;
        d.ranged.dmg = DmgRange(1, 7, 0);
        d.ranged.hit_chance_mod = 0;
        d.ranged.effective_range = 4;
        d.ranged.dmg_type = DmgType::physical;
        d.ranged.knocks_back = true;
        d.ranged.ammo_item_id = Id::iron_spike;
        d.ranged.att_msgs = {"fire", "fires a Spike Gun"};
        d.ranged.snd_msg = "I hear a very crude weapon being fired.";
        d.ranged.makes_ricochet_snd = true;
        d.ranged.projectile_color = colors::gray();
        d.spawn_std_range.min = 4;
        d.ranged.att_sfx = SfxId::spike_gun;
        d.ranged.snd_vol = SndVol::low;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d.id = Id::mi_go_gun;
        d.base_name = {
                "Electric Gun", "Electric Gun", "an Electric Gun"
        };
        d.base_descr = {
                "A weapon created by the Mi-go. It fires devastating bolts of "
                "electricity.",

                "The weapon does not use ammunition, instead it draws power "
                "from the life force of the wielder (3 Hit Points per attack)."
        };
        d.spawn_std_range = Range(-1, -1);
        d.weight = Weight::medium;
        d.tile = TileId::mi_go_gun;
        d.color = colors::yellow();
        d.ranged.dmg = DmgRange(3, 12, 1);
        d.ranged.hit_chance_mod = 5;
        d.ranged.effective_range = 4;
        {
                auto prop = new PropParalyzed();

                prop->set_duration(2);

                d.ranged.prop_applied = ItemAttProp(prop);
        }
        d.ranged.dmg_type = DmgType::electric;
        d.ranged.has_infinite_ammo = true;
        d.ranged.projectile_leaves_trail = true;
        d.ranged.projectile_color = colors::yellow();
        d.melee.att_msgs = {"strike", "strikes me with a Mi-go Electric Gun"};
        d.ranged.att_msgs = {"fire", "fires a Mi-go Electric Gun"};
        d.ranged.snd_msg = "I hear a bolt of electricity.";
        d.ranged.att_sfx = SfxId::mi_go_gun_fire;
        d.ranged.makes_ricochet_snd = false;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d.id = Id::trap_dart;
        d.allow_spawn = false;
        d.ranged.has_infinite_ammo = true;
        d.ranged.dmg = DmgRange(1, 8);
        d.ranged.hit_chance_mod = 70;
        d.ranged.effective_range = 6;
        d.ranged.snd_msg = "I hear the launching of a projectile.";
        d.ranged.att_sfx = SfxId::END; // TODO: Make a sound effect for this
        d.ranged.makes_ricochet_snd = true;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d = g_data[(size_t)Id::trap_dart];
        d.id = Id::trap_dart_poison;
        d.ranged.prop_applied = ItemAttProp(new PropPoisoned());
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::trap_spear;
        d.allow_spawn = false;
        d.weight = Weight::heavy;
        d.melee.dmg = DmgRange(2, 12);
        d.melee.hit_chance_mod = 85;
        d.melee.dmg_method = DmgMethod::piercing;
        d.melee.hit_small_sfx = SfxId::hit_sharp;
        d.melee.hit_medium_sfx = SfxId::hit_sharp;
        d.melee.miss_sfx = SfxId::miss_heavy;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn);
        d = g_data[(size_t)Id::trap_spear];
        d.id = Id::trap_spear_poison;
        d.ranged.prop_applied = ItemAttProp(new PropPoisoned());
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::explosive);
        d.id = Id::dynamite;
        d.base_name = {"Dynamite", "Sticks of Dynamite", "a Stick of Dynamite"};
        d.base_descr = {
                "An explosive material based on nitroglycerin. The name comes "
                "from the ancient Greek word for \"power\"."
        };
        d.weight = Weight::light;
        d.tile = TileId::dynamite;
        d.color = colors::light_red();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::explosive);
        d.id = Id::flare;
        d.base_name = {"Flare", "Flares", "a Flare"};
        d.base_descr = {
                "A type of pyrotechnic that produces a brilliant light or "
                "intense heat without an explosion."
        };
        d.weight = Weight::light;
        d.tile = TileId::flare;
        d.color = colors::gray();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::explosive);
        d.id = Id::molotov;
        d.base_name = {
                "Molotov Cocktail", "Molotov Cocktails", "a Molotov Cocktail"
        };
        d.base_descr = {
                "An improvised incendiary weapon made of a glass bottle "
                "containing flammable liquid and some cloth for ignition. In "
                "action, the cloth is lit and the bottle hurled at a target, "
                "causing an immediate fireball followed by a raging fire."
        };
        d.weight = Weight::light;
        d.tile = TileId::molotov;
        d.color = colors::white();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::explosive);
        d.id = Id::smoke_grenade;
        d.base_name = {"Smoke Grenade", "Smoke Grenades", "a Smoke Grenade"};
        d.base_descr = {
                "A sheet steel cylinder with emission holes releasing smoke "
                "when the grenade is ignited. Their primary use is to create "
                "smoke screens for concealment. The fumes produced can harm "
                "the eyes, throat and lungs - so it is recommended to wear a "
                "protective mask."
        };
        d.weight = Weight::light;
        d.tile = TileId::flare;
        d.color = colors::green();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::throwing_wpn);
        d.id = Id::thr_knife;
        d.base_name = {"Throwing Knife", "Throwing Knives", "a Throwing Knife"};
        d.base_descr = {
                "A knife specially designed and weighted so that it can be "
                "thrown effectively."
        };
        d.weight = Weight::extra_light;
        d.tile = TileId::dagger;
        d.character = '/';
        d.color = colors::white();
        d.ranged.dmg = DmgRange(2, 6);
        d.ranged.throw_hit_chance_mod = 10;
        d.ranged.effective_range = 5;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.max_stack_at_spawn = 6;
        d.land_on_hard_snd_msg = "I hear a clanking sound.";
        d.land_on_hard_sfx = SfxId::metal_clank;
        d.main_att_mode = AttMode::thrown;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::throwing_wpn);
        d.id = Id::rock;
        d.base_name = {"Rock", "Rocks", "a Rock"};
        d.base_descr = {
                "Although not a very impressive weapon, with skill they can "
                "be used with some result."
        };
        d.weight = Weight::extra_light;
        d.tile = TileId::rock;
        d.character = '*';
        d.color = colors::gray();
        d.ranged.dmg = DmgRange(1, 3);
        d.ranged.effective_range = 4;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.max_stack_at_spawn = 3;
        d.main_att_mode = AttMode::thrown;
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::dagger;
        d.base_name = {"Dagger", "Daggers", "a Dagger"};
        d.base_descr = {
                "Commonly associated with deception, stealth, and treachery. "
                "Many assassinations have been carried out with the use of a "
                "dagger.",

                "Melee attacks performed with a dagger against an unaware "
                "opponent does +300% damage (in addition to the normal +50% "
                "damage from stealth attacks).",

                "Melee attacks with daggers are silent."
        };
        d.weight = Weight::light;
        d.tile = TileId::dagger;
        d.melee.att_msgs = {"stab", "stabs me with a Dagger"};
        d.melee.dmg = DmgRange(1, 4);
        d.melee.hit_chance_mod = 20;
        d.melee.dmg_method = DmgMethod::piercing;
        d.melee.is_noisy = false;
        d.melee.hit_medium_sfx = SfxId::hit_sharp;
        d.melee.hit_hard_sfx = SfxId::hit_sharp;
        d.melee.miss_sfx = SfxId::miss_light;
        d.ranged.throw_hit_chance_mod = -5;
        d.ranged.effective_range = 4;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::tomb);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::hatchet;
        d.base_name = {"Hatchet", "Hatchets", "a Hatchet"};
        d.base_descr = {
                "A small axe with a short handle. Hatchets are reliable "
                "weapons - they are easy to use, and cause decent damage for "
                "their low weight.",

                "Melee attacks with hatchets are silent."
        };
        d.weight = Weight::light;
        d.tile = TileId::axe;
        d.melee.att_msgs = {"strike", "strikes me with a Hatchet"};
        d.melee.dmg = DmgRange(1, 5);
        d.melee.hit_chance_mod = 15;
        d.melee.att_corpse = true;
        d.melee.dmg_method = DmgMethod::slashing;
        d.melee.is_noisy = false;
        d.melee.hit_medium_sfx = SfxId::hit_sharp;
        d.melee.hit_hard_sfx = SfxId::hit_sharp;
        d.melee.miss_sfx = SfxId::miss_light;
        d.ranged.throw_hit_chance_mod = 0;
        d.ranged.effective_range = 5;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::club;
        d.base_name = {"Club", "Clubs", "a Club"};
        d.base_descr = {
                "Wielded since prehistoric times.",

                "Melee attacks with clubs are silent."
        };
        d.spawn_std_range = Range(g_dlvl_first_mid_game, g_dlvl_last);
        d.weight = Weight::medium;
        d.tile = TileId::club;
        d.color = colors::brown();
        d.melee.att_msgs = {"strike", "strikes me with a Club"};
        d.melee.dmg = DmgRange(2, 6);
        d.melee.hit_chance_mod = 10;
        d.melee.att_corpse = true;
        d.melee.dmg_method = DmgMethod::blunt;
        d.melee.is_noisy = false;
        d.melee.miss_sfx = SfxId::miss_medium;
        d.ranged.throw_hit_chance_mod = -5;
        d.ranged.effective_range = 4;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.land_on_hard_snd_msg = "I hear a thudding sound.";
        d.land_on_hard_sfx = SfxId::END;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::hammer;
        d.base_name = {"Hammer", "Hammers", "a Hammer"};
        d.base_descr = {
                "Typically used for construction, but can be quite devastating "
                "when wielded as a weapon.",

                "Melee attacks with hammers are noisy."
        };
        d.weight = Weight::medium;
        d.tile = TileId::hammer;
        d.melee.att_msgs = {"smash", "smashes me with a Hammer"};
        d.melee.dmg = DmgRange(2, 8);
        d.melee.hit_chance_mod = 5;
        d.melee.att_corpse = true;
        d.melee.dmg_method = DmgMethod::blunt;
        d.melee.is_noisy = true;
        d.melee.miss_sfx = SfxId::miss_medium;
        d.ranged.throw_hit_chance_mod = -5;
        d.ranged.effective_range = 4;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::machete;
        d.base_name = {"Machete", "Machetes", "a Machete"};
        d.base_descr = {
                "A large cleaver-like knife. It serves well both as a cutting "
                "tool and weapon.",

                "Melee attacks with machetes are noisy."
        };
        d.weight = Weight::medium;
        d.tile = TileId::machete;
        d.melee.att_msgs = {"chop", "chops me with a Machete"};
        d.melee.dmg = DmgRange(2, 10);
        d.melee.hit_chance_mod = 0;
        d.melee.att_corpse = true;
        d.melee.dmg_method = DmgMethod::slashing;
        d.melee.hit_small_sfx = SfxId::hit_sharp;
        d.melee.hit_medium_sfx = SfxId::hit_sharp;
        d.melee.miss_sfx = SfxId::miss_medium;
        d.melee.is_noisy = true;
        d.ranged.throw_hit_chance_mod = -5;
        d.ranged.effective_range = 4;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::axe;
        d.base_name = {"Axe", "Axes", "an Axe"};
        d.base_descr = {
                "A tool intended for felling trees, splitting timber, etc. "
                "Used as a weapon it can deliver devastating blows, although "
                "it requires some skill to use effectively.",

                "Melee attacks with axes are noisy."
        };
        d.weight = Weight::medium;
        d.tile = TileId::axe;
        d.melee.att_msgs = {"strike", "strikes me with an axe"};
        d.melee.dmg = DmgRange(2, 12);
        d.melee.hit_chance_mod = -5;
        d.melee.att_corpse = true;
        d.melee.att_terrain = true;
        d.melee.dmg_method = DmgMethod::slashing;
        d.melee.miss_sfx = SfxId::miss_medium;
        d.melee.is_noisy = true;
        d.ranged.throw_hit_chance_mod = -5;
        d.ranged.effective_range = 4;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::tomb);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::spiked_mace;
        d.base_name = {"Spiked Mace", "Spiked Maces", "a Spiked Mace"};
        d.base_descr = {
                "A brutal weapon, utilizing a combination of blunt-force and "
                "puncture.",

                "Attacks with this weapon have a 25% chance to stun the "
                "victim, rendering them unable to act for a brief time.",

                "Melee attacks with spiked maces are noisy."
        };
        d.weight = (Weight::medium + Weight::heavy) / 2;
        d.tile = TileId::spiked_mace;
        d.melee.att_msgs = {"strike", "strikes me with a spiked mace"};
        d.melee.dmg = DmgRange(2, 14);
        d.melee.hit_chance_mod = -10;
        d.melee.att_corpse = true;
        d.melee.att_terrain = false;
        d.melee.dmg_method = DmgMethod::piercing;
        d.melee.miss_sfx = SfxId::miss_heavy;
        d.melee.is_noisy = true;
        d.ranged.throw_hit_chance_mod = -5;
        d.ranged.effective_range = 4;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::tomb);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::pitch_fork;
        d.base_name = {"Pitchfork", "Pitchforks", "a Pitchfork"};
        d.base_descr = {
                "A long staff with a forked, four-pronged end.",

                "Pitchforks are useful in keeping attackers at bay (+15% "
                "chance to evade melee attacks, victims are pushed back when "
                "stabbed)."
        };
        d.weight = Weight::heavy;
        d.tile = TileId::pitchfork;
        d.melee.att_msgs = {"strike", "strikes me with a Pitchfork"};
        d.melee.dmg = DmgRange(3, 12);
        d.melee.hit_chance_mod = -15;
        d.melee.att_corpse = true;
        d.melee.knocks_back = true;
        d.melee.dmg_method = DmgMethod::piercing;
        d.melee.is_noisy = true;
        d.melee.hit_small_sfx = SfxId::hit_sharp;
        d.melee.hit_medium_sfx = SfxId::hit_sharp;
        d.melee.miss_sfx = SfxId::miss_heavy;
        d.ranged.throw_hit_chance_mod = -10;
        d.ranged.effective_range = 3;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::sledge_hammer;
        d.base_name = {"Sledgehammer", "Sledgehammers", "a Sledgehammer"};
        d.base_descr = {
                "Often used in destruction work for breaking through walls. "
                "It can deal a great amount of damage, although it is "
                "cumbersome to carry, and it requires some skill to use "
                "effectively."
        };
        d.weight = Weight::heavy;
        d.tile = TileId::sledge_hammer;
        d.melee.att_msgs = {"smash", "smash me with a Sledgehammer"};
        d.melee.dmg = DmgRange(3, 15);
        d.melee.hit_chance_mod = -15;
        d.melee.att_corpse = true;
        d.melee.att_terrain = true;
        d.melee.dmg_method = DmgMethod::blunt;
        d.melee.miss_sfx = SfxId::miss_heavy;
        d.ranged.throw_hit_chance_mod = -10;
        d.ranged.effective_range = 3;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.native_containers.push_back(terrain::Id::cabinet);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::throwing_wpn);
        d.id = Id::iron_spike;
        d.base_name = {"Iron Spike", "Iron Spikes", "an Iron Spike"};
        d.base_descr = {
                "Can be useful for wedging things closed."
                /*TODO: or prying things open."*/
        };
        d.weight = Weight::extra_light;
        d.tile = TileId::iron_spike;
        d.is_stackable = true;
        d.color = colors::gray();
        d.character = '/';
        d.ranged.throw_hit_chance_mod = -5;
        d.ranged.dmg = DmgRange(1, 4);
        d.ranged.effective_range = 3;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.max_stack_at_spawn = 12;
        d.land_on_hard_snd_msg = "I hear a clanking sound.";
        d.land_on_hard_sfx = SfxId::metal_clank;
        d.main_att_mode = AttMode::thrown;
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::player_kick;
        d.melee.att_msgs = {"kick", ""};
        d.melee.hit_chance_mod = 15;
        d.melee.dmg = DmgRange(1, 2);
        d.melee.knocks_back = true;
        d.melee.dmg_method = DmgMethod::kicking;
        d.melee.att_terrain = true;
        d.melee.att_corpse = true;
        d.melee.miss_sfx = SfxId::miss_medium;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::player_stomp;
        d.melee.att_msgs = {"stomp", ""};
        d.melee.hit_chance_mod =
                g_data[(size_t)Id::player_kick].melee.hit_chance_mod;
        d.melee.dmg =
                g_data[(size_t)Id::player_kick].melee.dmg;
        d.melee.miss_sfx =
                g_data[(size_t)Id::player_kick].melee.miss_sfx;
        d.melee.dmg_method = DmgMethod::kicking;
        d.melee.knocks_back = false;
        d.melee.att_terrain = false;
        d.melee.att_corpse = false;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::player_punch;
        d.base_name = {"Punch", "", ""};
        d.melee.att_msgs = {"punch", ""};
        d.melee.hit_chance_mod = 20;
        d.melee.dmg = DmgRange(1, 1);
        d.melee.miss_sfx = SfxId::miss_light;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::player_ghoul_claw;
        d.base_name = {"Claw", "", ""};
        d.melee.att_msgs = {"claw", ""};
        d.melee.hit_chance_mod = 20;
        d.melee.dmg = DmgRange(1, 8);
        d.melee.att_corpse = true;
        d.melee.dmg_method = DmgMethod::slashing;
        d.melee.hit_small_sfx = SfxId::hit_sharp;
        d.melee.hit_medium_sfx = SfxId::hit_sharp;
        d.melee.miss_sfx = SfxId::miss_medium;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_bite;
        d.melee.att_msgs = {"", "bites me"};
        d.melee.dmg_method = DmgMethod::piercing;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_claw;
        d.melee.att_msgs = {"", "claws me"};
        d.melee.dmg_method = DmgMethod::slashing;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_strike;
        d.melee.att_msgs = {"", "strikes me"};
        d.melee.dmg_method = DmgMethod::blunt;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_punch;
        d.melee.att_msgs = {"", "punches me"};
        d.melee.dmg_method = DmgMethod::blunt;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn_intr);
        d.id = Id::intr_acid_spit;
        d.ranged.att_msgs = {"", "spits acid pus at me"};
        d.ranged.snd_msg = "I hear spitting.";
        d.ranged.projectile_color = colors::light_green();
        d.ranged.dmg_type = DmgType::acid;
        d.ranged.projectile_character = '*';
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn_intr);
        d.id = Id::intr_snake_venom_spit;
        d.ranged.att_msgs = {"", "spits venom at me"};
        d.ranged.snd_msg = "I hear hissing and spitting.";
        d.ranged.projectile_color = colors::light_green();
        d.ranged.dmg_type = DmgType::physical;
        d.ranged.projectile_character = '*';
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn_intr);
        d.id = Id::intr_fire_breath;
        d.ranged.att_msgs = {"", "breathes fire at me"};
        d.ranged.snd_msg = "I hear a burst of flames.";
        d.ranged.projectile_color = colors::light_red();
        d.ranged.projectile_character = '*';
        d.ranged.projectile_tile = TileId::blast1;
        d.ranged.projectile_leaves_trail = true;
        d.ranged.dmg_type = DmgType::fire;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn_intr);
        d.id = Id::intr_energy_breath;
        d.ranged.att_msgs = {"", "breathes lightning at me"};
        d.ranged.snd_msg = "I hear a burst of lightning.";
        d.ranged.projectile_color = colors::yellow();
        d.ranged.projectile_character = '*';
        d.ranged.projectile_tile = TileId::blast1;
        d.ranged.projectile_leaves_trail = true;
        d.ranged.dmg_type = DmgType::electric;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_raven_peck;
        d.melee.att_msgs = {"", "pecks at me"};
        d.melee.dmg_method = DmgMethod::piercing;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_vampiric_bite;
        d.melee.att_msgs = {"", "bites me"};
        d.melee.dmg_method = DmgMethod::piercing;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_strangle;
        d.melee.att_msgs = {"", "strangles me"};
        d.melee.dmg_method = DmgMethod::blunt;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_ghost_touch;
        d.melee.att_msgs = {"", "reaches for me"};
        d.melee.dmg_type = DmgType::spirit;
        d.melee.dmg_method = DmgMethod::blunt;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_sting;
        d.melee.att_msgs = {"", "stings me"};
        d.melee.dmg_method = DmgMethod::piercing;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_mind_leech_sting;
        d.melee.att_msgs = {"", "stings me"};
        d.melee.dmg_method = DmgMethod::piercing;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_spirit_leech_sting;
        d.melee.att_msgs = {"", "stings me"};
        d.melee.dmg_method = DmgMethod::piercing;
        d.melee.dmg_type = DmgType::spirit;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_life_leech_sting;
        d.melee.att_msgs = {"", "stings me"};
        d.melee.dmg_method = DmgMethod::piercing;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_spear_thrust;
        d.melee.att_msgs = {"", "strikes me with a spear"};
        d.melee.dmg_method = DmgMethod::piercing;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn_intr);
        d.id = Id::intr_net_throw;
        d.ranged.att_msgs = {"", "throws a net at me"};
        d.ranged.snd_msg = "I hear a whooshing sound.";
        d.ranged.projectile_color = colors::brown();
        d.ranged.projectile_character = '*';
        d.ranged.projectile_tile = TileId::web;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_maul;
        d.melee.att_msgs = {"", "mauls me"};
        d.melee.dmg_method = DmgMethod::blunt;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_pus_spew;
        d.melee.att_msgs = {"", "spews pus on me"};
        d.melee.dmg_method = DmgMethod::blunt;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_acid_touch;
        d.melee.att_msgs = {"", "touches me"};
        d.melee.dmg_type = DmgType::acid;
        d.melee.dmg_method = DmgMethod::elemental;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_dust_engulf;
        d.melee.att_msgs = {"", "engulfs me"};
        d.melee.dmg_method = DmgMethod::elemental;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_fire_engulf;
        d.melee.att_msgs = {"", "engulfs me"};
        d.melee.dmg_method = DmgMethod::elemental;
        d.melee.dmg_type = DmgType::fire;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_energy_engulf;
        d.melee.att_msgs = {"", "engulfs me"};
        d.melee.dmg_method = DmgMethod::elemental;
        d.melee.dmg_type = DmgType::electric;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn_intr);
        d.id = Id::intr_spores;
        d.melee.att_msgs = {"", "releases spores at me"};
        d.melee.dmg_method = DmgMethod::blunt;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::ranged_wpn_intr);
        d.id = Id::intr_web_bola;
        d.ranged.att_msgs = {"", "shoots a web bola at me"};
        d.ranged.snd_msg = "";
        d.ranged.projectile_color = colors::light_white();
        d.ranged.projectile_tile = TileId::blast1;
        d.ranged.projectile_character = '*';
        d.ranged.snd_vol = SndVol::low;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::armor);
        d.id = Id::armor_leather_jacket;
        d.base_name = {"Leather Jacket", "", "a Leather Jacket"};
        d.base_descr = {
                "It offers some protection."
        };
        d.weight = Weight::light;
        d.color = colors::brown();
        d.spawn_std_range.min = 1;
        d.armor.armor_points = 1;
        d.armor.dmg_to_durability_factor = 1.0;
        d.land_on_hard_snd_msg = "";
        d.native_containers.push_back(terrain::Id::cabinet);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::armor);
        d.id = Id::armor_iron_suit;
        d.base_name = {"Iron Suit", "", "an Iron Suit"};
        d.base_descr = {
                "A crude armour constructed from metal plates, bolts, and "
                "leather straps.",

                "It can absorb a high amount of damage, but it makes sneaking "
                "and dodging more difficult.",

                "-20% stealth, -20% dodging."
        };
        d.ability_mods_while_equipped[(size_t)AbilityId::stealth] = -20;
        d.ability_mods_while_equipped[(size_t)AbilityId::dodging] = -20;
        d.weight = Weight::heavy;
        d.color = colors::white();
        d.spawn_std_range.min = 2;
        d.armor.armor_points = 5;
        d.armor.dmg_to_durability_factor = 0.3;
        d.land_on_hard_snd_msg = "I hear a crashing sound.";
        d.native_containers.push_back(terrain::Id::cabinet);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::armor);
        d.id = Id::armor_flak_jacket;
        d.base_name = {"Flak Jacket", "", "a Flak Jacket"};
        d.base_descr = {
                "An armour consisting of steel plates sewn into a waistcoat. "
                "It offers very good protection for its weight. Sneaking and "
                "dodging is slightly more difficult.",

                "-10% stealth, -10% dodging."
        };
        d.ability_mods_while_equipped[(size_t)AbilityId::stealth] = -10;
        d.ability_mods_while_equipped[(size_t)AbilityId::dodging] = -10;
        d.weight = Weight::medium;
        d.color = colors::green();
        d.spawn_std_range.min = 3;
        d.armor.armor_points = 3;
        d.armor.dmg_to_durability_factor = 0.5;
        d.land_on_hard_snd_msg = "I hear a thudding sound.";
        d.native_containers.push_back(terrain::Id::cabinet);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::armor);
        d.id = Id::armor_asb_suit;
        d.base_name = {"Asbestos Suit", "", "an Asbestos Suit"};
        d.base_descr = {
                "A one piece overall of asbestos fabric, including a hood, "
                "furnace mask, gloves and shoes. It protects the wearer "
                "against fire, acid and electricity, and also against smoke, "
                "fumes and gas.",

                "It is a bit bulky, so sneaking and dodging is slightly more "
                "difficult.",

                "-10% stealth, -10% dodging."
        };
        d.ability_mods_while_equipped[(size_t)AbilityId::stealth] = -10;
        d.ability_mods_while_equipped[(size_t)AbilityId::dodging] = -10;
        d.weight = Weight::medium;
        d.color = colors::light_red();
        d.spawn_std_range.min = 3;
        d.armor.armor_points = 1;
        d.armor.dmg_to_durability_factor = 1.0;
        d.land_on_hard_snd_msg = "";
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::chest);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::armor);
        d.id = Id::armor_mi_go;
        d.base_name = {"Mi-go Bio-armor", "", "a Mi-go Bio-armor"};
        d.base_descr = {
                "An extremely durable biological armor created by the Mi-go."
        };
        d.spawn_std_range = Range(-1, -1);
        d.weight = Weight::medium;
        d.color = colors::magenta();
        d.tile = TileId::mi_go_armor;
        d.armor.armor_points = 3;
        d.armor.dmg_to_durability_factor = 0.1;
        d.land_on_hard_snd_msg = "";
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::head_wear);
        d.id = Id::gas_mask;
        d.base_name = {"Gas Mask", "", "a Gas Mask"};
        d.base_descr = {
                "Protects the eyes, throat and lungs from smoke and fumes. It "
                "has a limited useful lifespan that is related to the "
                "absorbent capacity of the filter.",

                "Due to the small eye windows, aiming is slightly more "
                "difficult, and it is harder to detect sneaking enemies and "
                "hidden objects.",

                "-10% melee and ranged hit chance, -6% searching."
        };
        d.ability_mods_while_equipped[(size_t)AbilityId::melee]     = -10;
        d.ability_mods_while_equipped[(size_t)AbilityId::ranged]    = -10;
        d.ability_mods_while_equipped[(size_t)AbilityId::searching] = -6;
        d.is_stackable = false;
        d.color = colors::brown();
        d.tile = TileId::gas_mask;
        d.character = '[';
        d.spawn_std_range = Range(1, g_dlvl_last_early_game);
        d.chance_to_incl_in_spawn_list = 50;
        d.weight = Weight::light;
        d.land_on_hard_snd_msg = "";
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_aura_of_decay;
        d.spell_cast_from_scroll = SpellId::aura_of_decay;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_mayhem;
        d.spell_cast_from_scroll = SpellId::mayhem;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_telep;
        d.spell_cast_from_scroll = SpellId::teleport;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_pest;
        d.spell_cast_from_scroll = SpellId::pestilence;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_enfeeble;
        d.spell_cast_from_scroll = SpellId::enfeeble;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_slow;
        d.spell_cast_from_scroll = SpellId::slow;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_terrify;
        d.spell_cast_from_scroll = SpellId::terrify;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_searching;
        d.spell_cast_from_scroll = SpellId::searching;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_bless;
        d.spell_cast_from_scroll = SpellId::bless;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_darkbolt;
        d.spell_cast_from_scroll = SpellId::darkbolt;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_aza_wrath;
        d.spell_cast_from_scroll = SpellId::aza_wrath;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_opening;
        d.spell_cast_from_scroll = SpellId::opening;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_res;
        d.spell_cast_from_scroll = SpellId::res;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_summon_mon;
        d.spell_cast_from_scroll = SpellId::summon;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_light;
        d.spell_cast_from_scroll = SpellId::light;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_spectral_wpns;
        d.spell_cast_from_scroll = SpellId::spectral_wpns;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_transmut;
        d.spell_cast_from_scroll = SpellId::transmut;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_heal;
        d.spell_cast_from_scroll = SpellId::heal;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_see_invis;
        d.spell_cast_from_scroll = SpellId::see_invis;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_premonition;
        d.spell_cast_from_scroll = SpellId::premonition;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_slow_time;
        d.spell_cast_from_scroll = SpellId::slow_time;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::scroll);
        d.id = Id::scroll_spell_shield;
        d.spell_cast_from_scroll = SpellId::spell_shield;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_vitality;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_spirit;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_blindness;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_fortitude;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_paralyze;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_r_elec;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_conf;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_poison;
        mod_spawn_chance(d, 0.66);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_insight;
        d.chance_to_incl_in_spawn_list = 100;
        d.spawn_std_range.max = g_dlvl_last_mid_game;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_r_fire;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_curing;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_descent;
        mod_spawn_chance(d, 0.15);
        d.spawn_std_range.max = g_dlvl_last_mid_game;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::potion);
        d.id = Id::potion_invis;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::device);
        d.id = Id::device_blaster;
        d.base_name = {"Blaster Device", "Blaster Devices", "a Blaster Device"};
        d.color = colors::gray();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::tomb);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::device);
        d.id = Id::device_rejuvenator;
        d.base_name = {
                "Rejuvenator Device",
                "Rejuvenator Devices",
                "a Rejuvenator Device"
        };
        d.color = colors::gray();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::tomb);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::device);
        d.id = Id::device_translocator;
        d.base_name = {
                "Translocator Device",
                "Translocator Devices",
                "a Translocator Device"
        };
        d.color = colors::gray();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::tomb);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::device);
        d.id = Id::device_sentry_drone;
        d.base_name = {
                "Sentry Drone Device",
                "Sentry Drone Devices",
                "a Sentry Drone Device"
        };
        d.color = colors::gray();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::tomb);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::device);
        d.id = Id::device_deafening;
        d.base_name = {
                "Deafening Device", "Deafening Devices", "a Deafening Device"
        };
        d.color = colors::gray();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::tomb);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::device);
        d.id = Id::device_force_field;
        d.base_name = {
                "Force Field Device",
                "Force Field Devices",
                "a Force Field Device"
        };
        d.color = colors::gray();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::tomb);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::device);
        d.id = Id::lantern;
        d.is_prio_in_backpack_list = true;
        d.base_name = {
                "Electric Lantern", "Electric Lanterns", "an Electric Lantern"
        };
        d.base_descr = {
                "A portable light source."
        };
        d.spawn_std_range = Range(1, 10);
        d.chance_to_incl_in_spawn_list = 100;
        d.is_identified = true;
        d.tile = TileId::lantern;
        d.color = colors::yellow();
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        d.native_containers.push_back(terrain::Id::cocoon);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::rod);
        d.id = Id::rod_curing;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::rod);
        d.id = Id::rod_opening;
        d.spawn_std_range.max = g_dlvl_first_mid_game;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::rod);
        d.id = Id::rod_bless;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::rod);
        d.id = Id::rod_cloud_minds;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::rod);
        d.id = Id::rod_shockwave;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::general);
        d.id = Id::medical_bag;
        d.has_std_activate = true;
        d.is_prio_in_backpack_list = true;
        d.base_name = {"Medical Bag", "Medical Bags", "a Medical Bag"};
        d.base_descr = {
                "A portable bag of medical supplies. Can be used to treat "
                "Wounds or Infections."
        };
        d.weight = Weight::medium;
        d.spawn_std_range = Range(1, g_dlvl_last_mid_game);
        d.is_stackable = false;
        d.character = '%';
        d.color = colors::dark_brown();
        d.tile = TileId::medical_bag;
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::pharaoh_staff;
        d.base_name = {
                "Staff of the Pharaohs",
                "",
                "the Staff of the Pharaohs"
        };
        d.base_descr = {
                "Once wielded by long-forgotten kings in ancient times, this "
                "powerful artifact grants the power to command mummies. "
                "On each turn there is 10% chance that a mummy beholding the "
                "owner will become allied (while the weapon is carried or "
                "wielded)."
        };
        d.color = colors::magenta();
        d.weight = Weight::medium;
        d.tile = TileId::pharaoh_staff;
        d.melee.att_msgs = {
                "strike",
                "strikes me with the Staff of the Pharaohs"
        };
        d.melee.dmg = DmgRange(2, 8, 4);
        d.melee.hit_chance_mod = 0;
        d.melee.miss_sfx = SfxId::miss_medium;
        d.melee.dmg_method = DmgMethod::blunt;
        d.ranged.throw_hit_chance_mod = -10;
        d.ranged.effective_range = 3;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.is_unique = true;
        d.xp_on_found = 20;
        d.value = Value::supreme_treasure;
        d.chance_to_incl_in_spawn_list = 1;
        d.native_containers.push_back(terrain::Id::tomb);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::general);
        d.id = Id::refl_talisman;
        d.base_name = {
                "Talisman of Reflection",
                "",
                "the Talisman of Reflection"
        };
        d.base_descr = {
                "Whenever a hostile spell is blocked due to spell resistance, "
                "it is also reflected. The number of turns to regain spell "
                "resistance is halved."
        };
        d.color = colors::light_blue();
        d.weight = Weight::light;
        d.tile = TileId::amulet;
        d.character = '"';
        d.is_unique = true;
        d.xp_on_found = 20;
        d.value = Value::supreme_treasure;
        d.chance_to_incl_in_spawn_list = 1;
        d.native_containers.push_back(terrain::Id::tomb);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::general);
        d.id = Id::resurrect_talisman;
        d.base_name = {
                "Talisman of Resurrection",
                "",
                "the Talisman of Resurrection"
        };
        d.base_descr = {
                "This powerful charm brings the owner back to life upon bodily "
                "death. The talisman is destroyed in the process however, so "
                "one may only be brought back once."
        };
        d.color = colors::light_white();
        d.weight = Weight::light;
        d.tile = TileId::amulet;
        d.character = '"';
        d.is_unique = true;
        d.xp_on_found = 20;
        d.value = Value::supreme_treasure;
        d.chance_to_incl_in_spawn_list = 1;
        d.native_containers.push_back(terrain::Id::tomb);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::general);
        d.id = Id::tele_ctrl_talisman;
        d.base_name = {
                "Talisman of Teleportation Control",
                "",
                "the Talisman of Teleportation Control"
        };
        d.base_descr = {
                "Grants the owner the ability to control the destination when "
                "teleporting."
        };
        d.color = colors::orange();
        d.weight = Weight::light;
        d.tile = TileId::amulet;
        d.character = '"';
        d.is_unique = true;
        d.xp_on_found = 20;
        d.value = Value::supreme_treasure;
        d.chance_to_incl_in_spawn_list = 1;
        d.native_containers.push_back(terrain::Id::tomb);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::general);
        d.id = Id::clockwork;
        d.base_name = {
                "Arcane Clockwork",
                "",
                "the Arcane Clockwork",
        };
        d.base_descr = {
                "A mainspring-powered clockwork of unreal quality and beauty. "
                "When wound up, it causes the owner to move very swiftly for a "
                "brief time."
        };
        d.color = colors::yellow();
        d.weight = Weight::extra_light;
        d.tile = TileId::clockwork;
        d.character = '%';
        d.is_unique = true;
        d.xp_on_found = 20;
        d.value = Value::supreme_treasure;
        d.has_std_activate = true;
        d.chance_to_incl_in_spawn_list = 1;
        d.native_containers.push_back(terrain::Id::chest);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::general);
        d.id = Id::horn_of_malice;
        d.base_name = {
                "Horn of Malice",
                "",
                "the Horn of Malice"
        };
        d.base_descr = {
                "When blown, this sinister artifact emits a weird resonance "
                "which corrupts the psyche of all those within hearing range "
                "(excluding the horn blower) - causing them to consider all "
                "other creatures with intense hatred and distrust."
        };
        d.color = colors::gray();
        d.weight = Weight::light;
        d.tile = TileId::horn;
        d.character = '%';
        d.is_unique = true;
        d.xp_on_found = 20;
        d.value = Value::supreme_treasure;
        d.has_std_activate = true;
        d.chance_to_incl_in_spawn_list = 1;
        d.native_containers.push_back(terrain::Id::tomb);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::general);
        d.id = Id::horn_of_banishment;
        d.base_name = {
                "Horn of Banishment",
                "",
                "the Horn of Banishment"
        };
        d.base_descr = {
                "When blown, this instrument forces all magically summoned "
                "creatures within hearing range back to their original realm."
        };
        d.color = colors::magenta();
        d.weight = Weight::light;
        d.tile = TileId::horn;
        d.character = '%';
        d.is_unique = true;
        d.xp_on_found = 20;
        d.value = Value::supreme_treasure;
        d.has_std_activate = true;
        d.chance_to_incl_in_spawn_list = 1;
        d.native_containers.push_back(terrain::Id::tomb);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::melee_wpn);
        d.id = Id::spirit_dagger;
        d.base_name = {
                "Spirit Dagger",
                "",
                "the Spirit Dagger"};
        d.base_descr = {
                "A black dagger with elaborate ornaments. The blade appears "
                "blurry, as if perpetually covered in a dark haze.",

                "This weapon does extra damaged based on the current number "
                "of spirit points of the wielder. Successfully attacking an "
                "opponent drains one spirit point from the attacker.",

                "Attacking an unaware opponent with a dagger does +300% damage "
                "(in addition to the normal +50% damage from stealth attacks).",
        };
        d.weight = Weight::light;
        d.tile = TileId::dagger;
        d.color = colors::violet();
        d.melee.att_msgs = {"stab", "stabs me with a Dagger"};
        d.melee.dmg = DmgRange(1, 4);
        d.melee.hit_chance_mod = 20;
        d.melee.is_noisy = false;
        d.melee.hit_medium_sfx = SfxId::hit_sharp;
        d.melee.hit_hard_sfx = SfxId::hit_sharp;
        d.melee.miss_sfx = SfxId::miss_light;
        d.melee.dmg_method = DmgMethod::piercing;
        d.ranged.throw_hit_chance_mod = -5;
        d.ranged.effective_range = 4;
        d.ranged.max_range = d.ranged.effective_range + 3;
        d.is_unique = true;
        d.xp_on_found = 20;
        d.value = Value::supreme_treasure;
        d.chance_to_incl_in_spawn_list = 1;
        d.native_containers.push_back(terrain::Id::tomb);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::general);
        d.id = Id::orb_of_life;
        d.base_name = {
                "Orb of Life",
                "",
                "the Orb of Life"
        };
        d.base_descr = {
                "+4 Hit Points, grants resistance against poison and disease."
        };
        d.color = colors::light_white();
        d.weight = Weight::light;
        d.tile = TileId::orb;
        d.character = '"';
        d.is_unique = true;
        d.xp_on_found = 20;
        d.value = Value::supreme_treasure;
        d.chance_to_incl_in_spawn_list = 1;
        d.native_containers.push_back(terrain::Id::tomb);
        g_data[(size_t)d.id] = d;

        reset_data(d, ItemType::throwing_wpn);
        d.id = Id::zombie_dust;
        d.base_name = {
                "Zombie Dust",
                "Handfuls of Zombie Dust",
                "a handful of Zombie Dust"
        };
        d.base_descr = {
                "When thrown at a living (non-undead) creature, this powder "
                "causes paralyzation."
        };
        d.spawn_std_range.max = g_dlvl_last;
        d.weight = Weight::extra_light;
        d.tile = TileId::zombie_dust;
        d.character = '*';
        d.color = colors::brown();
        d.ranged.dmg = DmgRange(0, 0);
        d.ranged.throw_hit_chance_mod = 15;
        d.ranged.always_break_on_throw = true;
        d.ranged.effective_range = -1;
        d.ranged.max_range = 3;
        d.max_stack_at_spawn = 1;
        d.main_att_mode = AttMode::thrown;
        d.chance_to_incl_in_spawn_list = 35;
        d.native_containers.push_back(terrain::Id::chest);
        d.native_containers.push_back(terrain::Id::cabinet);
        g_data[(size_t)d.id] = d;

        TRACE_FUNC_END;
}

void cleanup()
{
        TRACE_FUNC_BEGIN;

        for (size_t i = 0; i < (size_t)Id::END; ++i)
        {
                ItemData& d = g_data[i];

                d.melee.prop_applied = ItemAttProp();

                d.ranged.prop_applied = ItemAttProp();
        }

        TRACE_FUNC_END;
}

void save()
{
        for (size_t i = 0; i < (size_t)Id::END; ++i)
        {
                const ItemData& d = g_data[i];

                saving::put_bool(d.is_identified);
                saving::put_bool(d.is_alignment_known);
                saving::put_bool(d.is_spell_domain_known);
                saving::put_bool(d.is_tried);
                saving::put_bool(d.is_found);
                saving::put_bool(d.allow_spawn);
                saving::put_int(d.chance_to_incl_in_spawn_list);
        }
}

void load()
{
        for (size_t i = 0; i < (size_t)Id::END; ++i)
        {
                ItemData& d = g_data[i];

                d.is_identified = saving::get_bool();
                d.is_alignment_known = saving::get_bool();
                d.is_spell_domain_known = saving::get_bool();
                d.is_tried = saving::get_bool();
                d.is_found = saving::get_bool();
                d.allow_spawn = saving::get_bool();
                d.chance_to_incl_in_spawn_list = saving::get_int();
        }
}

MeleeData::MeleeData() :
        is_melee_wpn(false),
        dmg(),
        hit_chance_mod(0),
        is_noisy(true),
        att_msgs(ItemAttMsgs()),
        prop_applied(ItemAttProp()),
        dmg_type(DmgType::physical),
        dmg_method(DmgMethod::slashing),
        knocks_back(false),
        att_corpse(false),
        att_terrain(false),
        hit_small_sfx(SfxId::END),
        hit_medium_sfx(SfxId::END),
        hit_hard_sfx(SfxId::END),
        miss_sfx(SfxId::END){}

RangedData::RangedData() :
        is_ranged_wpn(false),
        is_throwable_wpn(false),
        is_machine_gun(false),
        is_shotgun(false),
        max_ammo(0),
        dmg(),
        hit_chance_mod(0),
        throw_hit_chance_mod(0),
        always_break_on_throw(false),
        effective_range(6),
        max_range(g_fov_radi_int * 2),
        knocks_back(false),
        ammo_item_id(Id::END),
        dmg_type(DmgType::physical),
        has_infinite_ammo(false),
        projectile_character('/'),
        projectile_tile(TileId::projectile_std_front_slash),
        projectile_color(colors::white()),
        projectile_leaves_trail(false),
        att_msgs(ItemAttMsgs()),
        snd_msg(""),
        snd_vol(SndVol::low),
        makes_ricochet_snd(false),
        att_sfx(SfxId::END),
        reload_sfx(SfxId::END),
        prop_applied(ItemAttProp())
{

}

ArmorData::ArmorData() :
        armor_points(0),
        dmg_to_durability_factor(0.0) {}

ItemData::ItemData() :
        id(Id::END),
        type(ItemType::general),
        is_intr(false),
        has_std_activate(false),
        is_prio_in_backpack_list(false),
        value(Value::normal),
        weight(Weight::none),
        is_unique(false),
        allow_spawn(true),
        spawn_std_range(Range(1, g_dlvl_last)),
        max_stack_at_spawn(1),
        chance_to_incl_in_spawn_list(100),
        is_stackable(true),
        is_identified(true),
        is_alignment_known(true),
        is_spell_domain_known(true),
        is_tried(false),
        is_found(false),
        xp_on_found(0),
        base_name(),
        character('X'),
        color(colors::white()),
        tile(TileId::END),
        main_att_mode(AttMode::none),
        spell_cast_from_scroll(SpellId::END),
        land_on_hard_snd_msg("I hear a thudding sound."),
        land_on_hard_sfx(SfxId::END),
        allow_display_dmg(true),
        melee(MeleeData()),
        ranged(RangedData()),
        armor(ArmorData())
{
        for (size_t i = 0; i < (size_t)AbilityId::END; ++i)
        {
                ability_mods_while_equipped[i] = 0;
        }

        base_descr.clear();
        native_rooms.clear();
        native_containers.clear();
}

} // item
