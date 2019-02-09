// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "feature_door.hpp"

#include "actor.hpp"
#include "actor_player.hpp"
#include "debug.hpp"
#include "feature_data.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "postmortem.hpp"
#include "property_handler.hpp"
#include "text_format.hpp"

Door::Door(const P& feature_pos,
           const Wall* const mimic_feature,
           DoorType type,
           DoorSpawnState spawn_state) :
        Rigid(feature_pos),
        m_mimic_feature(mimic_feature),
        m_nr_spikes(0),
        m_is_open(false),
        m_is_stuck(false),
        m_is_secret(false),
        m_type(type)
{
        // Gates should never be secret
        ASSERT(!(m_type == DoorType::gate && m_mimic_feature));

        ASSERT(
                !(m_type == DoorType::gate &&
                  (spawn_state == DoorSpawnState::secret ||
                   spawn_state == DoorSpawnState::secret_and_stuck)));

        if (spawn_state == DoorSpawnState::any)
        {
                // NOTE: The chances below are just generic default behavior for
                // random doors placed wherever. Doors may be explicitly set to
                // other states elsewhere during map generation (e.g. set to
                // secret to hide an optional branch of the map).

                const int pct_secret =
                        (m_type == DoorType::gate)
                        ? 0
                        : (map::g_dlvl * 3);

                const int pct_stuck = 5;

                if (rnd::percent(pct_secret))
                {
                        if (rnd::percent(pct_stuck))
                        {
                                spawn_state = DoorSpawnState::secret_and_stuck;
                        }
                        else // Not stuck
                        {
                                spawn_state = DoorSpawnState::secret;
                        }
                }
                else // Not secret
                {
                        Fraction chance_open(3, 4);

                        if (chance_open.roll())
                        {
                                spawn_state = DoorSpawnState::open;
                        }
                        else // Closed
                        {
                                if (rnd::percent(pct_stuck))
                                {
                                        spawn_state = DoorSpawnState::stuck;
                                }
                                else // Not stuck
                                {
                                        spawn_state = DoorSpawnState::closed;
                                }
                        }
                }
        }

        switch (DoorSpawnState(spawn_state))
        {
        case DoorSpawnState::open:
                m_is_open = true;
                m_is_stuck = false;
                m_is_secret = false;
                break;

        case DoorSpawnState::closed:
                m_is_open = false;
                m_is_stuck = false;
                m_is_secret = false;
                break;

        case DoorSpawnState::stuck:
                m_is_open = false;
                m_is_stuck = true;
                m_is_secret = false;
                break;

        case DoorSpawnState::secret:
                m_is_open = false;
                m_is_stuck = false;
                m_is_secret = true;
                break;

        case DoorSpawnState::secret_and_stuck:
                m_is_open = false;
                m_is_stuck = true;
                m_is_secret = true;
                break;

        case DoorSpawnState::any:
                ASSERT(false);

                m_is_open = false;
                m_is_stuck = false;
                m_is_secret = false;
                break;
        }

} // Door

Door::~Door()
{
        // Unlink all levers
        if (m_type == DoorType::metal)
        {
                for (size_t i = 0; i < map::nr_cells(); ++i)
                {
                        auto* const rigid = map::g_cells.at(i).rigid;

                        if (rigid && (rigid->id() == FeatureId::lever))
                        {
                                auto* const lever = static_cast<Lever*>(rigid);

                                if (lever->is_linked_to(*this))
                                {
                                        lever->unlink();
                                }
                        }
                }
        }

        delete m_mimic_feature;
}

void Door::on_hit(const int dmg,
                  const DmgType dmg_type,
                  const DmgMethod dmg_method,
                  Actor* const actor)
{
        if (dmg_method == DmgMethod::forced)
        {
                // Forced
                map::put(new RubbleLow(m_pos));

                map::update_vision();

                return;
        }

        if (dmg_type == DmgType::physical)
        {
                // Shotgun
                if (dmg_method == DmgMethod::shotgun)
                {
                        if (!m_is_open)
                        {
                                switch (m_type)
                                {
                                case DoorType::wood:
                                case DoorType::gate:
                                {
                                        if (map::is_pos_seen_by_player(m_pos))
                                        {
                                                const std::string a =
                                                        m_is_secret
                                                        ? "A "
                                                        : "The ";

                                                msg_log::add(
                                                        a +
                                                        base_name_short() +
                                                        " is blown to pieces!");
                                        }

                                        map::put(new RubbleLow(m_pos));

                                        map::update_vision();

                                        return;
                                }
                                break;

                                case DoorType::metal:
                                        break;
                                }
                        }
                }

                // Explosion
                if (dmg_method == DmgMethod::explosion)
                {
                        //TODO
                }

                // Kicking, blunt (sledgehammers), or slashing (axes)
                if ((dmg_method == DmgMethod::kicking) ||
                    (dmg_method == DmgMethod::blunt) ||
                    (dmg_method == DmgMethod::slashing))
                {
                        ASSERT(actor);

                        const bool is_player = actor == map::g_player;

                        const bool is_cell_seen = map::is_pos_seen_by_player(m_pos);

                        const bool is_weak = actor->m_properties.has(PropId::weakened);

                        switch (m_type)
                        {
                        case DoorType::wood:
                        case DoorType::gate:
                        {
                                if (is_player)
                                {
                                        int destr_chance_pct = 25 + (dmg * 5) - (m_nr_spikes * 4);

                                        destr_chance_pct = std::max(1, destr_chance_pct);

                                        if (player_bon::has_trait(Trait::tough))
                                        {
                                                destr_chance_pct += 15;
                                        }

                                        if (player_bon::has_trait(Trait::rugged))
                                        {
                                                destr_chance_pct += 15;
                                        }

                                        if (actor->m_properties.has(PropId::frenzied))
                                        {
                                                destr_chance_pct += 30;
                                        }

                                        if (is_weak)
                                        {
                                                destr_chance_pct = 0;
                                        }

                                        destr_chance_pct = std::min(100, destr_chance_pct);

                                        if (destr_chance_pct > 0)
                                        {
                                                if (rnd::percent(destr_chance_pct))
                                                {
                                                        Snd snd("",
                                                                SfxId::door_break,
                                                                IgnoreMsgIfOriginSeen::yes,
                                                                m_pos,
                                                                actor,
                                                                SndVol::low,
                                                                AlertsMon::yes);

                                                        snd.run();

                                                        if (is_cell_seen)
                                                        {
                                                                if (m_is_secret)
                                                                {
                                                                        msg_log::add(
                                                                                "A " +
                                                                                base_name_short() +
                                                                                " crashes open!");
                                                                }
                                                                else
                                                                {
                                                                        msg_log::add(
                                                                                "The " +
                                                                                base_name_short() +
                                                                                " crashes open!");
                                                                }
                                                        }
                                                        else // Cell not seen
                                                        {
                                                                msg_log::add("I feel a door crashing open!");
                                                        }

                                                        map::put(new RubbleLow(m_pos));

                                                        map::update_vision();
                                                }
                                                else // Not destroyed
                                                {
                                                        const SfxId sfx =
                                                                m_is_secret ?
                                                                SfxId::END :
                                                                SfxId::door_bang;

                                                        Snd snd("",
                                                                sfx,
                                                                IgnoreMsgIfOriginSeen::no,
                                                                m_pos,
                                                                actor,
                                                                SndVol::low,
                                                                AlertsMon::yes);

                                                        snd.run();
                                                }
                                        }
                                        else // No chance of success
                                        {
                                                if (is_cell_seen && !m_is_secret)
                                                {
                                                        Snd snd("",
                                                                SfxId::door_bang,
                                                                IgnoreMsgIfOriginSeen::no,
                                                                actor->m_pos,
                                                                actor,
                                                                SndVol::low,
                                                                AlertsMon::yes);

                                                        snd.run();

                                                        msg_log::add("It seems futile.");
                                                }
                                        }
                                }
                                else // Is monster
                                {
                                        int destr_chance_pct = 7 - (m_nr_spikes * 2);

                                        destr_chance_pct = std::max(1, destr_chance_pct);

                                        if (is_weak)
                                        {
                                                destr_chance_pct = 0;
                                        }

                                        if (rnd::percent(destr_chance_pct))
                                        {
                                                // NOTE: When it's a monster bashing down the door, we
                                                // make the sound alert other monsters - since causes
                                                // nicer AI behavior (everyone near the door understands
                                                // that it's time to run inside)
                                                Snd snd("I hear a door crashing open!",
                                                        SfxId::door_break,
                                                        IgnoreMsgIfOriginSeen::yes,
                                                        m_pos,
                                                        actor,
                                                        SndVol::high,
                                                        AlertsMon::yes);

                                                snd.run();

                                                if (map::g_player->can_see_actor(*actor))
                                                {
                                                        msg_log::add(
                                                                "The " +
                                                                base_name_short() +
                                                                " crashes open!");
                                                }
                                                else if (is_cell_seen)
                                                {
                                                        msg_log::add(
                                                                "A " +
                                                                base_name_short() +
                                                                " crashes open!");
                                                }

                                                map::put(new RubbleLow(m_pos));

                                                map::update_vision();
                                        }
                                        else // Not destroyed
                                        {
                                                Snd snd("I hear a loud banging.",
                                                        SfxId::door_bang,
                                                        IgnoreMsgIfOriginSeen::yes,
                                                        actor->m_pos,
                                                        actor,
                                                        SndVol::high,
                                                        AlertsMon::no);

                                                snd.run();
                                        }
                                }

                        }
                        break; // wood, gate

                        case DoorType::metal:
                        {
                                if (is_player &&
                                    is_cell_seen &&
                                    !m_is_secret)
                                {
                                        msg_log::add(
                                                "It seems futile.",
                                                colors::msg_note(),
                                                false,
                                                MorePromptOnMsg::yes);
                                }
                        }
                        break; // metal

                        } // Door type switch

                } // Blunt or slashing damage method

        } // Physical damage

        // Fire
        if (dmg_method == DmgMethod::elemental &&
            dmg_type == DmgType::fire &&
            matl() == Matl::wood)
        {
                try_start_burning(true);
        }

} // on_hit

WasDestroyed Door::on_finished_burning()
{
        if (map::is_pos_seen_by_player(m_pos))
        {
                msg_log::add("The door burns down.");
        }

        RubbleLow* const rubble = new RubbleLow(m_pos);

        rubble->m_burn_state = BurnState::has_burned;

        map::put(rubble);

        map::update_vision();

        return WasDestroyed::yes;
}

bool Door::is_walkable() const
{
        return m_is_open;
}

bool Door::can_move(const Actor& actor) const
{
        return
                m_is_open ||
                actor.m_properties.has(PropId::ethereal) ||
                actor.m_properties.has(PropId::ooze);
}

bool Door::is_los_passable() const
{
        return m_is_open || (m_type == DoorType::gate);
}

bool Door::is_projectile_passable() const
{
        return m_is_open || (m_type == DoorType::gate);
}

bool Door::is_smoke_passable() const
{
        return m_is_open || (m_type == DoorType::gate);
}

std::string Door::base_name() const
{
        std::string ret = "";

        switch (m_type)
        {
        case DoorType::wood:
                ret = "wooden door";
                break;

        case DoorType::metal:
                ret = "metal door";
                break;

        case DoorType::gate:
                ret = "barred gate";
                break;
        }

        return ret;
}

std::string Door::base_name_short() const
{
        std::string ret = "";

        switch (m_type)
        {
        case DoorType::wood:
                ret = "door";
                break;

        case DoorType::metal:
                ret = "door";
                break;

        case DoorType::gate:
                ret = "barred gate";
                break;
        }

        return ret;
}

std::string Door::name(const Article article) const
{
        if (m_is_secret)
        {
                ASSERT(m_type != DoorType::gate);
                ASSERT(m_mimic_feature);

                return m_mimic_feature->name(article);
        }

        std::string a = "";

        std::string mod = "";

        if (m_burn_state == BurnState::burning)
        {
                a =
                        (article == Article::a)
                        ? "a "
                        : "the ";

                mod = "burning ";
        }

        if (m_is_open)
        {
                if (a.empty())
                {
                        a =
                                (article == Article::a)
                                ? "an "
                                : "the ";
                }

                mod += "open " ;
        }

        if (a.empty())
        {
                a =
                        (article == Article::a)
                        ? "a "
                        : "the ";
        }

        return a + mod + base_name();

} // name

Color Door::color_default() const
{
        if (m_is_secret)
        {
                return m_mimic_feature->color();
        }
        else
        {
                switch (m_type)
                {
                case DoorType::wood:
                        return colors::dark_brown();
                        break;

                case DoorType::metal:
                        return colors::cyan();
                        break;

                case DoorType::gate:
                        return colors::gray();
                        break;
                }
        }

        ASSERT(false);

        return colors::gray();
}

char Door::character() const
{
        if (m_is_secret)
        {
                ASSERT(m_type != DoorType::gate);
                ASSERT(m_mimic_feature);

                return m_mimic_feature->character();
        }
        else // Not secret
        {
                return m_is_open ? 39 : '+';
        }
}

TileId Door::tile() const
{
        TileId ret = TileId::END;

        if (m_is_secret)
        {
                ASSERT(m_type != DoorType::gate);
                ASSERT(m_mimic_feature);

                ret = m_mimic_feature->tile();
        }
        else // Not secret
        {
                switch (m_type)
                {
                case DoorType::wood:
                case DoorType::metal:
                {
                        ret =
                                m_is_open
                                ? TileId::door_open
                                : TileId::door_closed;
                }
                break;

                case DoorType::gate:
                {
                        ret =
                                m_is_open
                                ? TileId::gate_open
                                : TileId::gate_closed;
                }
                break;
                }
        }

        return ret;
}

Matl Door::matl() const
{
        switch (m_type)
        {
        case DoorType::wood:
                return Matl::wood;
                break;

        case DoorType::metal:
        case DoorType::gate:
                return Matl::metal;
                break;
        }

        ASSERT(false);

        return Matl::wood;
}

void Door::bump(Actor& actor_bumping)
{
        if (!actor_bumping.is_player())
        {
                return;
        }

        if (m_is_secret)
        {
                ASSERT(m_type != DoorType::gate);

                // Print messages as if this was a wall

                if (map::g_cells.at(m_pos).is_seen_by_player)
                {
                        TRACE << "Player bumped into secret door, "
                              << "with vision in cell" << std::endl;

                        msg_log::add(
                                feature_data::data(FeatureId::wall).
                                msg_on_player_blocked);
                }
                else // Not seen by player
                {
                        TRACE << "Player bumped into secret door, "
                              << "without vision in cell" << std::endl;

                        msg_log::add(
                                feature_data::data(FeatureId::wall).
                                msg_on_player_blocked_blind);
                }

                return;
        }

        if (!m_is_open)
        {
                try_open(&actor_bumping);
        }

} // bump

void Door::reveal(const Verbosity verbosity)
{
        if (!m_is_secret)
        {
                return;
        }

        m_is_secret = false;

        if (verbosity == Verbosity::verbose &&
            map::g_cells.at(m_pos).is_seen_by_player)
        {
                msg_log::add("A secret is revealed.");
        }
}

void Door::set_secret()
{
        ASSERT(m_type != DoorType::gate);

        m_is_open = false;
        m_is_secret = true;
}

bool Door::try_jam(Actor* actor_trying)
{
        const bool is_player = actor_trying == map::g_player;

        const bool tryer_is_blind = !actor_trying->m_properties.allow_see();

        if (m_is_secret || m_is_open)
        {
                return false;
        }

        // Door is in correct state for spiking (known, closed)
        ++m_nr_spikes;
        m_is_stuck = true;

        if (is_player)
        {
                std::string a =
                        tryer_is_blind
                        ?  "a "
                        : "the ";

                msg_log::add(
                        "I jam " +
                        a +
                        base_name_short() +
                        " with a spike.");
        }

        game_time::tick();
        return true;
}

void Door::try_close(Actor* actor_trying)
{
        const bool is_player = actor_trying == map::g_player;

        const bool tryer_is_blind = !actor_trying->m_properties.allow_see();

        if (is_player &&
            m_type == DoorType::metal)
        {
                if (tryer_is_blind)
                {
                        msg_log::add("There is a metal door here, but it's stuck.");
                }
                else
                {
                        msg_log::add("The door is stuck.");
                }

                msg_log::add("Perhaps it is handled elsewhere.");

                return;
        }

        bool is_closable = true;

        const bool player_see_tryer =
                is_player
                ? true
                : map::g_player->can_see_actor(*actor_trying);

        // Already closed?
        if (is_closable && !m_is_open)
        {
                is_closable = false;

                if (is_player)
                {
                        if (tryer_is_blind)
                        {
                                msg_log::add("I find nothing there to close.");
                        }
                        else // Can see
                        {
                                msg_log::add("I see nothing there to close.");
                        }
                }
        }

        // Blocked?
        if (is_closable)
        {
                bool is_blocked_by_actor = false;

                for (Actor* actor : game_time::g_actors)
                {
                        if ((actor->m_state != ActorState::destroyed) &&
                            (actor->m_pos == m_pos))
                        {
                                is_blocked_by_actor = true;

                                break;
                        }
                }

                if (is_blocked_by_actor ||
                    map::g_cells.at(m_pos).item)
                {
                        is_closable = false;

                        if (is_player)
                        {
                                if (tryer_is_blind)
                                {
                                        msg_log::add(
                                                "Something is blocking the " +
                                                base_name_short() +
                                                ".");
                                }
                                else // Can see
                                {
                                        msg_log::add(
                                                "The " +
                                                base_name_short() +
                                                " is blocked.");
                                }
                        }
                }
        }

        if (is_closable)
        {
                // Door is in correct state for closing (open, working, not blocked)

                if (tryer_is_blind)
                {
                        if (rnd::coin_toss())
                        {
                                m_is_open = false;

                                if (is_player)
                                {
                                        Snd snd("",
                                                SfxId::door_close,
                                                IgnoreMsgIfOriginSeen::yes,
                                                m_pos,
                                                actor_trying,
                                                SndVol::low,
                                                AlertsMon::yes);

                                        snd.run();

                                        msg_log::add("I fumble with a " +
                                                     base_name_short() +
                                                     ", but manage to close it.");
                                }
                                else // Monster closing
                                {
                                        Snd snd("I hear a door closing.",
                                                SfxId::door_close,
                                                IgnoreMsgIfOriginSeen::yes,
                                                m_pos,
                                                actor_trying,
                                                SndVol::low,
                                                AlertsMon::no);

                                        snd.run();

                                        if (player_see_tryer)
                                        {
                                                const std::string actor_name_the =
                                                        text_format::first_to_upper(
                                                                actor_trying->name_the());

                                                msg_log::add(
                                                        actor_name_the +
                                                        "fumbles, but manages to close a " +
                                                        base_name_short() +
                                                        ".");
                                        }
                                }
                        }
                        else // Fail to close
                        {
                                if (is_player)
                                {
                                        msg_log::add(
                                                "I fumble blindly with a " +
                                                base_name_short() +
                                                ", and fail to close it.");
                                }
                                else // Monster failing to close
                                {
                                        if (player_see_tryer)
                                        {
                                                const std::string actor_name_the =
                                                        text_format::first_to_upper(
                                                                actor_trying->name_the());

                                                msg_log::add(
                                                        actor_name_the +
                                                        " fumbles blindly, and fails to close a " +
                                                        base_name_short() +
                                                        ".");
                                        }
                                }
                        }
                }
                else // Can see
                {
                        m_is_open = false;

                        if (is_player)
                        {
                                const auto alerts_mon =
                                        player_bon::has_trait(Trait::silent)
                                        ? AlertsMon::no
                                        : AlertsMon::yes;

                                Snd snd("",
                                        SfxId::door_close,
                                        IgnoreMsgIfOriginSeen::yes,
                                        m_pos,
                                        actor_trying,
                                        SndVol::low,
                                        alerts_mon);

                                snd.run();

                                msg_log::add(
                                        "I close the " +
                                        base_name_short() +
                                        ".");
                        }
                        else // Is a monster closing
                        {
                                Snd snd("I hear a door closing.",
                                        SfxId::door_close,
                                        IgnoreMsgIfOriginSeen::yes,
                                        m_pos,
                                        actor_trying,
                                        SndVol::low,
                                        AlertsMon::no);

                                snd.run();

                                if (player_see_tryer)
                                {
                                        const std::string actor_name_the =
                                                text_format::first_to_upper(
                                                        actor_trying->name_the());

                                        msg_log::add(
                                                actor_name_the +
                                                " closes a " +
                                                base_name_short() +
                                                ".");
                                }
                        }
                }
        }

        // TODO: It doesn't seem like a turn is spent if player is blind and fails
        // to close the door?
        if (!m_is_open && is_closable)
        {
                game_time::tick();
        }

        if (!m_is_open)
        {
                map::update_vision();
        }

} // try_close

void Door::try_open(Actor* actor_trying)
{
        TRACE_FUNC_BEGIN;

        const bool is_player = actor_trying == map::g_player;

        const bool player_see_door =
                map::g_cells.at(m_pos)
                .is_seen_by_player;

        const bool player_see_tryer =
                is_player
                ? true
                : map::g_player->can_see_actor(*actor_trying);

        if (is_player &&
            m_type == DoorType::metal)
        {
                if (!player_see_door)
                {
                        msg_log::add("There is a closed metal door here.");
                }

                msg_log::add("I find no way to open it.");

                msg_log::add("Perhaps it is handled elsewhere.");

                return;
        }

        if (m_is_stuck)
        {
                TRACE << "Is stuck" << std::endl;

                if (is_player)
                {
                        msg_log::add(
                                "The " +
                                base_name_short() +
                                " seems to be stuck.");
                }
        }
        else // Not stuck
        {
                TRACE << "Is not stuck" << std::endl;

                const bool tryer_can_see = actor_trying->m_properties.allow_see();

                if (tryer_can_see)
                {
                        TRACE << "Tryer can see, opening" << std::endl;
                        m_is_open = true;

                        if (is_player)
                        {
                                const auto alerts_mon =
                                        player_bon::has_trait(Trait::silent)
                                        ? AlertsMon::no
                                        : AlertsMon::yes;

                                Snd snd("",
                                        SfxId::door_open,
                                        IgnoreMsgIfOriginSeen::yes,
                                        m_pos,
                                        actor_trying,
                                        SndVol::low,
                                        alerts_mon);

                                snd.run();

                                msg_log::add(
                                        "I open the " +
                                        base_name_short() +
                                        ".");
                        }
                        else // Is monster
                        {
                                Snd snd("I hear a door open.",
                                        SfxId::door_open,
                                        IgnoreMsgIfOriginSeen::yes,
                                        m_pos,
                                        actor_trying,
                                        SndVol::low,
                                        AlertsMon::no);

                                snd.run();

                                if (player_see_tryer)
                                {
                                        const std::string actor_name_the =
                                                text_format::first_to_upper(
                                                        actor_trying->name_the());

                                        msg_log::add(
                                                actor_name_the +
                                                " opens a " +
                                                base_name_short() +
                                                ".");
                                }
                                else if (player_see_door)
                                {
                                        msg_log::add(
                                                "I see a " +
                                                base_name_short() +
                                                " opening.");
                                }
                        }
                }
                else // Tryer is blind
                {
                        if (rnd::coin_toss())
                        {
                                TRACE << "Tryer is blind, but open succeeded anyway"
                                      << std::endl;

                                m_is_open = true;

                                if (is_player)
                                {
                                        Snd snd("",
                                                SfxId::door_open,
                                                IgnoreMsgIfOriginSeen::yes,
                                                m_pos,
                                                actor_trying,
                                                SndVol::low,
                                                AlertsMon::yes);

                                        snd.run();

                                        msg_log::add(
                                                "I fumble with a " +
                                                base_name_short() +
                                                ", but finally manage to open it.");
                                }
                                else // Is monster
                                {
                                        Snd snd("I hear something open a door awkwardly.",
                                                SfxId::door_open,
                                                IgnoreMsgIfOriginSeen::yes,
                                                m_pos,
                                                actor_trying,
                                                SndVol::low,
                                                AlertsMon::no);

                                        snd.run();

                                        if (player_see_tryer)
                                        {
                                                const std::string actor_name_the =
                                                        text_format::first_to_upper(
                                                                actor_trying->name_the());

                                                msg_log::add(
                                                        actor_name_the +
                                                        "fumbles, but manages to open a " +
                                                        base_name_short() +
                                                        ".");
                                        }
                                        else if (player_see_door)
                                        {
                                                msg_log::add(
                                                        "I see a " +
                                                        base_name_short() +
                                                        " open awkwardly.");
                                        }
                                }
                        }
                        else // Failed to open
                        {
                                TRACE << "Tryer is blind, and open failed" << std::endl;

                                if (is_player)
                                {
                                        Snd snd("",
                                                SfxId::END,
                                                IgnoreMsgIfOriginSeen::yes,
                                                m_pos,
                                                actor_trying,
                                                SndVol::low,
                                                AlertsMon::yes);

                                        snd.run();

                                        msg_log::add(
                                                "I fumble blindly with a " +
                                                base_name_short() +
                                                ", and fail to open it.");
                                }
                                else // Is monster
                                {
                                        // Emitting the sound from the actor instead of the door,
                                        // because the sound message should be received even if the
                                        // door is seen
                                        Snd snd("I hear something attempting to open a door.",
                                                SfxId::END,
                                                IgnoreMsgIfOriginSeen::yes,
                                                actor_trying->m_pos,
                                                actor_trying,
                                                SndVol::low,
                                                AlertsMon::no);

                                        snd.run();

                                        if (player_see_tryer)
                                        {
                                                const std::string actor_name_the =
                                                        text_format::first_to_upper(
                                                                actor_trying->name_the());

                                                msg_log::add(
                                                        actor_name_the +
                                                        " fumbles blindly, and fails to open a " +
                                                        base_name_short() +
                                                        ".");
                                        }
                                }

                                game_time::tick();
                        }
                }
        }

        if (m_is_open)
        {
                TRACE << "Open was successful" << std::endl;

                if (m_is_secret)
                {
                        TRACE << "Was secret, now revealing" << std::endl;
                        reveal(Verbosity::verbose);
                }

                game_time::tick();

                map::update_vision();
        }

} // try_open

void Door::on_lever_pulled(Lever* const lever)
{
        (void)lever;

        if (m_is_open)
        {
                close(nullptr);
        }
        else // Closed
        {
                open(nullptr);
        }
}

DidOpen Door::open(Actor* const actor_opening)
{
        (void)actor_opening;

        m_is_open = true;

        m_is_secret= false;

        m_is_stuck = false;

        // TODO: This is kind of a hack...
        if (m_type == DoorType::metal)
        {
                Snd snd("",
                        SfxId::END,
                        IgnoreMsgIfOriginSeen::yes,
                        m_pos,
                        nullptr,
                        SndVol::low,
                        AlertsMon::yes);

                snd.run();
        }

        return DidOpen::yes;
}

DidClose Door::close(Actor* const actor_closing)
{
        (void)actor_closing;

        m_is_open = false;

        // TODO: This is kind of a hack...
        if (m_type == DoorType::metal)
        {
                Snd snd("",
                        SfxId::END,
                        IgnoreMsgIfOriginSeen::yes,
                        m_pos,
                        nullptr,
                        SndVol::low,
                        AlertsMon::yes);

                snd.run();
        }

        return DidClose::yes;
}
