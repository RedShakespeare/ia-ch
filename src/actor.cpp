// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "actor.hpp"

#include "init.hpp"

#include "actor_death.hpp"
#include "actor_hit.hpp"
#include "actor_items.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "drop.hpp"
#include "explosion.hpp"
#include "fov.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "item.hpp"
#include "look.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "map_travel.hpp"
#include "marker.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "popup.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "terrain_door.hpp"
#include "terrain_mob.hpp"
#include "terrain_trap.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// actor
// -----------------------------------------------------------------------------
namespace actor {

int max_hp(const Actor& actor)
{
        int result = actor.m_base_max_hp;

        result = actor.m_properties.affect_max_hp(result);

        return std::max(1, result);
}

int max_sp(const Actor& actor)
{
        int result = actor.m_base_max_sp;

        result = actor.m_properties.affect_max_spi(result);

        return std::max(1, result);
}

void init_actor(Actor& actor, const P& pos_, ActorData& data)
{
        actor.m_pos = pos_;

        actor.m_data = &data;

        // NOTE: Cannot compare against the global player pointer here, since it
        // may not yet have been set up
        if (actor.m_data->id == actor::Id::player) {
                actor.m_base_max_hp = data.hp;
        } else {
                // Is monster
                const int hp_max_variation_pct = 25;
                const int hp_variation = (data.hp * hp_max_variation_pct) / 100;
                Range hp_range(data.hp - hp_variation, data.hp + hp_variation);
                hp_range.min = std::max(1, hp_range.min);
                actor.m_base_max_hp = hp_range.roll();
        }

        actor.m_hp = actor.m_base_max_hp;
        actor.m_sp = actor.m_base_max_sp = data.spi;
        actor.m_lair_pos = actor.m_pos;

        actor.m_properties.apply_natural_props_from_actor_data();

        if (!actor.is_player()) {
                actor_items::make_for_actor(actor);

                // Monster ghouls start allied to player ghouls
                const bool set_allied =
                        data.is_ghoul &&
                        (player_bon::bg() == Bg::ghoul) &&
                        // TODO: This is a hack to not allow the boss Ghoul to
                        // become allied
                        (data.id != actor::Id::high_priest_guard_ghoul);

                if (set_allied) {
                        Mon* const mon = static_cast<Mon*>(&actor);

                        mon->m_leader = map::g_player;
                }
        }
}

void print_aware_invis_mon_msg(const Mon& mon)
{
        std::string mon_ref;

        if (mon.m_data->is_ghost) {
                mon_ref = "some foul entity";
        } else if (mon.m_data->is_humanoid) {
                mon_ref = "someone";
        } else {
                mon_ref = "a creature";
        }

        msg_log::add(
                "There is " + mon_ref + " here!",
                colors::msg_note(),
                MsgInterruptPlayer::no,
                MorePromptOnMsg::yes);
}

// -----------------------------------------------------------------------------
// Actor
// -----------------------------------------------------------------------------
Actor::~Actor()
{
        // Free all items owning actors
        for (auto* item : m_inv.m_backpack) {
                item->clear_actor_carrying();
        }

        for (auto& slot : m_inv.m_slots) {
                if (slot.item) {
                        slot.item->clear_actor_carrying();
                }
        }
}

int Actor::ability(const AbilityId id, const bool is_affected_by_props) const
{
        return m_data->ability_values.val(id, is_affected_by_props, *this);
}

TileId Actor::tile() const
{
        if (is_corpse()) {
                return TileId::corpse2;
        }

        return m_data->tile;
}

char Actor::character() const
{
        if (is_corpse()) {
                return '&';
        }

        return m_data->character;
}

bool Actor::restore_hp(
        const int hp_restored,
        const bool is_allowed_above_max,
        const Verbose verbose)
{
        bool is_hp_gained = is_allowed_above_max;

        const int dif_from_max = actor::max_hp(*this) - hp_restored;

        // If HP is below limit, but restored HP will push it over the limit, HP
        // is set to max.
        if (!is_allowed_above_max &&
            (m_hp > dif_from_max) &&
            (m_hp < actor::max_hp(*this))) {
                m_hp = actor::max_hp(*this);

                is_hp_gained = true;
        }

        // If HP is below limit, and restored HP will NOT push it over the
        // limit, restored HP is added to current.
        if (is_allowed_above_max ||
            (m_hp <= dif_from_max)) {
                m_hp += hp_restored;

                is_hp_gained = true;
        }

        if ((verbose == Verbose::yes) && is_hp_gained) {
                if (is_player()) {
                        msg_log::add("I feel healthier!", colors::msg_good());
                } else if (map::g_player->can_see_actor(*this)) {
                        const std::string actor_name_the =
                                text_format::first_to_upper(
                                        m_data->name_the);

                        msg_log::add(actor_name_the + " looks healthier.");
                }
        }

        return is_hp_gained;
}

bool Actor::restore_sp(
        const int spi_restored,
        const bool is_allowed_above_max,
        const Verbose verbose)
{
        // Maximum allowed level to increase spirit to
        // * If we allow above max, we can raise spirit "infinitely"
        // * Otherwise we cap to max sp, or current sp, whichever is higher
        const int limit =
                is_allowed_above_max
                ? INT_MAX
                : std::max(m_sp, actor::max_sp(*this));

        const int sp_before = m_sp;

        m_sp = std::min(m_sp + spi_restored, limit);

        const bool is_spi_gained = m_sp > sp_before;

        if (verbose == Verbose::yes &&
            is_spi_gained) {
                if (is_player()) {
                        msg_log::add(
                                "I feel more spirited!",
                                colors::msg_good());
                } else {
                        if (map::g_player->can_see_actor(*this)) {
                                const std::string actor_name_the =
                                        text_format::first_to_upper(
                                                m_data->name_the);

                                msg_log::add(
                                        actor_name_the +
                                        " looks more spirited.");
                        }
                }
        }

        return is_spi_gained;
}

void Actor::change_max_hp(const int change, const Verbose verbose)
{
        m_base_max_hp = std::max(1, m_base_max_hp + change);

        if (verbose == Verbose::no) {
                return;
        }

        if (is_player()) {
                if (change > 0) {
                        msg_log::add(
                                "I feel more vigorous!",
                                colors::msg_good());
                } else if (change < 0) {
                        msg_log::add(
                                "I feel frailer!",
                                colors::msg_bad());
                }
        } else if (map::g_player->can_see_actor(*this)) {
                const std::string actor_name_the =
                        text_format::first_to_upper(
                                name_the());

                if (change > 0) {
                        msg_log::add(actor_name_the + " looks more vigorous.");
                } else if (change < 0) {
                        msg_log::add(actor_name_the + " looks frailer.");
                }
        }
}

void Actor::change_max_sp(const int change, const Verbose verbose)
{
        m_base_max_sp = std::max(1, m_base_max_sp + change);

        if (verbose == Verbose::no) {
                return;
        }

        if (is_player()) {
                if (change > 0) {
                        msg_log::add(
                                "My spirit is stronger!",
                                colors::msg_good());
                } else if (change < 0) {
                        msg_log::add(
                                "My spirit is weaker!",
                                colors::msg_bad());
                }
        } else if (map::g_player->can_see_actor(*this)) {
                const std::string actor_name_the =
                        text_format::first_to_upper(
                                name_the());

                if (change > 0) {
                        msg_log::add(
                                actor_name_the +
                                " appears to grow in spirit.");
                } else if (change < 0) {
                        msg_log::add(
                                actor_name_the +
                                " appears to shrink in spirit.");
                }
        }
}

int Actor::armor_points() const
{
        int ap = 0;

        // Worn armor
        if (m_data->is_humanoid) {
                auto* armor =
                        static_cast<item::Armor*>(
                                m_inv.item_in_slot(SlotId::body));

                if (armor) {
                        ap += armor->armor_points();
                }
        }

        // "Natural armor"
        if (is_player()) {
                if (player_bon::has_trait(Trait::thick_skinned)) {
                        ++ap;
                }
        }

        return ap;
}

std::string Actor::death_msg() const
{
        // Do not print a standard split message if this monster will split on
        // death (it will print a split message instead)
        if (m_properties.has(PropId::splits_on_death)) {
                return "";
        }

        const std::string actor_name_the =
                text_format::first_to_upper(
                        name_the());

        std::string msg_end;

        if (m_data->death_msg_override.empty()) {
                msg_end = "dies.";
        } else {
                msg_end = m_data->death_msg_override;
        }

        return actor_name_the + " " + msg_end;
}

DidAction Actor::try_eat_corpse()
{
        const bool actor_is_player = is_player();

        PropWound* wound = nullptr;

        if (actor_is_player) {
                Prop* prop = m_properties.prop(PropId::wound);

                if (prop) {
                        wound = static_cast<PropWound*>(prop);
                }
        }

        if ((m_hp >= actor::max_hp(*this)) && !wound) {
                // Not "hungry"
                return DidAction::no;
        }

        Actor* corpse = nullptr;

        // Check all corpses here, if this is the player eating, stop at any
        // corpse which is prioritized for bashing (Zombies)
        for (Actor* const actor : game_time::g_actors) {
                if ((actor->m_pos == m_pos) &&
                    (actor->m_state == ActorState::corpse)) {
                        corpse = actor;

                        if (actor_is_player && actor->m_data->prio_corpse_bash) {
                                break;
                        }
                }
        }

        if (corpse) {
                const int corpse_max_hp = corpse->m_base_max_hp;

                const int destr_one_in_n =
                        constr_in_range(1, corpse_max_hp / 4, 8);

                const bool is_destroyed = rnd::one_in(destr_one_in_n);

                const std::string corpse_name_the =
                        corpse->m_data->corpse_name_the;

                Snd snd("I hear ripping and chewing.",
                        SfxId::bite,
                        IgnoreMsgIfOriginSeen::yes,
                        m_pos,
                        this,
                        SndVol::low,
                        AlertsMon::no,
                        MorePromptOnMsg::no);

                snd.run();

                if (actor_is_player) {
                        msg_log::add("I feed on " + corpse_name_the + ".");
                } else {
                        // Is monster
                        if (map::g_player->can_see_actor(*this)) {
                                const std::string actor_name_the =
                                        text_format::first_to_upper(
                                                name_the());

                                msg_log::add(
                                        actor_name_the +
                                        " feeds on " +
                                        corpse_name_the +
                                        ".");
                        }
                }

                if (is_destroyed) {
                        corpse->m_state = ActorState::destroyed;

                        map::make_gore(m_pos);
                        map::make_blood(m_pos);
                }

                if (actor_is_player && is_destroyed) {
                        msg_log::add(
                                text_format::first_to_upper(corpse_name_the) +
                                " is completely devoured.");

                        std::vector<Actor*> corpses_here;

                        for (auto* const actor : game_time::g_actors) {
                                if ((actor->m_pos == m_pos) &&
                                    (actor->m_state == ActorState::corpse)) {
                                        corpses_here.push_back(actor);
                                }
                        }

                        if (!corpses_here.empty()) {
                                msg_log::more_prompt();

                                for (auto* const other_corpse : corpses_here) {
                                        const std::string name =
                                                text_format::first_to_upper(
                                                        other_corpse->m_data
                                                                ->corpse_name_a);

                                        msg_log::add(name + ".");
                                }
                        }
                }

                // Heal
                on_feed();

                return DidAction::yes;
        }

        return DidAction::no;
}

void Actor::on_feed()
{
        const int hp_restored = rnd::range(3, 5);

        restore_hp(hp_restored, false, Verbose::no);

        if (is_player()) {
                Prop* const prop = m_properties.prop(PropId::wound);

                if (prop && rnd::one_in(6)) {
                        auto* const wound = static_cast<PropWound*>(prop);

                        wound->heal_one_wound();
                }
        }
}

void Actor::add_light(Array2<bool>& light_map) const
{
        const bool is_alive_radiant =
                (m_state == ActorState::alive) &&
                m_properties.has(PropId::radiant);

        const bool is_burning = m_properties.has(PropId::burning);

        if (is_alive_radiant || is_burning) {
                for (const auto d : dir_utils::g_dir_list_w_center) {
                        light_map.at(m_pos + d) = true;
                }
        }

        add_light_hook(light_map);
}

bool Actor::is_player() const
{
        return this == map::g_player;
}

} // namespace actor
