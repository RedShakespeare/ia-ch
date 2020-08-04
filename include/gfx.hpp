// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef GFX_HPP
#define GFX_HPP

#include <string>

struct P;

namespace gfx {

// NOTE: When updating this, also update the translation tables in the cpp file
enum class TileId {
        aim_marker_head,
        aim_marker_line,
        alchemist_bench_empty,
        alchemist_bench_full,
        altar,
        ammo,
        amoeba,
        amulet,
        ape,
        armor,
        axe,
        barrel,
        bat,
        blast1,
        blast2,
        bog_tcher,
        bookshelf_empty,
        bookshelf_full,
        brain,
        brazier,
        bush,
        byakhee,
        cabinet_closed,
        cabinet_open,
        cave_wall_front,
        cave_wall_top,
        chains,
        chest_closed,
        chest_open,
        chthonian,
        church_bench,
        clockwork,
        club,
        cocoon_closed,
        cocoon_open,
        corpse,
        corpse2,
        crawling_hand,
        crawling_intestines,
        croc_head_mummy,
        crowbar,
        crystal,
        cultist_dagger,
        cultist_firearm,
        cultist_spiked_mace,
        dagger,
        deep_one,
        device1,
        device2,
        door_broken,
        door_closed,
        door_open,
        dynamite,
        dynamite_lit,
        egypt_wall_front,
        egypt_wall_top,
        elder_sign,
        excl_mark,
        fiend,
        flare,
        flare_gun,
        flare_lit,
        floating_skull,
        floor,
        fountain,
        fungi,
        gas_mask,
        gas_spore,
        gate_closed,
        gate_open,
        ghost,
        ghoul,
        giant_spider,
        glasuu,
        gong,
        gore1,
        gore2,
        gore3,
        gore4,
        gore5,
        gore6,
        gore7,
        gore8,
        grate,
        grave_stone,
        hammer,
        hangbridge_hor,
        hangbridge_ver,
        heart,
        holy_symbol,
        horn,
        hound,
        hunting_horror,
        incinerator,
        iron_spike,
        khaga,
        lantern,
        leech,
        leng_elder,
        lever_left,
        lever_right,
        lockpick,
        locust,
        machete,
        mantis,
        mass_of_worms,
        medical_bag,
        mi_go,
        mi_go_armor,
        mi_go_gun,
        molotov,
        monolith,
        mummy,
        ooze,
        orb,
        phantasm,
        pharaoh_staff,
        pillar,
        pillar_broken,
        pillar_carved,
        pistol,
        pit,
        pitchfork,
        player_firearm,
        player_melee,
        polyp,
        potion,
        projectile_std_back_slash,
        projectile_std_dash,
        projectile_std_front_slash,
        projectile_std_vertical_bar,
        pylon,
        rat,
        rat_thing,
        raven,
        revolver,
        rifle,
        ring,
        rock,
        rod,
        rubble_high,
        rubble_low,
        sarcophagus,
        scorched_ground,
        scroll,
        shadow,
        shapeshifter,
        shotgun,
        sledge_hammer,
        smoke,
        snake,
        spider,
        spiked_mace,
        spirit,
        square_checkered,
        square_checkered_sparse,
        stairs_down,
        stalagmite,
        stopwatch,
        tentacles,
        the_high_priest,
        tomb_closed,
        tomb_open,
        tommy_gun,
        trap_general,
        trapez,
        tree,
        tree_fungi,
        vines,
        void_traveler,
        vortex,
        wall_front,
        wall_front_alt1,
        wall_front_alt2,
        wall_top,
        water,
        web,
        weight,
        witch_or_warlock,
        wolf,
        wraith,
        zombie_armed,
        zombie_bloated,
        zombie_dust,
        zombie_unarmed,

        END
};

P character_pos(char character);

TileId str_to_tile_id(const std::string& str);

std::string tile_id_to_str(TileId id);

} // namespace gfx

#endif // GFX_HPP
