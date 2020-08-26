// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "attack.hpp"

#include "actor.hpp"
#include "actor_hit.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "actor_see.hpp"
#include "attack_data.hpp"
#include "drop.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "io.hpp"
#include "item.hpp"
#include "knockback.hpp"
#include "line_calc.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "property_handler.hpp"
#include "terrain.hpp"
#include "terrain_mob.hpp"
#include "terrain_trap.hpp"
#include "text_format.hpp"
#include "viewport.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static const int s_nr_cell_jumps_mg_projectiles = 2;

struct Projectile {
        P pos {0, 0};
        int dmg {0};
        int path_idx {0};
        bool is_dead {false};
        int obstructed_in_path_idx {-1};
        actor::Actor* actor_hit {nullptr};
        terrain::Terrain* terrain_hit {nullptr};
        bool is_seen_by_player {false};
        gfx::TileId tile {gfx::TileId::END};
        char character {-1};
        Color color {colors::white()};
        std::unique_ptr<RangedAttData> att_data {nullptr};

#ifndef NDEBUG
        friend std::ostream& operator<<(
                std::ostream& os,
                const Projectile& p)
        {
                os
                        << "POS: "
                        << "{"
                        << p.pos.x
                        << " ,"
                        << p.pos.y
                        << "}"
                        << " - PATH IDX: "
                        << p.path_idx
                        << " - OBSTRUCTED AT IDX: "
                        << p.obstructed_in_path_idx;

                const auto live_str =
                        p.is_dead
                        ? " - DEAD"
                        : " - LIVE";

                os << live_str;

                if (p.actor_hit) {
                        os
                                << " - ACTOR HIT: "
                                << p.actor_hit->name_a();
                }

                if (p.terrain_hit) {
                        os
                                << " - TERRAIN HIT: "
                                << p.terrain_hit->name(Article::a);
                }

                return os;
        }
#endif // NDEBUG
};

struct ProjectileFireData {
        std::vector<Projectile> projectiles {};
        std::vector<P> path {};
        std::vector<actor::Actor*> actors_seen;
        P origin {0, 0};
        P aim_pos {0, 0};
        actor::Size aim_lvl {(actor::Size)0};
        actor::Actor* attacker = nullptr;
        item::Wpn* wpn = nullptr;
        int animation_delay = 0;
        bool projectile_animation_leaves_trail = false;

#ifndef NDEBUG
        friend std::ostream& operator<<(
                std::ostream& os,
                const ProjectileFireData& d)
        {
                os
                        << "PROJECTILE FIRE DATA:"
                        << std::endl
                        << "ORIGIN: "
                        << "{"
                        << d.origin.x
                        << " ,"
                        << d.origin.y
                        << "}"
                        << std::endl
                        << "AIM POS: "
                        << "{"
                        << d.aim_pos.x
                        << " ,"
                        << d.aim_pos.y
                        << "}"
                        << std::endl
                        << "AIM LVL: "
                        << (int)d.aim_lvl
                        << std::endl;

                os << "PATH: ";

                for (const auto& p : d.path) {
                        os
                                << "{"
                                << p.x
                                << " ,"
                                << p.y
                                << "} ";
                }

                os << std::endl;

                os
                        << "PROJECTILES:"
                        << std::endl;

                for (const auto& proj : d.projectiles) {
                        os << "    " << proj << std::endl;
                }

                return os;
        }
#endif // NDEBUG
};

enum class HitSize {
        small,
        medium,
        hard
};

static size_t nr_projectiles_for_ranged_weapon(const item::Wpn& wpn)
{
        size_t nr_projectiles = 1;

        if (wpn.data().ranged.is_machine_gun) {
                nr_projectiles = g_nr_mg_projectiles;
        }

        return nr_projectiles;
}

// TODO: It would be better to use absolute numbers, rather than relative to
// the weapon's max damage (if a weapon does 100 maximum damage, it would still
// be pretty catastrophic if it did 50 damage - but currently this would only
// count as a "small" hit). Perhaps use values relative to 'min_dng_to_wound'
// (in global.hpp) as thresholds instead.
static HitSize relative_hit_size(const int dmg, const int wpn_max_dmg)
{
        HitSize result = HitSize::small;

        if (wpn_max_dmg >= 4) {
                if (dmg > ((wpn_max_dmg * 5) / 6)) {
                        result = HitSize::hard;
                } else if (dmg > (wpn_max_dmg / 2)) {
                        result = HitSize::medium;
                }
        }

        return result;
}

static HitSize relative_hit_size_melee(const int dmg, const AttData& att_data)
{
        const int max_dmg = att_data.dmg_range.total_range().max;

        return relative_hit_size(dmg, max_dmg);
}

static HitSize relative_hit_size_ranged(const int dmg, const AttData& att_data)
{
        const int max_dmg = att_data.dmg_range.total_range().max;

        return relative_hit_size(dmg, max_dmg);
}

static std::string hit_size_punctuation_str(const HitSize hit_size)
{
        switch (hit_size) {
        case HitSize::small:
                return ".";

        case HitSize::medium:
                return "!";

        case HitSize::hard:
                return "!!!";
        }

        return "";
}

static void print_player_melee_miss_msg()
{
        msg_log::add("I miss.");
}

static void print_mon_melee_miss_msg(const MeleeAttData& att_data)
{
        if (!att_data.defender) {
                ASSERT(false);

                return;
        }

        const bool is_player_seeing_attacker =
                can_player_see_actor(*att_data.attacker);

        const bool is_player_seeing_defender =
                can_player_see_actor(*att_data.defender);

        const bool is_attacker_pos_seen =
                map::g_cells.at(att_data.attacker->m_pos).is_seen_by_player;

        if (!is_attacker_pos_seen ||
            (!is_player_seeing_attacker && !is_player_seeing_defender)) {
                // Attacker position not seen, or two unseen monsters fighting
                // each other - a sound message will be printed
                return;
        }

        std::string attacker_name;

        if (is_player_seeing_attacker) {
                attacker_name =
                        text_format::first_to_upper(
                                att_data.attacker->name_the());
        } else {
                attacker_name = "It";
        }

        std::string defender_name;

        if (att_data.defender->is_player()) {
                defender_name = "me";
        } else {
                if (is_player_seeing_defender) {
                        defender_name = att_data.defender->name_the();
                } else {
                        defender_name = "it";
                }
        }

        const std::string msg =
                attacker_name +
                " misses " +
                defender_name +
                ".";

        const auto interrupt =
                att_data.defender->is_player()
                ? MsgInterruptPlayer::yes
                : MsgInterruptPlayer::no;

        msg_log::add(msg, colors::text(), interrupt);
}

static void print_player_melee_hit_msg(
        const int dmg,
        const MeleeAttData& att_data)
{
        const std::string wpn_verb =
                att_data.att_item->data().melee.att_msgs.player;

        std::string other_name;

        if (can_player_see_actor(*att_data.defender)) {
                other_name = att_data.defender->name_the();
        } else {
                // Player cannot see defender
                other_name = "it";
        }

        const std::string dmg_punct =
                hit_size_punctuation_str(
                        relative_hit_size_melee(
                                dmg,
                                att_data));

        if (att_data.is_intrinsic_att) {
                const std::string att_mod_str =
                        att_data.is_weak_attack
                        ? " feebly"
                        : "";

                msg_log::add(
                        std::string(
                                "I " +
                                wpn_verb +
                                " " +
                                other_name +
                                att_mod_str +
                                dmg_punct),
                        colors::msg_good());
        } else {
                // Not intrinsic attack
                std::string att_mod_str;

                if (att_data.is_weak_attack) {
                        att_mod_str = "feebly ";
                } else if (att_data.is_backstab) {
                        att_mod_str = "covertly ";
                }

                const Color color =
                        att_data.is_backstab
                        ? colors::light_blue()
                        : colors::msg_good();

                const std::string wpn_name_a =
                        att_data.att_item->name(
                                ItemRefType::a,
                                ItemRefInf::none);

                msg_log::add(
                        std::string(
                                "I " +
                                wpn_verb +
                                " " +
                                other_name +
                                " " +
                                att_mod_str +
                                "with " +
                                wpn_name_a +
                                dmg_punct),
                        color);
        }
}

static void print_mon_melee_hit_msg(const int dmg, const MeleeAttData& att_data)
{
        if (!att_data.defender) {
                ASSERT(false);

                return;
        }

        const bool is_player_seeing_attacker =
                can_player_see_actor(*att_data.attacker);

        const bool is_player_seeing_defender =
                can_player_see_actor(*att_data.defender);

        const bool is_attacker_pos_seen =
                map::g_cells.at(att_data.attacker->m_pos).is_seen_by_player;

        if (!is_attacker_pos_seen ||
            (!is_player_seeing_attacker && !is_player_seeing_defender)) {
                // Attacker position not seen, or two unseen monsters fighting
                // each other - a sound message will be printed
                return;
        }

        std::string attacker_name;

        if (is_player_seeing_attacker) {
                attacker_name =
                        text_format::first_to_upper(
                                att_data.attacker->name_the());
        } else {
                attacker_name = "It";
        }

        std::string wpn_verb = att_data.att_item->data().melee.att_msgs.other;

        std::string defender_name;

        if (att_data.defender->is_player()) {
                defender_name = "me";
        } else {
                if (is_player_seeing_defender) {
                        defender_name = att_data.defender->name_the();
                } else {
                        defender_name = "it";
                }
        }

        const std::string dmg_punct =
                hit_size_punctuation_str(
                        relative_hit_size_melee(dmg, att_data));

        std::string used_wpn_str;

        if (!att_data.att_item->data().is_intr &&
            // TODO: This is hacky
            (att_data.attacker->id() != actor::Id::spectral_wpn)) {
                const std::string wpn_name_a =
                        att_data.att_item->name(
                                ItemRefType::a,
                                ItemRefInf::none,
                                ItemRefAttInf::none);

                used_wpn_str = " with " + wpn_name_a;
        }

        const std::string msg =
                attacker_name +
                " " +
                wpn_verb +
                " " +
                defender_name +
                used_wpn_str +
                dmg_punct;

        const auto color =
                att_data.defender->is_player()
                ? colors::msg_bad()
                : colors::text();

        const auto interrupt =
                att_data.defender->is_player()
                ? MsgInterruptPlayer::yes
                : MsgInterruptPlayer::no;

        msg_log::add(msg, color, interrupt);
}

static void print_no_attacker_hit_player_melee_msg(
        const int dmg,
        const MeleeAttData& att_data)
{
        const std::string dmg_punct =
                hit_size_punctuation_str(
                        relative_hit_size_melee(
                                dmg,
                                att_data));

        msg_log::add(
                "I am hit" + dmg_punct,
                colors::msg_bad(),
                MsgInterruptPlayer::yes);
}

static void print_no_attacker_hit_mon_melee_msg(
        const int dmg,
        const MeleeAttData& att_data)
{
        const std::string other_name =
                text_format::first_to_upper(
                        att_data.defender->name_the());

        Color msg_color = colors::msg_good();

        if (map::g_player->is_leader_of(att_data.defender)) {
                // Monster is allied to player, use a neutral color
                // instead (we do not use red color here, since that
                // is reserved for player taking damage).
                msg_color = colors::white();
        }

        const std::string dmg_punct =
                hit_size_punctuation_str(
                        relative_hit_size_melee(
                                dmg,
                                att_data));

        msg_log::add(
                other_name + " is hit" + dmg_punct,
                msg_color,
                MsgInterruptPlayer::yes);
}

static void print_melee_miss_msg(const MeleeAttData& att_data)
{
        if (!att_data.attacker) {
                ASSERT(false);

                return;
        }

        if (att_data.attacker->is_player()) {
                print_player_melee_miss_msg();
        } else {
                print_mon_melee_miss_msg(att_data);
        }
}

static void print_melee_hit_msg(const int dmg, const MeleeAttData& att_data)
{
        if (!att_data.defender) {
                ASSERT(false);

                return;
        }

        if (att_data.attacker) {
                if (att_data.attacker->is_player()) {
                        print_player_melee_hit_msg(dmg, att_data);
                } else {
                        print_mon_melee_hit_msg(dmg, att_data);
                }
        } else {
                // No attacker (e.g. trap attack)
                if (att_data.defender->is_player()) {
                        print_no_attacker_hit_player_melee_msg(dmg, att_data);
                } else if (can_player_see_actor(*att_data.defender)) {
                        print_no_attacker_hit_mon_melee_msg(dmg, att_data);
                }
        }
}

static audio::SfxId melee_hit_sfx(const int dmg, const MeleeAttData& att_data)
{
        const auto hit_size = relative_hit_size_melee(dmg, att_data);

        switch (hit_size) {
        case HitSize::small:
                return att_data.att_item->data().melee.hit_small_sfx;

        case HitSize::medium:
                return att_data.att_item->data().melee.hit_medium_sfx;

        case HitSize::hard:
                return att_data.att_item->data().melee.hit_hard_sfx;
        }

        return audio::SfxId::END;
}

static AlertsMon is_melee_snd_alerting_mon(
        const actor::Actor* const attacker,
        const item::Item& wpn)
{
        const bool is_wpn_noisy = wpn.data().melee.is_noisy;

        if (is_wpn_noisy) {
                const bool is_player_silent =
                        player_bon::has_trait(Trait::silent);

                const bool is_player = (attacker == map::g_player);

                if (is_player && is_player_silent) {
                        return AlertsMon::no;
                } else {
                        return AlertsMon::yes;
                }
        } else {
                return AlertsMon::no;
        }
}

static void print_melee_msg(
        const ActionResult att_result,
        const int dmg,
        const MeleeAttData& att_data)
{
        if (!att_data.defender) {
                ASSERT(false);

                return;
        }

        if (att_result <= ActionResult::fail) {
                print_melee_miss_msg(att_data);
        } else {
                print_melee_hit_msg(dmg, att_data);
        }
}

static std::string melee_snd_msg(const MeleeAttData& att_data)
{
        std::string snd_msg;

        // Only print a message if player is not involved
        if ((att_data.defender != map::g_player) &&
            (att_data.attacker != map::g_player)) {
                // TODO: This message is not appropriate for traps
                snd_msg = "I hear fighting.";
        }

        return snd_msg;
}

static void emit_melee_snd(
        const ActionResult att_result,
        const int dmg,
        const MeleeAttData& att_data)
{
        if (!att_data.defender) {
                ASSERT(false);

                return;
        }

        const auto snd_alerts_mon =
                is_melee_snd_alerting_mon(
                        att_data.attacker,
                        *att_data.att_item);

        const std::string snd_msg = melee_snd_msg(att_data);

        audio::SfxId sfx = audio::SfxId::END;

        if (att_result <= ActionResult::fail) {
                sfx = att_data.att_item->data().melee.miss_sfx;
        } else {
                sfx = melee_hit_sfx(dmg, att_data);
        }

        const auto origin =
                att_data.attacker
                ? att_data.attacker->m_pos
                : att_data.defender->m_pos;

        auto ignore_msg_if_origin_seeen = IgnoreMsgIfOriginSeen::yes;

        if (att_data.attacker) {
                const bool is_player_seeing_defender =
                        can_player_see_actor(*att_data.defender);

                const bool is_player_seeing_attacker =
                        can_player_see_actor(*att_data.attacker);

                if (!is_player_seeing_attacker && !is_player_seeing_defender) {
                        // Two unseen monsters fighting each other - always
                        // include the sound message
                        ignore_msg_if_origin_seeen = IgnoreMsgIfOriginSeen::no;
                }
        }

        Snd snd(
                snd_msg,
                sfx,
                ignore_msg_if_origin_seeen,
                origin,
                att_data.attacker,
                SndVol::low,
                snd_alerts_mon);

        snd.run();
}

static void print_player_fire_ranged_msg(const item::Wpn& wpn)
{
        const std::string attack_verb = wpn.data().ranged.att_msgs.player;

        msg_log::add("I " + attack_verb + ".");
}

static void print_mon_fire_ranged_msg(const RangedAttData& att_data)
{
        if (!can_player_see_actor(*att_data.attacker)) {
                return;
        }

        const std::string attacker_name_the =
                text_format::first_to_upper(
                        att_data.attacker->name_the());

        const std::string attack_verb =
                att_data.att_item->data().ranged.att_msgs.other;

        std::string wpn_used_str;

        if (!att_data.att_item->data().is_intr) {
                const std::string wpn_name_a =
                        att_data.att_item->name(
                                ItemRefType::a,
                                ItemRefInf::none,
                                ItemRefAttInf::none);

                wpn_used_str = " " + wpn_name_a;
        }

        const std::string msg =
                attacker_name_the +
                " " +
                attack_verb +
                wpn_used_str +
                ".";

        const auto interrupt =
                (att_data.defender == map::g_player)
                ? MsgInterruptPlayer::yes
                : MsgInterruptPlayer::no;

        msg_log::add(msg, colors::text(), interrupt);
}

static void print_ranged_fire_msg(
        const RangedAttData& att_data,
        const item::Wpn& wpn)
{
        if (!att_data.attacker) {
                // No attacker actor (e.g. a trap firing a dart)
                return;
        }

        if (att_data.attacker == map::g_player) {
                print_player_fire_ranged_msg(wpn);
        } else {
                print_mon_fire_ranged_msg(att_data);
        }
}

static void print_projectile_hit_player_msg(const Projectile& projectile)
{
        const std::string dmg_punct =
                hit_size_punctuation_str(
                        relative_hit_size_ranged(
                                projectile.dmg,
                                *projectile.att_data));

        msg_log::add(
                "I am hit" + dmg_punct,
                colors::msg_bad(),
                MsgInterruptPlayer::yes);
}

static void print_projectile_hit_mon_msg(const Projectile& projectile)
{
        std::string other_name = "It";

        const auto& defender = *projectile.att_data->defender;

        if (can_player_see_actor(defender)) {
                other_name =
                        text_format::first_to_upper(
                                defender.name_the());
        }

        const std::string dmg_punct =
                hit_size_punctuation_str(
                        relative_hit_size_ranged(
                                projectile.dmg,
                                *projectile.att_data));

        msg_log::add(other_name + " is hit" + dmg_punct, colors::msg_good());
}

static void print_projectile_hit_actor_msg(const Projectile& projectile)
{
        ASSERT(projectile.att_data->defender);

        if (projectile.att_data->defender->is_player()) {
                print_projectile_hit_player_msg(projectile);
        } else {
                // Defender is monster
                const P& pos = projectile.att_data->defender->m_pos;

                if (!map::g_cells.at(pos).is_seen_by_player) {
                        return;
                }

                print_projectile_hit_mon_msg(projectile);
        }
}

static std::unique_ptr<Snd> ranged_fire_snd(
        const AttData& att_data,
        const item::Wpn& wpn,
        const P& origin)
{
        std::unique_ptr<Snd> snd;

        const std::string snd_msg = wpn.data().ranged.snd_msg;

        if (!snd_msg.empty()) {
                const audio::SfxId sfx = wpn.data().ranged.att_sfx;

                const SndVol vol = wpn.data().ranged.snd_vol;

                std::string snd_msg_used = snd_msg;

                if (att_data.attacker == map::g_player) {
                        snd_msg_used = "";
                }

                snd =
                        std::make_unique<Snd>(
                                snd_msg_used,
                                sfx,
                                IgnoreMsgIfOriginSeen::yes,
                                origin,
                                att_data.attacker,
                                vol,
                                AlertsMon::yes);
        }

        return snd;
}

static void emit_projectile_hit_actor_snd(const P& pos)
{
        Snd snd(
                "A creature is hit.",
                audio::SfxId::hit_small,
                IgnoreMsgIfOriginSeen::yes,
                pos,
                nullptr,
                SndVol::low,
                AlertsMon::no);

        snd.run();
}

static void emit_projectile_hit_terrain_snd(
        const P& pos,
        const item::Wpn& wpn)
{
        if (wpn.data().ranged.makes_ricochet_snd) {
                // TODO: Check hit material, soft and wood should not cause
                // a ricochet sound
                Snd snd(
                        "I hear a ricochet.",
                        audio::SfxId::ricochet,
                        IgnoreMsgIfOriginSeen::yes,
                        pos,
                        nullptr,
                        SndVol::low,
                        AlertsMon::yes);

                snd_emit::run(snd);
        }
}

static actor::Actor* get_actor_hit_by_projectile(
        const ActionResult att_result,
        const Projectile& projectile)
{
        const auto& att_data = *projectile.att_data;

        if (!att_data.defender) {
                return nullptr;
        }

        const bool is_actor_aimed_for = (projectile.pos == att_data.aim_pos);

        const bool can_hit_height =
                (att_data.defender_size >= actor::Size::humanoid) ||
                is_actor_aimed_for;

        if (!projectile.is_dead &&
            (att_result >= ActionResult::success) &&
            can_hit_height) {
                return att_data.defender;
        }

        return nullptr;
}

static terrain::Terrain* get_terrain_blocking_projectile(const P& pos)
{
        auto* terrain = map::g_cells.at(pos).terrain;

        if (!terrain->is_projectile_passable()) {
                return terrain;
        }

        for (auto* const mob : game_time::g_mobs) {
                if (!mob->is_projectile_passable()) {
                        return mob;
                }
        }

        return nullptr;
}

static terrain::Terrain* get_ground_blocking_projectile(
        const Projectile& projectile)
{
        // Hit the ground?
        const auto& att_data = *projectile.att_data;

        const auto& pos = projectile.pos;

        const bool has_hit_ground =
                (pos == att_data.aim_pos) &&
                (att_data.aim_lvl == actor::Size::floor);

        if (has_hit_ground) {
                return map::g_cells.at(pos).terrain;
        } else {
                return nullptr;
        }
}

static void try_apply_attack_property_on_actor(
        const ItemAttProp& att_prop,
        actor::Actor& actor,
        const DmgType& dmg_type)
{
        if (!rnd::percent(att_prop.pct_chance_to_apply)) {
                return;
        }

        const bool is_resisting_dmg =
                actor.m_properties.is_resisting_dmg(
                        dmg_type,
                        Verbose::no);

        if (!is_resisting_dmg) {
                auto* const prop_cpy =
                        property_factory::make(
                                att_prop.prop->id());

                const auto duration_mode =
                        att_prop.prop->duration_mode();

                if (duration_mode == PropDurationMode::specific) {
                        prop_cpy->set_duration(
                                att_prop.prop->nr_turns_left());
                } else if (duration_mode == PropDurationMode::indefinite) {
                        prop_cpy->set_indefinite();
                }

                actor.m_properties.apply(prop_cpy);
        }
}

static void hit_actor_with_projectile(
        const Projectile& projectile,
        item::Wpn& wpn)
{
        ASSERT(projectile.actor_hit);

        if (!projectile.actor_hit) {
                return;
        }

        const AttData& att_data = *projectile.att_data;

        if (att_data.attacker == map::g_player) {
                auto& mon = static_cast<actor::Mon&>(*att_data.defender);
                mon.set_player_aware_of_me();
        }

        if (projectile.dmg > 0) {
                actor::hit(
                        *projectile.actor_hit,
                        projectile.dmg,
                        wpn.data().ranged.dmg_type,
                        AllowWound::yes);
        }

        // NOTE: This is run regardless of if the defender died or not
        wpn.on_ranged_hit(*projectile.actor_hit);

        wpn.on_projectile_blocked(projectile.pos);

        if (projectile.actor_hit->is_alive()) {
                auto att_prop =
                        wpn.prop_applied_on_ranged(
                                projectile.att_data->attacker);

                if (att_prop.prop) {
                        try_apply_attack_property_on_actor(
                                att_prop,
                                *projectile.actor_hit,
                                wpn.data().ranged.dmg_type);
                }

                // Knock-back?
                if (wpn.data().ranged.knocks_back &&
                    att_data.attacker) {
                        const bool is_spike_gun =
                                wpn.data().id == item::Id::spike_gun;

                        knockback::run(
                                *(att_data.defender),
                                att_data.attacker->m_pos,
                                is_spike_gun);
                }
        }
}

static void hit_terrain_with_projectile(
        const P& projectile_pos,
        const P& terrain_pos,
        item::Wpn& wpn)
{
        (void)terrain_pos;

        // TODO: This was in the 'shotgun' function (but only the shotgun was
        // calling the terrain 'hit' method in attack.cpp - the normal
        // projectile firing did not do this). Reimplement somehow.
        // cell.terrain->hit(
        //         att_data.dmg,
        //         DmgType::physical,
        //         DmgMethod::shotgun,
        //         nullptr);

        wpn.on_projectile_blocked(projectile_pos);
}

static void init_projectiles_gfx(ProjectileFireData& fire_data)
{
        ASSERT(fire_data.wpn);
        ASSERT(!fire_data.path.empty());

        if (!fire_data.wpn || fire_data.path.empty()) {
                return;
        }

        fire_data.projectile_animation_leaves_trail =
                fire_data.wpn->data().ranged.projectile_leaves_trail;

        char projectile_character =
                fire_data.wpn->data().ranged.projectile_character;

        if (projectile_character == '/') {
                const auto& path = fire_data.path;
                const P& origin = fire_data.origin;

                const int i = (path.size() > 2) ? 2 : 1;

                const P& ref_pos = path[i];

                if (ref_pos.y == origin.y) {
                        projectile_character = '-';
                } else if (ref_pos.x == origin.x) {
                        projectile_character = '|';
                } else if (
                        ((ref_pos.x > origin.x) && (ref_pos.y < origin.y)) ||
                        ((ref_pos.x < origin.x) && (ref_pos.y > origin.y))) {
                        projectile_character = '/';
                } else if (
                        ((ref_pos.x > origin.x) && (ref_pos.y > origin.y)) ||
                        ((ref_pos.x < origin.x) && (ref_pos.y < origin.y))) {
                        projectile_character = '\\';
                }
        }

        auto projectile_tile = fire_data.wpn->data().ranged.projectile_tile;

        if (projectile_tile == gfx::TileId::projectile_std_front_slash) {
                if (projectile_character == '-') {
                        projectile_tile = gfx::TileId::projectile_std_dash;
                } else if (projectile_character == '|') {
                        projectile_tile = gfx::TileId::projectile_std_vertical_bar;
                } else if (projectile_character == '\\') {
                        projectile_tile = gfx::TileId::projectile_std_back_slash;
                }
        }

        const auto projectile_color =
                fire_data.wpn->data().ranged.projectile_color;

        for (auto& projectile : fire_data.projectiles) {
                projectile.tile = projectile_tile;
                projectile.character = projectile_character;
                projectile.color = projectile_color;
        }
}

static void init_projectiles_animation_delay(ProjectileFireData& fire_data)
{
        const int denom = fire_data.projectiles.size();

        fire_data.animation_delay = (config::delay_projectile_draw() / denom);
}

static ProjectileFireData init_projectiles_fire_data(
        actor::Actor* const attacker,
        item::Wpn& wpn,
        const P& origin,
        const P& aim_pos)
{
        ProjectileFireData fire_data;

        fire_data.attacker = attacker;
        fire_data.wpn = &wpn;
        fire_data.origin = origin;
        fire_data.aim_pos = aim_pos;

        const size_t nr_projectiles = nr_projectiles_for_ranged_weapon(wpn);

        fire_data.projectiles.resize(nr_projectiles);

        for (size_t i = 0; i < nr_projectiles; ++i) {
                auto& proj = fire_data.projectiles[i];

                // Projectile path indexes are initially set up so that all
                // projectiles start with a negative index (they are queued up
                // to enter the path)
                proj.path_idx = -((int)i * s_nr_cell_jumps_mg_projectiles) - 1;

                proj.att_data =
                        std::make_unique<RangedAttData>(
                                fire_data.attacker,
                                fire_data.origin,
                                fire_data.aim_pos,
                                fire_data.origin, // Current position
                                *fire_data.wpn,
                                std::nullopt); // Undefined aim level
        }

        fire_data.aim_lvl = fire_data.projectiles[0].att_data->aim_lvl;

        const bool stop_at_target = (fire_data.aim_lvl == actor::Size::floor);

        fire_data.path =
                line_calc::calc_new_line(
                        fire_data.origin,
                        fire_data.aim_pos,
                        stop_at_target,
                        999,
                        false);

        init_projectiles_gfx(fire_data);

        init_projectiles_animation_delay(fire_data);

        return fire_data;
}

static bool is_all_projectiles_dead(const std::vector<Projectile>& projectiles)
{
        for (const auto& projectile : projectiles) {
                if (!projectile.is_dead) {
                        return false;
                }
        }

        return true;
}

static void run_projectile_hits(ProjectileFireData& fire_data)
{
        for (auto& projectile : fire_data.projectiles) {
                if (projectile.is_dead) {
                        continue;
                }

                if (projectile.actor_hit) {
                        hit_actor_with_projectile(projectile, *fire_data.wpn);

                        projectile.is_dead = true;

                        continue;
                }

                if (projectile.terrain_hit) {
                        const int prev_idx = projectile.path_idx - 1;

                        ASSERT(prev_idx >= 0);

                        if (prev_idx < 0) {
                                continue;
                        }

                        P current_pos;
                        P terrain_pos;

                        if (projectile.terrain_hit->is_projectile_passable()) {
                                // The terrain does not block projectiles -
                                // assuming this is floor
                                current_pos = projectile.pos;
                                terrain_pos = current_pos;
                        } else {
                                // The terrain blocks projectiles (e.g. a wall),
                                // do not consider the projectile to be inside
                                // the terrain, but in the cell before it
                                current_pos = fire_data.path[prev_idx];
                                terrain_pos = projectile.pos;
                        }

                        hit_terrain_with_projectile(
                                current_pos,
                                terrain_pos,
                                *fire_data.wpn);

                        projectile.is_dead = true;

                        continue;
                }
        }
}

static void advance_projectiles_on_path(ProjectileFireData& fire_data)
{
        for (auto& projectile : fire_data.projectiles) {
                if (projectile.is_dead) {
                        continue;
                }

                if (projectile.path_idx == (int)fire_data.path.size() - 1) {
                        // Projectile is already at the maximum path index.
                        // This situation is unexpected - consider it a critical
                        // error in debug mode, and just kill the projectile in
                        // release mode.
#ifndef NDEBUG
                        TRACE << fire_data;

                        ASSERT(false);
#endif // NDEBUG

                        projectile.is_dead = true;

                        continue;
                }

                ++projectile.path_idx;

                if (projectile.path_idx >= 0) {
                        projectile.pos = fire_data.path[projectile.path_idx];
                }
        }
}

static void update_projectile_states(ProjectileFireData& fire_data)
{
        advance_projectiles_on_path(fire_data);

        for (auto& projectile : fire_data.projectiles) {
                if (projectile.is_dead ||
                    (projectile.path_idx < 1)) {
                        continue;
                }

                const auto& projectile_pos =
                        fire_data.path[projectile.path_idx];

                projectile.att_data =
                        std::make_unique<RangedAttData>(
                                fire_data.attacker,
                                fire_data.origin,
                                fire_data.aim_pos,
                                projectile_pos,
                                *fire_data.wpn,
                                fire_data.aim_lvl);

                if (projectile.att_data->defender) {
                        // The projectile is at a potential target to attack,
                        // store information about an encountered actor.
                        fire_data.actors_seen.push_back(
                                projectile.att_data->defender);
                }

                const auto att_result =
                        ability_roll::roll(projectile.att_data->hit_chance_tot);

                projectile.dmg =
                        projectile.att_data->dmg_range.total_range().roll();

                projectile.is_seen_by_player =
                        map::g_cells.at(projectile_pos)
                                .is_seen_by_player;

                // Projectile out of weapon max range?
                const int max_range = fire_data.wpn->data().ranged.max_range;

                if (king_dist(fire_data.origin, projectile_pos) > max_range) {
                        projectile.is_dead = true;
                        projectile.obstructed_in_path_idx = projectile.path_idx;

                        continue;
                }

                projectile.actor_hit =
                        get_actor_hit_by_projectile(
                                att_result,
                                projectile);

                if (projectile.actor_hit) {
                        projectile.obstructed_in_path_idx = projectile.path_idx;
                        projectile.color = colors::light_red();

                        continue;
                }

                projectile.terrain_hit =
                        get_terrain_blocking_projectile(projectile.pos);

                if (projectile.terrain_hit) {
                        projectile.obstructed_in_path_idx = projectile.path_idx;
                        projectile.color = colors::yellow();

                        continue;
                }

                projectile.terrain_hit =
                        get_ground_blocking_projectile(projectile);

                if (projectile.terrain_hit) {
                        projectile.obstructed_in_path_idx = projectile.path_idx;

                        const auto terrain_id = projectile.terrain_hit->id();

                        if ((terrain_id == terrain::Id::liquid_shallow) ||
                            (terrain_id == terrain::Id::liquid_deep)) {
                                projectile.color =
                                        projectile.terrain_hit->color();
                        } else {
                                projectile.color =
                                        colors::yellow();
                        }

                        continue;
                }
        }
}

static void run_projectiles_messages_and_sounds(
        const ProjectileFireData& fire_data)
{
        for (auto& projectile : fire_data.projectiles) {
                if (projectile.is_dead) {
                        continue;
                }

                // Projectile entering the path this update?
                if (projectile.path_idx == 0) {
                        // NOTE: The initial attack sound(s) must NOT alert
                        // monsters, since this would immediately make them
                        // aware before any attack data is set. This would
                        // result in the player not getting ranged attack
                        // bonuses against unaware monsters.
                        // An extra sound is run when the attack ends (without a
                        // message or audio), which may alert monsters.
                        auto snd =
                                ranged_fire_snd(
                                        *projectile.att_data,
                                        *fire_data.wpn,
                                        fire_data.origin);

                        if (snd) {
                                snd->set_alerts_mon(AlertsMon::no);

                                snd->run();
                        }

                        continue;
                }

                if (projectile.actor_hit) {
                        emit_projectile_hit_actor_snd(projectile.pos);

                        print_projectile_hit_actor_msg(projectile);

                        continue;
                }

                if (projectile.terrain_hit) {
                        emit_projectile_hit_terrain_snd(
                                projectile.pos,
                                *fire_data.wpn);
                }
        }
}

static void draw_projectile(const Projectile& projectile)
{
        if (projectile.tile == gfx::TileId::END) {
                return;
        }

        if (!viewport::is_in_view(projectile.pos)) {
                return;
        }

        io::draw_symbol(
                projectile.tile,
                projectile.character,
                Panel::map,
                viewport::to_view_pos(projectile.pos),
                projectile.color);
}

static void draw_projectile_hit(
        Projectile& projectile,
        const int animation_delay)
{
        if (config::is_tiles_mode()) {
                projectile.tile = gfx::TileId::blast1;

                draw_projectile(projectile);

                io::update_screen();

                io::sleep(animation_delay / 2);

                projectile.tile = gfx::TileId::blast2;

                draw_projectile(projectile);

                io::update_screen();

                io::sleep(animation_delay / 2);
        } else {
                // Text mode
                projectile.character = '*';

                draw_projectile(projectile);

                io::update_screen();

                io::sleep(animation_delay);
        }
}

static bool should_draw_projectile_as_travelling(const Projectile& projectile)
{
        return !projectile.is_dead &&
                projectile.is_seen_by_player &&
                (projectile.path_idx >= 1);
}

static bool should_draw_projectile_as_hit(const Projectile& projectile)
{
        return !projectile.is_dead &&
                projectile.is_seen_by_player &&
                (projectile.obstructed_in_path_idx >= 0);
}

static void draw_projectiles(ProjectileFireData& fire_data)
{
        if (!fire_data.projectile_animation_leaves_trail) {
                states::draw();
        }

        // Draw travelling projectiles
        for (auto& projectile : fire_data.projectiles) {
                if (!should_draw_projectile_as_travelling(projectile)) {
                        continue;
                }

                draw_projectile(projectile);
        }

        io::update_screen();

        // Draw projectiles hitting something
        for (auto& projectile : fire_data.projectiles) {
                if (!should_draw_projectile_as_hit(projectile)) {
                        continue;
                }

                draw_projectile_hit(projectile, fire_data.animation_delay);
        }
}

static bool is_any_projectile_seen(const std::vector<Projectile>& projectiles)
{
        for (const auto& projectile : projectiles) {
                if (projectile.is_seen_by_player) {
                        return true;
                }
        }

        return false;
}

static ProjectileFireData fire_projectiles(
        actor::Actor* const attacker,
        item::Wpn& wpn,
        const P& origin,
        const P& aim_pos)
{
        auto fire_data =
                init_projectiles_fire_data(
                        attacker,
                        wpn,
                        origin,
                        aim_pos);

        print_ranged_fire_msg(
                *fire_data.projectiles[0].att_data,
                *fire_data.wpn);

        while (true) {
                if (is_all_projectiles_dead(fire_data.projectiles)) {
                        // Run a sound without message or audio, which can alert
                        // monsters (the initial fire sound is not allowed to
                        // alert monsters, since this would prevent ranged
                        // attack bonuses against unaware monsters)
                        const auto& att_data =
                                *fire_data.projectiles[0].att_data;

                        auto snd =
                                ranged_fire_snd(
                                        att_data,
                                        *fire_data.wpn,
                                        fire_data.origin);

                        if (snd) {
                                snd->clear_msg();

                                snd->clear_sfx();

                                snd->run();
                        }

                        return fire_data;
                }

                update_projectile_states(fire_data);

                run_projectiles_messages_and_sounds(fire_data);

                draw_projectiles(fire_data);

                run_projectile_hits(fire_data);

                if (is_any_projectile_seen(fire_data.projectiles)) {
                        io::sleep(fire_data.animation_delay);
                }
        }

        return {};
}

static void melee_hit_actor(
        const int dmg,
        actor::Actor& defender,
        const actor::Actor* const attacker,
        const P& attacker_origin,
        item::Wpn& wpn,
        const MeleeAttData& att_data)
{
        const bool is_ranged_wpn =
                wpn.data().type == ItemType::ranged_wpn;

        const AllowWound allow_wound =
                is_ranged_wpn
                ? AllowWound::no
                : AllowWound::yes;

        const auto dmg_type = wpn.data().melee.dmg_type;

        actor::hit(defender, dmg, dmg_type, allow_wound);

        if (defender.m_data->can_bleed &&
            (is_physical_dmg_type(dmg_type) ||
             (dmg_type == DmgType::pure) ||
             (dmg_type == DmgType::light))) {
                map::make_blood(defender.m_pos);
        }

        wpn.on_melee_hit(defender, dmg);

        if (defender.is_alive()) {
                auto att_prop =
                        wpn.prop_applied_on_melee(
                                att_data.attacker);

                if (att_prop.prop) {
                        try_apply_attack_property_on_actor(
                                att_prop,
                                defender,
                                wpn.data().ranged.dmg_type);
                }

                // NOTE: The 'can_bleed' flag is used as a condition here for
                // which monsters can be weakened by crippling strikes - it
                // should be a good enough rule so that crippling strikes can
                // only be applied against monsters where it makes sense
                if ((attacker == map::g_player) &&
                    player_bon::has_trait(Trait::crippling_strikes) &&
                    defender.m_data->can_bleed &&
                    // TODO: This prevents applying on Worm Masses, but it's
                    // hacky, and only makes sense *right now*, there should be
                    // some better attribute to control this
                    !defender.m_properties.has(PropId::splits_on_death) &&
                    rnd::percent(60)) {
                        auto weak = property_factory::make(PropId::weakened);

                        weak->set_duration(rnd::range(2, 3));

                        defender.m_properties.apply(weak);
                }

                if (wpn.data().melee.knocks_back) {
                        knockback::run(
                                defender,
                                attacker_origin,
                                false);
                }
        } else {
                // Defender was killed
                wpn.on_melee_kill(defender);
        }
}

static bool melee_should_break_wpn(
        const ActionResult att_result,
        const actor::Actor* attacker,
        const item::Item& wpn)
{
        const bool is_crit_fail =
                (att_result == ActionResult::fail_critical);

        const bool player_cursed =
                map::g_player->m_properties.has(PropId::cursed);

        const bool is_wielding_wpn =
                (map::g_player->m_inv.item_in_slot(SlotId::wpn) == &wpn);

        const int break_one_in_n = 32;

        return (attacker == map::g_player) &&
                is_wielding_wpn &&
                player_cursed &&
                is_crit_fail &&
                rnd::one_in(break_one_in_n);
}

// -----------------------------------------------------------------------------
// attack
// -----------------------------------------------------------------------------
namespace attack {

void melee(
        actor::Actor* const attacker,
        const P& attacker_origin,
        actor::Actor& defender,
        item::Wpn& wpn)
{
        if (attacker && (attacker != map::g_player)) {
                // A monster attacked, bump monster awareness
                static_cast<actor::Mon*>(attacker)
                        ->become_aware_player(
                                actor::AwareSource::other);
        }

        map::update_vision();

        const MeleeAttData att_data(attacker, defender, wpn);

        const auto att_result = ability_roll::roll(att_data.hit_chance_tot);

        const int dmg = att_data.dmg_range.total_range().roll();

        print_melee_msg(att_result, dmg, att_data);

        emit_melee_snd(att_result, dmg, att_data);

        if (att_result >= ActionResult::success) {
                melee_hit_actor(
                        dmg,
                        defender,
                        attacker,
                        attacker_origin,
                        wpn,
                        att_data);
        }

        // If player is cursed and the attack critically fails, occasionally
        // break the weapon
        if (melee_should_break_wpn(att_result, attacker, wpn)) {
                auto* const item =
                        map::g_player->m_inv
                                .remove_item_in_slot(SlotId::wpn, false);

                ASSERT(item);

                if (item) {
                        const std::string item_name =
                                item->name(
                                        ItemRefType::plain,
                                        ItemRefInf::none);

                        msg_log::add(
                                "My " + item_name + " breaks!",
                                colors::msg_note(),
                                MsgInterruptPlayer::yes,
                                MorePromptOnMsg::yes);

                        delete item;
                }
        }

        if (attacker) {
                if (defender.is_player()) {
                        // A monster attacked the player
                        auto* const mon =
                                static_cast<actor::Mon*>(att_data.attacker);

                        mon->set_player_aware_of_me();
                } else {
                        // A monster was attacked (by player or another monster)
                        static_cast<actor::Mon&>(defender)
                                .become_aware_player(
                                        actor::AwareSource::attacked);
                }

                // Attacking ends cloaking and sanctuary
                attacker->m_properties.end_prop(PropId::cloaked);
                attacker->m_properties.end_prop(PropId::sanctuary);

                game_time::tick();
        }
} // melee

DidAction ranged(
        actor::Actor* const attacker,
        const P& origin,
        const P& aim_pos,
        item::Wpn& wpn)
{
        map::update_vision();

        DidAction did_attack = DidAction::no;

        const bool has_inf_ammo = wpn.data().ranged.has_infinite_ammo;

        const int nr_projectiles = nr_projectiles_for_ranged_weapon(wpn);

        ProjectileFireData projectile_data;

        if ((wpn.m_ammo_loaded >= nr_projectiles) || has_inf_ammo) {
                wpn.pre_ranged_attack();

                projectile_data =
                        fire_projectiles(
                                attacker,
                                wpn,
                                origin,
                                aim_pos);

                did_attack = DidAction::yes;

                if (!has_inf_ammo) {
                        wpn.m_ammo_loaded -= nr_projectiles;
                }

                // Player could have for example fired an explosive weapon into
                // a wall and killed themselves - if so, abort early
                if (!map::g_player->is_alive()) {
                        return DidAction::yes;
                }
        }

        states::draw();

        if ((did_attack == DidAction::yes) && attacker) {
                // Attacking ends cloaking and sanctuary
                attacker->m_properties.end_prop(PropId::cloaked);
                attacker->m_properties.end_prop(PropId::sanctuary);

                if (attacker->is_player() ||
                    attacker->is_actor_my_leader(map::g_player)) {
                        // Attacker is player, or a monster allied to the
                        // player, alert all encountered monsters
                        for (auto* const actor : projectile_data.actors_seen) {
                                if (actor->is_player()) {
                                        continue;
                                }

                                static_cast<actor::Mon*>(actor)
                                        ->become_aware_player(
                                                actor::AwareSource::attacked);
                        }
                }

                if (!attacker->is_player()) {
                        // A monster attacked, bump its awareness
                        static_cast<actor::Mon*>(attacker)
                                ->become_aware_player(
                                        actor::AwareSource::other);
                }

                game_time::tick();
        }

        return did_attack;

} // ranged

} // namespace attack
