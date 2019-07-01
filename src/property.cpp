// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "property.hpp"

#include <algorithm>

#include "actor.hpp"
#include "actor_death.hpp"
#include "actor_factory.hpp"
#include "actor_hit.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "explosion.hpp"
#include "terrain.hpp"
#include "game_time.hpp"
#include "io.hpp"
#include "item_factory.hpp"
#include "knockback.hpp"
#include "line_calc.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "misc.hpp"
#include "msg_log.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "saving.hpp"
#include "teleport.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Property base class
// -----------------------------------------------------------------------------
Prop::Prop(PropId id) :
        m_id(id),
        m_data(property_data::g_data[(size_t)id]),
        m_nr_turns_left(m_data.std_rnd_turns.roll()),
        m_duration_mode(PropDurationMode::standard),
        m_owner(nullptr),
        m_src(PropSrc::END),
        m_item_applying(nullptr)
{

}

// -----------------------------------------------------------------------------
// Specific properties
// -----------------------------------------------------------------------------
void PropBlessed::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::cursed,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );

        bless_adjacent();
}

void PropBlessed::on_more(const Prop& new_prop)
{
        (void)new_prop;

        bless_adjacent();
}

void PropBlessed::bless_adjacent() const
{
        // "Bless" adjacent fountains
        const P& p = m_owner->m_pos;

        for (const P& d : dir_utils::g_dir_list_w_center)
        {
                const P p_adj(p + d);

                Cell& cell = map::g_cells.at(p_adj);

                auto* const terrain = cell.terrain;

                if (terrain->id() != terrain::Id::fountain)
                {
                        continue;
                }

                static_cast<terrain::Fountain*>(terrain)->bless();
        }
}

void PropCursed::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::blessed,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );

        curse_adjacent();
}

void PropCursed::on_more(const Prop& new_prop)
{
        (void)new_prop;

        curse_adjacent();
}

void PropCursed::curse_adjacent() const
{
        // "Curse" adjacent fountains
        const P& p = m_owner->m_pos;

        for (const P& d : dir_utils::g_dir_list_w_center)
        {
                const P p_adj(p + d);

                Cell& cell = map::g_cells.at(p_adj);

                auto* const terrain = cell.terrain;

                if (terrain->id() != terrain::Id::fountain)
                {
                        continue;
                }

                static_cast<terrain::Fountain*>(terrain)->curse();
        }
}

PropEnded PropEntangled::on_tick()
{
        if (!m_owner->m_properties.has(PropId::swimming))
        {
                return PropEnded::no;
        }

        if (m_owner->is_player())
        {
                msg_log::add("I am drowning!", colors::msg_bad());
        }
        else if (map::g_player->can_see_actor(*m_owner))
        {
                const auto name_the =
                        text_format::first_to_upper(
                                m_owner->name_the());

                msg_log::add(name_the + " is drowning.", colors::msg_good());
        }

        actor::hit(*m_owner, 1, DmgType::physical);

        return PropEnded::no;
}

void PropEntangled::on_applied()
{
        // TODO: Rather than doing this on the "on_applied" hook (which should
        // probably be reserved for when the property actually does get applied,
        // i.e. it shall not be removed), consider checking this elsewhere,
        // perhaps in a new function such as "is_resisting_self"
        try_player_end_with_machete();
}

PropEnded PropEntangled::affect_move_dir(const P& actor_pos, Dir& dir)
{
        (void)actor_pos;

        if (dir == Dir::center)
        {
                return PropEnded::no;
        }

        if (try_player_end_with_machete())
        {
                return PropEnded::yes;
        }

        dir = Dir::center;

        if (m_owner->is_player())
        {
                msg_log::add("I struggle to tear free!", colors::msg_bad());
        }
        else // Is monster
        {
                if (map::g_player->can_see_actor(*m_owner))
                {
                        const std::string actor_name_the =
                                text_format::first_to_upper(
                                        m_owner->name_the());

                        msg_log::add(actor_name_the +
                                     " struggles to tear free.",
                                     colors::msg_good());
                }
        }

        if (rnd::one_in(8))
        {
                m_owner->m_properties.end_prop(id());

                return PropEnded::yes;
        }

        return PropEnded::no;
}

bool PropEntangled::try_player_end_with_machete()
{
        if (m_owner != map::g_player)
        {
                return false;
        }

        auto* item = m_owner->m_inv.item_in_slot(SlotId::wpn);

        if (item && (item->id() == item::Id::machete))
        {
                msg_log::add("I cut myself free with my Machete.");

                m_owner->m_properties.end_prop(
                        id(),
                        PropEndConfig(
                                PropEndAllowCallEndHook::no,
                                PropEndAllowMsg::no,
                                PropEndAllowHistoricMsg::yes)
                        );

                return true;
        }

        return false;
}

void PropSlowed::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::hasted,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

void PropHasted::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::slowed,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

void PropClockworkHasted::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::slowed,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

void PropSummoned::on_end()
{
        m_owner->m_state = ActorState::destroyed;

        actor::unset_actor_as_leader_for_all_mon(*m_owner);
}

PropEnded PropInfected::on_tick()
{
#ifndef NDEBUG
        ASSERT(!m_owner->m_properties.has(PropId::diseased));
#endif // NDEBUG

        if (map::g_player->m_active_medical_bag)
        {
                ++m_nr_turns_left;

                return PropEnded::no;
        }

        const int allow_disease_below_turns_left = 50;

        const int apply_disease_one_in_n = m_nr_turns_left - 1;

        const bool apply_disease =
                (m_nr_turns_left <= allow_disease_below_turns_left) &&
                ((apply_disease_one_in_n <= 0) ||
                 rnd::one_in(apply_disease_one_in_n));

        if (apply_disease)
        {
                auto* const owner = m_owner;

                owner->m_properties.end_prop(
                        id(),
                        PropEndConfig(
                                PropEndAllowCallEndHook::no,
                                PropEndAllowMsg::no,
                                PropEndAllowHistoricMsg::yes)
                        );

                // NOTE: This property is now deleted

                auto prop_diseased = new PropDiseased();

                prop_diseased->set_indefinite();

                owner->m_properties.apply(prop_diseased);

                msg_log::more_prompt();

                return PropEnded::yes;
        }

        return PropEnded::no;
}

int PropDiseased::affect_max_hp(const int hp_max) const
{
        return hp_max / 2;
}

void PropDiseased::on_applied()
{
        // End infection
        m_owner->m_properties.end_prop(
                PropId::infected,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

bool PropDiseased::is_resisting_other_prop(const PropId prop_id) const
{
#ifndef NDEBUG
        ASSERT(!m_owner->m_properties.has(PropId::infected));
#endif // NDEBUG

        // Getting infected while already diseased is just annoying
        return prop_id == PropId::infected;
}

PropEnded PropDescend::on_tick()
{
        ASSERT(m_owner->is_player());

        if (m_nr_turns_left <= 1)
        {
                game_time::g_is_magic_descend_nxt_std_turn = true;
        }

        return PropEnded::no;
}

void PropZuulPossessPriest::on_placed()
{
        // If the number left allowed to spawn is zero now, this means that Zuul
        // has been spawned for the first time - hide Zuul inside a priest.
        // Otherwise we do nothing, and just spawn Zuul. When the possessed
        // actor is killed, Zuul will be allowed to spawn infinitely (this is
        // handled elsewhere).

        const int nr_left_allowed = m_owner->m_data->nr_left_allowed_to_spawn;

        ASSERT(nr_left_allowed <= 0);

        const bool should_possess = (nr_left_allowed >= 0);

        if (should_possess)
        {
                m_owner->m_state = ActorState::destroyed;

                auto* actor =
                        actor::make(
                                actor::Id::cultist_priest,
                                m_owner->m_pos);

                auto* prop = new PropPossessedByZuul();

                prop->set_indefinite();

                actor->m_properties.apply(
                        prop,
                        PropSrc::intr,
                        true,
                        Verbosity::silent);

                actor->restore_hp(999, false, Verbosity::silent);
        }
}

void PropPossessedByZuul::on_death()
{
        // An actor possessed by Zuul has died - release Zuul!

        if (map::g_player->can_see_actor(*m_owner))
        {
                const std::string& name1 =
                        text_format::first_to_upper(
                                m_owner->name_the());

                const std::string& name2 =
                        actor::g_data[(size_t)actor::Id::zuul].name_the;

                msg_log::add(name1 + " was possessed by " + name2 + "!");
        }

        m_owner->m_state = ActorState::destroyed;

        const P& pos = m_owner->m_pos;

        map::make_gore(pos);

        map::make_blood(pos);

        // Zuul is now free, allow it to spawn infinitely
        actor::g_data[(size_t)actor::Id::zuul].nr_left_allowed_to_spawn = -1;

        actor::spawn(
                pos,
                {actor::Id::zuul},
                map::rect())
                .make_aware_of_player();
}

PropEnded PropPoisoned::on_tick()
{
        if (m_owner->is_alive() &&
            (game_time::turn_nr() % g_poison_dmg_n_turn) == 0)
        {
                int dmg = 1;

                if (m_owner->is_player())
                {
                        msg_log::add(
                                "I am suffering from the poison!",
                                colors::msg_bad(),
                                true);
                }
                else // Is monster
                {
                        dmg *= 2;

                        if (map::g_player->can_see_actor(*m_owner))
                        {
                                const std::string actor_name_the =
                                        text_format::first_to_upper(
                                                m_owner->name_the());

                                msg_log::add(
                                        actor_name_the +
                                        " suffers from poisoning!");
                        }
                }

                actor::hit(*m_owner, dmg, DmgType::pure);
        }

        return PropEnded::no;
}

void PropAiming::on_hit()
{
        m_owner->m_properties.end_prop(id());
}

bool PropTerrified::allow_attack_melee(const Verbosity verbosity) const
{
        if (m_owner->is_player() && verbosity == Verbosity::verbose)
        {
                msg_log::add("I am too terrified to engage in close combat!");
        }

        return false;
}

bool PropTerrified::allow_attack_ranged(const Verbosity verbosity) const
{
        (void)verbosity;
        return true;
}

void PropTerrified::on_applied()
{
        // If this is a monster, we reset its last direction moved. Otherwise it
        // would probably tend to move toward the player even while terrified
        // (the AI would typically use the idle movement algorithm, which
        // favors stepping in the same direction as the last move).

        if (!m_owner->is_player())
        {
                auto* const mon = static_cast<actor::Mon*>(m_owner);

                mon->m_last_dir_moved = Dir::center;
        }
}

PropEnded PropNailed::affect_move_dir(const P& actor_pos, Dir& dir)
{
        (void)actor_pos;

        if (dir == Dir::center)
        {
                return PropEnded::no;
        }

        dir = Dir::center;

        if (m_owner->is_player())
        {
                msg_log::add("I struggle to tear out the spike!",
                             colors::msg_bad());
        }
        else // Is monster
        {
                if (map::g_player->can_see_actor(*m_owner))
                {
                        const std::string actor_name_the =
                                text_format::first_to_upper(
                                        m_owner->name_the());

                        msg_log::add(actor_name_the + " struggles in pain!",
                                     colors::msg_good());
                }
        }

        actor::hit(*m_owner, rnd::range(1, 3), DmgType::physical);

        if (!m_owner->is_alive() ||
            !rnd::one_in(4))
        {
                return PropEnded::no;
        }

        --m_nr_spikes;

        if (m_nr_spikes > 0)
        {
                if (m_owner->is_player())
                {
                        msg_log::add("I rip out a spike from my flesh!");
                }
                else if (map::g_player->can_see_actor(*m_owner))
                {
                        const std::string actor_name_the =
                                text_format::first_to_upper(
                                        m_owner->name_the());

                        msg_log::add(actor_name_the + " tears out a spike!");
                }
        }

        return PropEnded::no;
}

void PropWound::save() const
{
        saving::put_int(m_nr_wounds);
}

void PropWound::load()
{
        m_nr_wounds = saving::get_int();
}

int PropWound::ability_mod(const AbilityId ability) const
{
        // A player with Survivalist receives no ability penalties
        if (m_owner->is_player() &&
            player_bon::has_trait(Trait::survivalist))
        {
                return 0;
        }

        if (ability == AbilityId::melee)
        {
                return (m_nr_wounds * -5);
        }
        else if (ability == AbilityId::dodging)
        {
                return (m_nr_wounds * -5);
        }

        return 0;
}

int PropWound::affect_max_hp(const int hp_max) const
{
        const int pen_pct_per_wound = 10;

        int hp_pen_pct = m_nr_wounds * pen_pct_per_wound;

        // The HP penalty is halved for a player with Survivalist
        if (m_owner->is_player() &&
            player_bon::has_trait(Trait::survivalist))
        {
                hp_pen_pct /= 2;
        }

        // Cap the penalty percentage
        hp_pen_pct = std::min(70, hp_pen_pct);

        return (hp_max * (100 - hp_pen_pct)) / 100;
}

void PropWound::heal_one_wound()
{
        ASSERT(m_nr_wounds > 0);

        --m_nr_wounds;

        if (m_nr_wounds > 0)
        {
                msg_log::add("A wound is healed.");
        }
        else // This was the last wound
        {
                // End self
                m_owner->m_properties.end_prop(id());
        }
}

void PropWound::on_more(const Prop& new_prop)
{
        (void)new_prop;

        ++m_nr_wounds;

        if (m_nr_wounds >= 5)
        {
                if (m_owner == map::g_player)
                {
                        msg_log::add("I die from my wounds!");
                }

                actor::kill(
                        *m_owner,
                        IsDestroyed::no,
                        AllowGore::no,
                        AllowDropItems::yes);
        }
}

PropHpSap::PropHpSap() :
        Prop(PropId::hp_sap),
        m_nr_drained(rnd::range(1, 3)) {}

void PropHpSap::save() const
{
        saving::put_int(m_nr_drained);
}

void PropHpSap::load()
{
        m_nr_drained = saving::get_int();
}

int PropHpSap::affect_max_hp(const int hp_max) const
{
        return (hp_max - m_nr_drained);
}

void PropHpSap::on_more(const Prop& new_prop)
{
        m_nr_drained +=
                static_cast<const PropHpSap*>(&new_prop)
                ->m_nr_drained;
}

PropSpiSap::PropSpiSap() :
        Prop(PropId::spi_sap),
        m_nr_drained(1) {}

void PropSpiSap::save() const
{
        saving::put_int(m_nr_drained);
}

void PropSpiSap::load()
{
        m_nr_drained = saving::get_int();
}

int PropSpiSap::affect_max_spi(const int spi_max) const
{
        return (spi_max - m_nr_drained);
}

void PropSpiSap::on_more(const Prop& new_prop)
{
        m_nr_drained +=
                static_cast<const PropSpiSap*>(&new_prop)
                ->m_nr_drained;
}

PropMindSap::PropMindSap() :
        Prop(PropId::mind_sap),
        m_nr_drained(rnd::range(1, 3)) {}

void PropMindSap::save() const
{
        saving::put_int(m_nr_drained);
}

void PropMindSap::load()
{
        m_nr_drained = saving::get_int();
}

int PropMindSap::affect_shock(const int shock) const
{
        return (shock + m_nr_drained);
}

void PropMindSap::on_more(const Prop& new_prop)
{
        m_nr_drained +=
                static_cast<const PropMindSap*>(&new_prop)
                ->m_nr_drained;
}

bool PropConfused::allow_read_absolute(const Verbosity verbosity) const
{
        if (m_owner->is_player() && verbosity == Verbosity::verbose)
        {
                msg_log::add("I am too confused to read.");
        }

        return false;
}

bool PropConfused::allow_cast_intr_spell_absolute(
        const Verbosity verbosity) const
{
        if (m_owner->is_player() &&
            (verbosity == Verbosity::verbose))
        {
                msg_log::add("I am too confused to concentrate!");
        }

        return false;
}

bool PropConfused::allow_attack_melee(const Verbosity verbosity) const
{
        (void)verbosity;

        if (m_owner != map::g_player)
        {
                return rnd::coin_toss();
        }

        return true;
}

bool PropConfused::allow_attack_ranged(const Verbosity verbosity) const
{
        (void)verbosity;

        if (m_owner != map::g_player)
        {
                return rnd::coin_toss();
        }

        return true;
}

PropEnded PropConfused::affect_move_dir(const P& actor_pos, Dir& dir)
{
        if (dir == Dir::center)
        {
                return PropEnded::no;
        }

        Array2<bool> blocked(map::dims());

        const R area_check_blocked(
                actor_pos - P(1, 1),
                actor_pos + P(1, 1));

        map_parsers::BlocksActor(*m_owner, ParseActors::yes)
                .run(blocked,
                     area_check_blocked,
                     MapParseMode::overwrite);

        if (rnd::one_in(8))
        {
                std::vector<P> d_bucket;

                for (const P& d : dir_utils::g_dir_list)
                {
                        const P tgt_p(actor_pos + d);

                        if (!blocked.at(tgt_p))
                        {
                                d_bucket.push_back(d);
                        }
                }

                if (!d_bucket.empty())
                {
                        const P& d = rnd::element(d_bucket);

                        dir = dir_utils::dir(d);
                }
        }

        return PropEnded::no;
}

PropEnded PropFrenzied::affect_move_dir(const P& actor_pos, Dir& dir)
{
        if (!m_owner->is_player() ||
            (dir == Dir::center))
        {
                return PropEnded::no;
        }

        const auto seen_foes = m_owner->seen_foes();

        if (seen_foes.empty())
        {
                return PropEnded::no;
        }

        std::vector<P> seen_foes_cells;

        seen_foes_cells.clear();

        for (auto* actor : seen_foes)
        {
                seen_foes_cells.push_back(actor->m_pos);
        }

        sort(begin(seen_foes_cells),
             end(seen_foes_cells),
             IsCloserToPos(actor_pos));

        const P& closest_mon_pos = seen_foes_cells[0];

        Array2<bool> blocked(map::dims());

        map_parsers::BlocksActor(*m_owner, ParseActors::no)
                .run(blocked, blocked.rect());

        const auto line =
                line_calc::calc_new_line(
                        actor_pos,
                        closest_mon_pos,
                        true,
                        999,
                        false);

        if (line.size() > 1)
        {
                for (const P& pos : line)
                {
                        if (blocked.at(pos))
                        {
                                return PropEnded::no;
                        }
                }

                dir = dir_utils::dir(line[1] - actor_pos);
        }

        return PropEnded::no;
}

bool PropFrenzied::is_resisting_other_prop(const PropId prop_id) const
{
        return
                prop_id == PropId::confused ||
                prop_id == PropId::fainted ||
                prop_id == PropId::terrified ||
                prop_id == PropId::weakened;
}

void PropFrenzied::on_applied()
{
        const PropEndConfig prop_end_config(
                PropEndAllowCallEndHook::no,
                PropEndAllowMsg::no,
                PropEndAllowHistoricMsg::yes);

        m_owner->m_properties.end_prop(PropId::confused, prop_end_config);
        m_owner->m_properties.end_prop(PropId::fainted, prop_end_config);
        m_owner->m_properties.end_prop(PropId::terrified, prop_end_config);
        m_owner->m_properties.end_prop(PropId::weakened, prop_end_config);
}

void PropFrenzied::on_end()
{
        // Only the player (except for Ghoul background) gets tired after a
        // frenzy (it looks weird for monsters)
        if (m_owner->is_player() && (player_bon::bg() != Bg::ghoul))
        {
                m_owner->m_properties.apply(new PropWeakened());
        }
}

bool PropFrenzied::allow_read_absolute(const Verbosity verbosity) const
{
        if (m_owner->is_player() && verbosity == Verbosity::verbose)
        {
                msg_log::add("I am too enraged to read!");
        }

        return false;
}

bool PropFrenzied::allow_cast_intr_spell_absolute(
        const Verbosity verbosity) const
{
        if (m_owner->is_player() &&
            (verbosity == Verbosity::verbose))
        {
                msg_log::add("I am too enraged to concentrate!");
        }

        return false;
}

PropEnded PropBurning::on_tick()
{
        if (m_owner->is_player())
        {
                msg_log::add("AAAARGH IT BURNS!!!", colors::light_red());
        }

        actor::hit(*m_owner, rnd::range(1, 3), DmgType::fire);

        return PropEnded::no;
}

bool PropBurning::allow_read_chance(const Verbosity verbosity) const
{
        if (!rnd::coin_toss())
        {
                if (m_owner->is_player() &&
                    (verbosity == Verbosity::verbose))
                {
                        msg_log::add("I fail to concentrate!");
                }

                return false;
        }

        return true;
}

bool PropBurning::allow_cast_intr_spell_chance(const Verbosity verbosity) const
{
        if (!rnd::coin_toss())
        {
                if (m_owner->is_player() &&
                    (verbosity == Verbosity::verbose))
                {
                        msg_log::add("I fail to concentrate!");
                }

                return false;
        }

        return true;
}

bool PropBurning::allow_attack_ranged(const Verbosity verbosity) const
{
        if (m_owner->is_player() && (verbosity == Verbosity::verbose))
        {
                msg_log::add("Not while burning.");
        }

        return false;
}

PropActResult PropRecloaks::on_act()
{
        if (m_owner->is_alive() &&
            !m_owner->m_properties.has(PropId::cloaked) &&
            rnd::one_in(8))
        {
                auto prop_cloaked = new PropCloaked();

                prop_cloaked->set_indefinite();

                m_owner->m_properties.apply(prop_cloaked);

                game_time::tick();

                PropActResult result;

                result.did_action = DidAction::yes;
                result.prop_ended = PropEnded::no;

                return result;
        }

        return PropActResult();
}

bool PropBlind::allow_read_absolute(const Verbosity verbosity) const
{
        if (m_owner->is_player() &&
            (verbosity == Verbosity::verbose))
        {
                msg_log::add("I cannot read while blind.");
        }

        return false;
}

bool PropBlind::should_update_vision_on_toggled() const
{
        return m_owner->is_player();
}

PropEnded PropParalyzed::on_tick()
{
        if (!m_owner->m_properties.has(PropId::swimming))
        {
                return PropEnded::no;
        }

        if (m_owner->is_player())
        {
                msg_log::add("I am drowning!", colors::msg_bad());
        }
        else if (map::g_player->can_see_actor(*m_owner))
        {
                const auto name_the =
                        text_format::first_to_upper(
                                m_owner->name_the());

                msg_log::add(name_the + " is drowning.", colors::msg_good());
        }

        actor::hit(*m_owner, 1, DmgType::physical);

        return PropEnded::no;
}

void PropParalyzed::on_applied()
{
        auto* const player = map::g_player;

        if (m_owner->is_player())
        {
                auto* const active_explosive = player->m_active_explosive;

                if (active_explosive)
                {
                        active_explosive->on_player_paralyzed();
                }
        }
}

bool PropFainted::should_update_vision_on_toggled() const
{
        return m_owner->is_player();
}

PropEnded PropFlared::on_tick()
{
        actor::hit(*m_owner, 1, DmgType::fire);

        if (m_nr_turns_left <= 1)
        {
                m_owner->m_properties.apply(new PropBurning());

                m_owner->m_properties.end_prop(id());

                return PropEnded::yes;
        }

        return PropEnded::no;
}

DmgResistData PropRAcid::is_resisting_dmg(const DmgType dmg_type) const
{
        DmgResistData d;

        d.is_resisted = (dmg_type == DmgType::acid);

        d.msg_resist_player = "I feel a faint burning sensation.";

        d.msg_resist_mon = "seems unaffected.";

        return d;
}

DmgResistData PropRElec::is_resisting_dmg(const DmgType dmg_type) const
{
        DmgResistData d;

        d.is_resisted = (dmg_type == DmgType::electric);

        d.msg_resist_player = "I feel a faint tingle.";

        d.msg_resist_mon = "seems unaffected.";

        return d;
}

bool PropRConf::is_resisting_other_prop(const PropId prop_id) const
{
        return prop_id == PropId::confused;
}

void PropRConf::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::confused,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

bool PropRFear::is_resisting_other_prop(const PropId prop_id) const
{
        return prop_id == PropId::terrified;
}

void PropRFear::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::terrified,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );

        if (m_owner->is_player() &&
            m_duration_mode == PropDurationMode::indefinite)
        {
                insanity::on_permanent_rfear();
        }
}

bool PropRSlow::is_resisting_other_prop(const PropId prop_id) const
{
        return prop_id == PropId::slowed;
}

void PropRSlow::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::slowed,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

bool PropRPhys::is_resisting_other_prop(const PropId prop_id) const
{
        (void)prop_id;
        return false;
}

void PropRPhys::on_applied()
{

}

DmgResistData PropRPhys::is_resisting_dmg(const DmgType dmg_type) const
{
        DmgResistData d;

        d.is_resisted = (dmg_type == DmgType::physical);

        d.msg_resist_player = "I resist harm.";

        d.msg_resist_mon = "seems unharmed.";

        return d;
}

bool PropRFire::is_resisting_other_prop(const PropId prop_id) const
{
        return prop_id == PropId::burning;
}

void PropRFire::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::burning,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

DmgResistData PropRFire::is_resisting_dmg(const DmgType dmg_type) const
{
        DmgResistData d;

        d.is_resisted = (dmg_type == DmgType::fire);

        d.msg_resist_player = "I feel warm.";

        d.msg_resist_mon = "seems unaffected.";

        return d;
}

bool PropRPoison::is_resisting_other_prop(const PropId prop_id) const
{
        return prop_id == PropId::poisoned;
}

void PropRPoison::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::poisoned,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

bool PropRSleep::is_resisting_other_prop(const PropId prop_id) const
{
        return prop_id == PropId::fainted;
}

void PropRSleep::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::fainted,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

bool PropRDisease::is_resisting_other_prop(const PropId prop_id) const
{
        return prop_id == PropId::diseased || prop_id == PropId::infected;
}

void PropRDisease::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::diseased,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );

        m_owner->m_properties.end_prop(
                PropId::infected,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

bool PropRBlind::is_resisting_other_prop(const PropId prop_id) const
{
        return prop_id == PropId::blind;
}

void PropRBlind::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::blind,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

bool PropRPara::is_resisting_other_prop(const PropId prop_id) const
{
        return prop_id == PropId::paralyzed;
}

void PropRPara::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::paralyzed,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

bool PropSeeInvis::is_resisting_other_prop(const PropId prop_id) const
{
        return prop_id == PropId::blind;
}

void PropSeeInvis::on_applied()
{
        m_owner->m_properties.end_prop(
                PropId::blind,
                PropEndConfig(
                        PropEndAllowCallEndHook::no,
                        PropEndAllowMsg::no,
                        PropEndAllowHistoricMsg::yes)
                );
}

PropEnded PropBurrowing::on_tick()
{
        const P& p = m_owner->m_pos;

        map::g_cells.at(p).terrain->hit(
                1, // Doesn't matter
                DmgType::physical,
                DmgMethod::forced);

        return PropEnded::no;
}

PropActResult PropVortex::on_act()
{
        TRACE_FUNC_BEGIN;

        // Not supported yet
        if (m_owner->is_player())
        {
                return PropActResult();
        }

        if (!m_owner->is_alive())
        {
                return PropActResult();
        }

        if (pull_cooldown > 0)
        {
                --pull_cooldown;
        }

        auto* const mon = static_cast<actor::Mon*>(m_owner);

        if ((mon->m_aware_of_player_counter <= 0) ||
            (pull_cooldown > 0))
        {
                return PropActResult();
        }

        const P& player_pos = map::g_player->m_pos;

        if (is_pos_adj(mon->m_pos, player_pos, true) ||
            !rnd::coin_toss())
        {
                return PropActResult();
        }

        TRACE << "Monster with vortex property attempting to pull player"
              << std::endl;

        const P delta = player_pos - mon->m_pos;

        P knockback_from_pos = player_pos;

        if (delta.x >  1)
        {
                ++knockback_from_pos.x;
        }

        if (delta.x < -1)
        {
                --knockback_from_pos.x;
        }

        if (delta.y >  1)
        {
                ++knockback_from_pos.y;
        }

        if (delta.y < -1)
        {
                --knockback_from_pos.y;
        }

        if (knockback_from_pos == player_pos)
        {
                return PropActResult();
        }

        TRACE << "Pos found to knockback player from: "
              << knockback_from_pos.x << ", "
              << knockback_from_pos.y << std::endl;

        TRACE << "Player pos: "
              << player_pos.x << ", " << player_pos.y << std::endl;

        Array2<bool> blocked_los(map::dims());

        const R fov_rect = fov::fov_rect(mon->m_pos, blocked_los.dims());

        map_parsers::BlocksLos()
                .run(blocked_los,
                     fov_rect,
                     MapParseMode::overwrite);

        if (mon->can_see_actor(*(map::g_player), blocked_los))
        {
                TRACE << "Is seeing player" << std::endl;

                mon->set_player_aware_of_me();

                if (map::g_player->can_see_actor(*mon))
                {
                        const auto name_the = text_format::first_to_upper(
                                m_owner->name_the());

                        msg_log::add(name_the + " pulls me!");
                }
                else
                {
                        msg_log::add("A powerful wind is pulling me!");
                }

                TRACE << "Attempt pull (knockback)" << std::endl;

                // TODO: Add sfx
                knockback::run(
                        *map::g_player,
                        knockback_from_pos,
                        false,
                        Verbosity::silent);

                pull_cooldown = 2;

                game_time::tick();

                PropActResult result;

                result.did_action = DidAction::yes;
                result.prop_ended = PropEnded::no;

                return result;
        }

        return PropActResult();
}

void PropExplodesOnDeath::on_death()
{
        TRACE_FUNC_BEGIN;

        // Setting the actor to destroyed here, just to make sure there will not
        // be a recursive call chain due to explosion damage to the corpse
        m_owner->m_state = ActorState::destroyed;

        explosion::run(m_owner->m_pos, ExplType::expl);

        TRACE_FUNC_END;
}

void PropSplitsOnDeath::on_death()
{
        // Not supported yet
        if (m_owner->is_player())
        {
                return;
        }

        const int actor_max_hp = actor::max_hp(*m_owner);

        // Do not allow splitting if HP is reduced to this point (if the monster
        // is killed "hard" enough, it doesn't split)
        const bool is_very_destroyed =
                m_owner->m_hp <=
                (actor_max_hp * (-5));

        const P pos = m_owner->m_pos;

        const auto f_id = map::g_cells.at(pos).terrain->id();

        if (is_very_destroyed ||
            f_id == terrain::Id::chasm ||
            f_id == terrain::Id::liquid_deep ||
            (game_time::g_actors.size() >= g_max_nr_actors_on_map))
        {
                return;
        }

        auto* const leader = static_cast<actor::Mon*>(m_owner)->m_leader;

        const auto spawned =
                actor::spawn(pos, {2, m_owner->id()}, map::rect())
                .make_aware_of_player()
                .set_leader(leader);

        std::for_each(
                std::begin(spawned.monsters),
                std::end(spawned.monsters),
                [this](auto* const mon)
                {
                        auto prop_waiting = new PropWaiting();

                        prop_waiting->set_duration(1);

                        mon->m_properties.apply(prop_waiting);

                        // The new actors should usually not also split
                        if (rnd::fraction(4, 5))
                        {
                                mon->m_properties.end_prop(
                                        PropId::splits_on_death);
                        }

                        // If the original actor is burning, the spawned actors
                        // should too
                        if (m_owner->m_properties.has(PropId::burning))
                        {
                                mon->m_properties.apply(
                                        new PropBurning(),
                                        PropSrc::intr,
                                        false, // Do not force effect
                                        Verbosity::silent);
                        }
                });

        // If no leader yet, set the first actor as leader of the second
        if (!leader && (spawned.monsters.size() == 2))
        {
                spawned.monsters[1]->m_leader = spawned.monsters[0];
        }
}

PropActResult PropCorpseEater::on_act()
{
        auto did_action = DidAction::no;

        if (m_owner->is_alive() && rnd::coin_toss())
        {
                did_action = m_owner->try_eat_corpse();

                if (did_action == DidAction::yes)
                {
                        game_time::tick();
                }
        }

        PropActResult result;

        result.did_action = did_action;
        result.prop_ended = PropEnded::no;

        return result;
}

PropActResult PropTeleports::on_act()
{
        auto did_action = DidAction::no;

        const int teleport_one_in_n = 12;

        if (m_owner->is_alive() && rnd::one_in(teleport_one_in_n))
        {
                teleport(*m_owner);

                game_time::tick();

                did_action = DidAction::yes;
        }

        PropActResult result;

        result.did_action = did_action;
        result.prop_ended = PropEnded::no;

        return result;
}

PropActResult PropCorruptsEnvColor::on_act()
{
        const auto pos = m_owner->m_pos;

        auto* r = map::g_cells.at(pos).terrain;

        r->corrupt_color();

        return PropActResult();
}

void PropAltersEnv::on_std_turn()
{
        Array2<bool> blocked(map::dims());

        map_parsers::BlocksWalking(ParseActors::no)
                .run(blocked, blocked.rect());

        const std::vector<terrain::Id> free_terrains = {
                terrain::Id::door,
                terrain::Id::liquid_deep,
        };

        for (int x = 0; x < blocked.w(); ++x)
        {
                for (int y = 0; y < blocked.h(); ++y)
                {
                        const P p(x, y);

                        if (map_parsers::IsAnyOfTerrains(free_terrains).cell(p))
                        {
                                blocked.at(p) = false;
                        }
                }
        }

        Array2<bool> has_actor(map::dims());

        for (auto actor : game_time::g_actors)
        {
                has_actor.at(actor->m_pos) = true;
        }

        const int r = 3;

        const int x0 = std::max(
                1,
                m_owner->m_pos.x - r);

        const int y0 = std::max(
                1,
                m_owner->m_pos.y - r);

        const int x1 = std::min(
                map::w() - 2,
                m_owner->m_pos.x + r);

        const int y1 = std::min(
                map::h() - 2,
                m_owner->m_pos.y + r);

        for (int x = x0; x <= x1; ++x)
        {
                for (int y = y0; y <= y1; ++y)
                {
                        if (has_actor.at(x, y) ||
                            map::g_cells.at(x, y).item ||
                            !rnd::one_in(6))
                        {
                                continue;
                        }

                        const P pos(x, y);

                        const auto current_id =
                                map::g_cells.at(x, y).terrain->id();

                        if (current_id == terrain::Id::wall)
                        {
                                map::put(new terrain::Floor(pos));

                                blocked.at(x, y) = false;
                        }
                        else if (current_id == terrain::Id::floor)
                        {
                                blocked.at(x, y) = true;

                                if (map_parsers::is_map_connected(blocked))
                                {
                                        map::put(new terrain::Wall(pos));
                                }
                                else
                                {
                                        blocked.at(x, y) = false;
                                }
                        }
                }
        }
}

void PropRegenerates::on_std_turn()
{
        if (m_owner->is_alive() &&
            !m_owner->m_properties.has(PropId::burning))
        {
                m_owner->restore_hp(2, false, Verbosity::silent);
        }
}

PropActResult PropCorpseRises::on_act()
{
        const auto pos = m_owner->m_pos;

        if (!m_owner->is_corpse() ||
            map::first_actor_at_pos(m_owner->m_pos) ||
            (map::g_cells.at(pos).terrain->id() == terrain::Id::liquid_deep))
        {
                return PropActResult();
        }

        if (m_nr_turns_until_allow_rise > 0)
        {
                --m_nr_turns_until_allow_rise;

                return PropActResult();
        }

        const int rise_one_in_n = 9;

        if (!rnd::one_in(rise_one_in_n))
        {
                return PropActResult();
        }

        m_owner->m_state = ActorState::alive;

        m_owner->m_hp = actor::max_hp(*m_owner) / 2;

        --m_owner->m_data->nr_kills;

        if (map::g_cells.at(pos).is_seen_by_player)
        {
                ASSERT(!m_owner->m_data->corpse_name_the.empty());

                const std::string name =
                        text_format::first_to_upper(
                                m_owner->m_data->corpse_name_the);

                msg_log::add(
                        name +
                        " rises again!!",
                        colors::text(),
                        true);

                map::g_player->incr_shock(
                        ShockLvl::frightening,
                        ShockSrc::see_mon);
        }

        static_cast<actor::Mon*>(m_owner)->become_aware_player(false);

        game_time::tick();

        m_has_risen = true;

        PropActResult result;

        result.did_action = DidAction::yes;
        result.prop_ended = PropEnded::no;

        return result;
}

void PropCorpseRises::on_death()
{
        // If we have already risen before, and were killed again leaving a
        // corpse, destroy the corpse to prevent rising multiple times
        if (m_owner->is_corpse() && m_has_risen)
        {
                m_owner->m_state = ActorState::destroyed;

                m_owner->m_properties.on_destroyed_alive();
        }
}

void PropSpawnsZombiePartsOnDestroyed::on_destroyed_alive()
{
        try_spawn_zombie_dust();

        try_spawn_zombie_parts();
}

void PropSpawnsZombiePartsOnDestroyed::on_destroyed_corpse()
{
        // NOTE: We do not spawn zombie parts when the corpse is destroyed (it's
        // pretty annoying if parts are spawned when you bash a corpse)
        try_spawn_zombie_dust();
}

void PropSpawnsZombiePartsOnDestroyed::try_spawn_zombie_parts() const
{
        if (!is_allowed_to_spawn_parts_here())
        {
                return;
        }

        const auto pos = m_owner->m_pos;

        // Spawning zombie part monsters is only allowed if the monster is not
        // destroyed "too hard". This is also rewarding heavy weapons, since
        // they will more often prevent spawning
        const bool is_very_destroyed = (m_owner->m_hp <= -8);

        const int summon_one_in_n = 5;

        if (is_very_destroyed || !rnd::one_in(summon_one_in_n))
        {
                return;
        }

        auto id_to_spawn = actor::Id::END;

        const std::vector<int> weights = {
                25,     // Hand
                25,     // Intestines
                1       // Floating skull
        };

        const int mon_choice = rnd::weighted_choice(weights);

        const std::string my_name = m_owner->name_the();

        std::string spawn_msg = "";

        switch (mon_choice)
        {
        case 0:
                id_to_spawn = actor::Id::crawling_hand;

                spawn_msg =
                        "The hand of " +
                        my_name +
                        " comes off and starts crawling around!";
                break;

        case 1:
                id_to_spawn = actor::Id::crawling_intestines;

                spawn_msg =
                        "The intestines of " +
                        my_name +
                        " starts crawling around!";
                break;

        case 2:
                id_to_spawn = actor::Id::floating_skull;

                spawn_msg =
                        "The head of " +
                        my_name +
                        " starts floating around!";
                break;

        }

        if (map::g_cells.at(pos).is_seen_by_player)
        {
                ASSERT(!spawn_msg.empty());

                msg_log::add(spawn_msg);

                map::g_player->incr_shock(
                        ShockLvl::frightening,
                        ShockSrc::see_mon);
        }

        ASSERT(id_to_spawn != actor::Id::END);

        const auto spawned = actor::spawn(
                pos,
                {id_to_spawn},
                map::rect())
                .make_aware_of_player();

        std::for_each(
                std::begin(spawned.monsters),
                std::end(spawned.monsters),
                [](auto* const mon)
                {
                        auto* waiting = new PropWaiting();

                        waiting->set_duration(1);

                        mon->m_properties.apply(waiting);
                });
}

void PropSpawnsZombiePartsOnDestroyed::try_spawn_zombie_dust() const
{
        if (!is_allowed_to_spawn_parts_here())
        {
                return;
        }

        const int make_dust_one_in_n = 7;

        if (rnd::one_in(make_dust_one_in_n))
        {
                item::make_item_on_floor(item::Id::zombie_dust, m_owner->m_pos);
        }
}

bool PropSpawnsZombiePartsOnDestroyed::is_allowed_to_spawn_parts_here() const
{
        const P& pos = m_owner->m_pos;

        const auto f_id = map::g_cells.at(pos).terrain->id();

        return
                (f_id != terrain::Id::chasm) &&
                (f_id != terrain::Id::liquid_deep);
}

void PropBreeds::on_std_turn()
{
        const int spawn_new_one_in_n = 50;

        if (m_owner->is_player() ||
            !m_owner->is_alive() ||
            m_owner->m_properties.has(PropId::burning) ||
            (game_time::g_actors.size() >= g_max_nr_actors_on_map) ||
            !rnd::one_in(spawn_new_one_in_n))
        {
                return;
        }

        auto* const mon = static_cast<actor::Mon*>(m_owner);

        auto* const leader_of_spawned_mon =
                mon->m_leader
                ? mon->m_leader
                : mon;

        const auto area_allowed = R(mon->m_pos - 1, mon->m_pos + 1);

        auto spawned =
                actor::spawn_random_position(
                        {mon->id()},
                        area_allowed)
                .set_leader(leader_of_spawned_mon);

        std::for_each(
                std::begin(spawned.monsters),
                std::end(spawned.monsters),
                [](auto* const spawned_mon)
                {
                        auto prop_waiting = new PropWaiting();

                        prop_waiting->set_duration(2);

                        spawned_mon->m_properties.apply(prop_waiting);
                });

        if (mon->m_aware_of_player_counter > 0)
        {
                spawned.make_aware_of_player();
        }
}

void PropConfusesAdjacent::on_std_turn()
{
        if (!m_owner->is_alive() ||
            !map::g_player->can_see_actor(*m_owner) ||
            !map::g_player->m_pos.is_adjacent(m_owner->m_pos))
        {
                return;
        }

        if (!map::g_player->m_properties.has(PropId::confused))
        {
                const std::string msg =
                        text_format::first_to_upper(m_owner->name_the()) +
                        " bewilders me.";

                msg_log::add(msg);
        }

        auto prop_confusd = new PropConfused();

        prop_confusd->set_duration(rnd::range(8, 12));

        map::g_player->m_properties.apply(prop_confusd);
}

PropActResult PropSpeaksCurses::on_act()
{
        if (m_owner->is_player())
        {
                return PropActResult();
        }

        auto* const mon = static_cast<actor::Mon*>(m_owner);

        if (!mon->is_alive() ||
            (mon->m_aware_of_player_counter <= 0) ||
            !rnd::one_in(4))
        {
                return PropActResult();
        }

        Array2<bool> blocked_los(map::dims());

        const R fov_rect = fov::fov_rect(mon->m_pos, blocked_los.dims());

        map_parsers::BlocksLos()
                .run(blocked_los,
                     fov_rect,
                     MapParseMode::overwrite);

        if (mon->can_see_actor(*map::g_player, blocked_los))
        {
                const bool player_see_owner =
                        map::g_player->can_see_actor(*mon);

                std::string snd_msg =
                        player_see_owner ?
                        text_format::first_to_upper(mon->name_the()) :
                        "Someone";

                snd_msg += " spews forth a litany of curses.";

                Snd snd(snd_msg,
                        SfxId::END,
                        IgnoreMsgIfOriginSeen::no,
                        mon->m_pos,
                        mon,
                        SndVol::high,
                        AlertsMon::no);

                snd_emit::run(snd);

                map::g_player->m_properties.apply(new PropCursed());

                game_time::tick();

                PropActResult result;

                result.did_action = DidAction::yes;
                result.prop_ended = PropEnded::no;

                return result;
        }

        return PropActResult();
}

void PropAuraOfDecay::save() const
{
        saving::put_int(m_dmg);
}

void PropAuraOfDecay::load()
{
        m_dmg = saving::get_int();
}

int PropAuraOfDecay::range() const
{
        return g_expl_std_radi;
}

void PropAuraOfDecay::on_std_turn()
{
        run_effect_on_actors();

        run_effect_on_env();
}

void PropAuraOfDecay::run_effect_on_actors() const
{
        for (auto* const actor : game_time::g_actors)
        {
                if (actor == m_owner ||
                    actor->m_state == ActorState::destroyed ||
                    king_dist(m_owner->m_pos, actor->m_pos) > range())
                {
                        continue;
                }

                const bool player_see_target =
                        map::g_player->can_see_actor(*actor);

                if (player_see_target)
                {
                        print_msg_actor_hit(*actor);
                }

                actor::hit(*actor, m_dmg, DmgType::pure);
        }
}

void PropAuraOfDecay::run_effect_on_env() const
{
        for (const P& d : dir_utils::g_dir_list)
        {
                const P p = m_owner->m_pos + d;

                if (!map::is_pos_inside_outer_walls(p))
                {
                        continue;
                }

                auto& cell = map::g_cells.at(p);

                auto* const terrain = cell.terrain;

                const auto id = terrain->id();

                if ((id == terrain::Id::wall ||
                     id == terrain::Id::rubble_high ||
                     id == terrain::Id::door) &&
                    rnd::one_in(250))
                {
                        if (cell.is_seen_by_player)
                        {
                                const std::string name =
                                        text_format::first_to_upper(
                                                terrain->name(Article::the));

                                msg_log::add(name + " collapses!");

                                msg_log::more_prompt();
                        }

                        terrain->hit(
                                1, // Doesn't actually matter
                                DmgType::physical,
                                DmgMethod::forced);
                }
                else if (id == terrain::Id::floor &&
                         rnd::one_in(100))
                {
                        map::put(new terrain::RubbleLow(p));
                }
                else if (id == terrain::Id::grass &&
                         rnd::one_in(10))
                {
                        static_cast<terrain::Grass*>(terrain)->m_type =
                                terrain::GrassType::withered;
                }
                else if (id == terrain::Id::fountain)
                {
                        static_cast<terrain::Fountain*>(terrain)->curse();
                }
        }
}

void PropAuraOfDecay::print_msg_actor_hit(const actor::Actor& actor) const
{
        if (actor.is_player())
        {
                msg_log::add("I am decaying!", colors::msg_bad());
        }
        // else // Monster is hit
        // {
        //         const std::string actor_name =
        //                 (actor.state() == ActorState::alive)
        //                 ? actor.name_the()
        //                 : actor.corpse_name_the();

        //         const Color msg_color =
        //                 (map::g_player->is_leader_of(&actor))
        //                 ? colors::text()
        //                 : colors::msg_good();

        //         msg_log::add(
        //                 text_format::first_to_upper(actor_name) + " decays!",
        //                 msg_color);
        // }
}

PropActResult PropMajorClaphamSummon::on_act()
{
        if (m_owner->is_player())
        {
                return PropActResult();
        }

        auto* const mon = static_cast<actor::Mon*>(m_owner);

        if (!mon->is_alive() ||
            (mon->m_aware_of_player_counter <= 0))
        {
                return PropActResult();
        }

        Array2<bool> blocked_los(map::dims());

        const R fov_rect = fov::fov_rect(mon->m_pos, blocked_los.dims());

        map_parsers::BlocksLos()
                .run(blocked_los,
                     fov_rect,
                     MapParseMode::overwrite);

        if (!mon->can_see_actor(*(map::g_player), blocked_los))
        {
                return PropActResult();
        }

        mon->set_player_aware_of_me();

        msg_log::add("Major Clapham Lee calls forth his Tomb-Legions!");

        std::vector<actor::Id> ids_to_summon = {actor::Id::dean_halsey};

        const int nr_of_extra_spawns = 4;

        const std::vector<actor::Id> possible_random_id_choices = {
                actor::Id::zombie,
                actor::Id::bloated_zombie
        };

        const std::vector<int> weights = {
                3,
                1
        };

        for (int i = 0; i < nr_of_extra_spawns; ++i)
        {
                const int idx = rnd::weighted_choice(weights);

                ids_to_summon.push_back(possible_random_id_choices[idx]);
        }

        auto spawned =
                actor::spawn(mon->m_pos, ids_to_summon, map::rect())
                .make_aware_of_player()
                .set_leader(mon);

        std::for_each(
                std::begin(spawned.monsters),
                std::end(spawned.monsters),
                [](auto* const spawned_mon)
                {
                        auto prop_summoned = new PropSummoned();

                        prop_summoned->set_indefinite();

                        spawned_mon->m_properties.apply(prop_summoned);

                        spawned_mon->m_is_player_feeling_msg_allowed = false;
                });

        map::g_player->incr_shock(ShockLvl::terrifying, ShockSrc::misc);

        mon->m_properties.end_prop(id());

        game_time::tick();

        PropActResult result;

        result.did_action = DidAction::yes;
        result.prop_ended = PropEnded::yes;

        return result;
}

void PropMagicSearching::save() const
{
        saving::put_int(m_range);

        saving::put_bool(m_allow_reveal_items);
}

void PropMagicSearching::load()
{
        m_range = saving::get_int();

        m_allow_reveal_items = saving::get_bool();
}

PropEnded PropMagicSearching::on_tick()
{
        ASSERT(m_owner->is_player());

        const int orig_x = map::g_player->m_pos.x;
        const int orig_y = map::g_player->m_pos.y;

        const int x0 = std::max(
                0,
                orig_x - m_range);

        const int y0 = std::max(
                0,
                orig_y - m_range);

        const int x1 = std::min(
                map::w() - 1,
                orig_x + m_range);

        const int y1 = std::min(
                map::h() - 1,
                orig_y + m_range);

        for (int y = y0; y <= y1; ++y)
        {
                for (int x = x0; x <= x1; ++x)
                {
                        auto& cell = map::g_cells.at(x, y);

                        auto* const t = cell.terrain;

                        const auto id = t->id();

                        if ((id == terrain::Id::trap) ||
                            (id == terrain::Id::door) ||
                            (id == terrain::Id::monolith) ||
                            (id == terrain::Id::stairs))
                        {
                                cell.is_seen_by_player = true;

                                cell.is_explored = true;

                                if (t->is_hidden())
                                {
                                        t->reveal(Verbosity::verbose);

                                        t->on_revealed_from_searching();

                                        msg_log::more_prompt();
                                }
                        }

                        if (m_allow_reveal_items && cell.item)
                        {
                                cell.is_seen_by_player = true;

                                cell.is_explored = true;
                        }
                }
        }

        const int det_mon_multiplier = 20;

        for (auto* actor : game_time::g_actors)
        {
                const P& p = actor->m_pos;

                if (actor->is_player() ||
                    !actor->is_alive() ||
                    (king_dist(map::g_player->m_pos, p) > m_range))
                {
                        continue;
                }

                static_cast<actor::Mon*>(actor)->set_player_aware_of_me(
                        det_mon_multiplier);
        }

        states::draw();

        map::g_player->update_fov();

        states::draw();

        return PropEnded::no;
}

bool PropSwimming::allow_read_absolute(const Verbosity verbosity) const
{
        if (m_owner->is_player() && verbosity == Verbosity::verbose)
        {
                msg_log::add("I cannot read this while swimming.");
        }

        return false;
}

bool PropSwimming::allow_attack_ranged(const Verbosity verbosity) const
{
        if (m_owner->is_player() && (verbosity == Verbosity::verbose))
        {
                msg_log::add("Not while swimming.");
        }

        return false;
}

bool PropCannotReadCurse::allow_read_absolute(const Verbosity verbosity) const
{
        if (m_owner->is_player() && verbosity == Verbosity::verbose)
        {
                msg_log::add("I cannot read it.");
        }

        return false;
}
