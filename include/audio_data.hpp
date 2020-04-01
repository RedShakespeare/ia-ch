// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef AUDIO_DATA_HPP
#define AUDIO_DATA_HPP

#include <string>

namespace audio {

enum class MusId {
        cthulhiana_madness,
        END
};

enum class SfxId {
        // Monster sounds
        dog_snarl,
        wolf_howl,
        hiss,
        zombie_growl,
        ghoul_growl,
        ooze_gurgle,
        flapping_wings,
        ape,

        // Weapon and attack sounds
        hit_small,
        hit_medium,
        hit_hard,
        hit_sharp,
        hit_corpse_break,
        miss_light,
        miss_medium,
        miss_heavy,
        pistol_fire,
        pistol_reload,
        revolver_fire,
        revolver_spin,
        rifle_fire,
        rifle_revolver_reload,
        shotgun_sawed_off_fire,
        shotgun_pump_fire,
        shotgun_reload,
        machine_gun_fire,
        machine_gun_reload,
        mi_go_gun_fire,
        spike_gun,
        bite,

        // Environment action sounds
        metal_clank,
        ricochet,
        explosion,
        explosion_molotov,
        gas,
        door_open,
        door_close,
        door_bang,
        door_break,
        tomb_open,
        fountain_drink,
        boss_voice1,
        boss_voice2,
        chains,
        statue_crash,
        glop,
        lever_pull,
        monolith,
        thunder,
        gong,

        // User interface sounds
        backpack,
        pickup,
        lantern,
        potion_quaff,
        strange_device_activate,
        strange_device_damaged,
        spell_generic,
        spell_shield_break,
        insanity_rise,
        death,
        menu_browse,
        menu_select,

        // Ambient sounds
        // TODO: This is ugly, there is no reason to enumerate the ambient
        // sounds, try to get rid of them (push the ambient audio chunks to a
        // separate vector instead of a fixed size array)
        AMB_START,
        amb001,
        amb002,
        amb003,
        amb004,
        amb005,
        amb006,
        amb007,
        amb008,
        amb009,
        amb010,
        amb011,
        amb012,
        amb013,
        amb014,
        amb015,
        amb016,
        amb017,
        amb018,
        amb019,
        amb020,
        amb021,
        amb022,
        amb023,
        amb024,
        amb025,
        amb026,
        amb027,
        amb028,
        amb029,
        amb030,
        amb031,
        amb032,
        amb033,
        amb034,
        amb035,
        amb036,
        amb037,
        amb038,
        amb039,
        amb040,
        amb041,
        amb042,
        amb043,
        amb044,
        amb045,
        amb046,
        amb047,
        amb048,
        amb049,

        END
};

SfxId str_to_sfx_id(const std::string& str);

std::string sfx_id_to_str(SfxId id);

} // namespace audio

#endif // AUDIO_DATA_HPP
