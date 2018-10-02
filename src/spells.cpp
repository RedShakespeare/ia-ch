#include "spells.hpp"

#include <algorithm>
#include <vector>

#include "actor_factory.hpp"
#include "actor_hit.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "explosion.hpp"
#include "feature_door.hpp"
#include "game.hpp"
#include "init.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "item_factory.hpp"
#include "item_scroll.hpp"
#include "knockback.hpp"
#include "line_calc.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "player_spells.hpp"
#include "postmortem.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_handler.hpp"
#include "sdl_base.hpp"
#include "teleport.hpp"
#include "text_format.hpp"
#include "viewport.hpp"

namespace spell_factory
{

Spell* make_spell_from_id(const SpellId spell_id)
{
        switch (spell_id)
        {
        case SpellId::aura_of_decay:
                return new SpellAuraOfDecay();

        case SpellId::enfeeble:
                return new SpellEnfeeble();

        case SpellId::slow:
                return new SpellSlow();

        case SpellId::terrify:
                return new SpellTerrify();

        case SpellId::disease:
                return new SpellDisease();

        case SpellId::force_bolt:
                return new SpellBolt(new ForceBolt);

        case SpellId::darkbolt:
                return new SpellBolt(new Darkbolt);

        case SpellId::aza_wrath:
                return new SpellAzaWrath();

        case SpellId::summon:
                return new SpellSummonMon();

        case SpellId::summon_tentacles:
                return new SpellSummonTentacles();

        case SpellId::heal:
                return new SpellHeal();

        case SpellId::knockback:
                return new SpellKnockBack();

        case SpellId::teleport:
                return new SpellTeleport();

        case SpellId::mayhem:
                return new SpellMayhem();

        case SpellId::pestilence:
                return new SpellPestilence();

        case SpellId::spectral_wpns:
                return new SpellSpectralWpns();

        case SpellId::searching:
                return new SpellSearching();

        case SpellId::opening:
                return new SpellOpening();

        case SpellId::frenzy:
                return new SpellFrenzy();

        case SpellId::bless:
                return new SpellBless();

        case SpellId::mi_go_hypno:
                return new SpellMiGoHypno();

        case SpellId::burn:
                return new SpellBurn();

        case SpellId::deafen:
                return new SpellDeafen();

        case SpellId::res:
                return new SpellRes();

        case SpellId::pharaoh_staff:
                return new SpellPharaohStaff();

        case SpellId::light:
                return new SpellLight();

        case SpellId::transmut:
                return new SpellTransmut();

        case SpellId::see_invis:
                return new SpellSeeInvis();

        case SpellId::spell_shield:
                return new SpellSpellShield();

        case SpellId::slow_time:
                return new SpellSlowTime();

        case SpellId::divert_attacks:
                return new SpellDivertAttacks();

        case SpellId::identify:
                return new SpellIdentify();

        case SpellId::END:
                break;
        }

        ASSERT(false);

        return nullptr;
}

} // spell_factory

StateId BrowseSpell::id()
{
        return StateId::browse_spells;
}

Range Spell::spi_cost(const SpellSkill skill, Actor* const caster) const
{
        int cost_max = max_spi_cost(skill);

        if (caster == map::player)
        {
                // Standing next to an altar reduces the cost
                const int x0 = std::max(
                        0,
                        caster->pos.x - 1);

                const int y0 = std::max(
                        0,
                        caster->pos.y - 1);

                const int x1 = std::min(
                        map::w() - 1,
                        caster->pos.x + 1);

                const int y1 = std::min(
                        map::h() - 1,
                        caster->pos.y + 1);

                for (int x = x0; x <= x1; ++x)
                {
                        for (int y = y0; y <= y1; ++y)
                        {
                                if (map::cells.at(x, y).rigid->id() ==
                                    FeatureId::altar)
                                {
                                        cost_max -= 1;
                                }
                        }
                }
        }

        cost_max = std::max(1, cost_max);

        const int cost_min = std::max(1, (cost_max + 1) / 2);

        return Range(cost_min, cost_max);
}

void Spell::cast(Actor* const caster,
                 const SpellSkill skill,
                 const SpellSrc spell_src) const
{
        TRACE_FUNC_BEGIN;

        ASSERT(caster);

        auto& properties = caster->properties;

        // If this is an intrinsic cast, check properties which NEVER allows
        // casting or speaking

        // NOTE: If this is a non-intrinsic cast (e.g. from a scroll), then we
        // assume that the caller has made all checks themselves
        if ((spell_src == SpellSrc::learned) &&
            (!properties.allow_cast_intr_spell_absolute(Verbosity::verbose) ||
             !properties.allow_speak(Verbosity::verbose)))
        {
                return;
        }

        // OK, we can try to cast

        if (caster->is_player())
        {
                TRACE << "Player casting spell" << std::endl;

                const ShockSrc shock_src =
                        (spell_src == SpellSrc::learned) ?
                        ShockSrc::cast_intr_spell :
                        ShockSrc::use_strange_item;

                const int value = shock_value();

                map::player->incr_shock((double)value, shock_src);

                // Make sound if noisy - casting from scrolls is always noisy
                if (is_noisy(skill) ||
                    (spell_src == SpellSrc::manuscript))
                {
                        Snd snd("",
                                SfxId::spell_generic,
                                IgnoreMsgIfOriginSeen::yes,
                                caster->pos,
                                caster,
                                SndVol::low,
                                AlertsMon::yes);

                        snd_emit::run(snd);
                }
        }
        else // Caster is monster
        {
                TRACE << "Monster casting spell" << std::endl;

                // Make sound if noisy - casting from scrolls is always noisy
                if (is_noisy(skill) ||
                    (spell_src == SpellSrc::manuscript))
                {
                        Mon* const mon = static_cast<Mon*>(caster);

                        const bool is_mon_seen =
                                map::player->can_see_actor(*mon);

                        std::string spell_msg = mon->data->spell_msg;

                        if (!spell_msg.empty())
                        {
                                std::string mon_name = "";

                                if (is_mon_seen)
                                {
                                        mon_name =
                                                text_format::first_to_upper(
                                                        mon->name_the());
                                }
                                else // Cannot see monster
                                {
                                        mon_name =
                                                mon->data->is_humanoid
                                                ? "Someone"
                                                : "Something";
                                }

                                spell_msg = mon_name + " " + spell_msg;
                        }

                        Snd snd(spell_msg,
                                SfxId::END,
                                IgnoreMsgIfOriginSeen::no,
                                caster->pos,
                                caster,
                                SndVol::low,
                                AlertsMon::no);

                        snd_emit::run(snd);
                }
        }

        bool allow_cast = true;

        if (spell_src != SpellSrc::manuscript)
        {
                const Range cost = spi_cost(skill, caster);

                actor::hit_sp(*caster, cost.roll(), Verbosity::silent);

                // Check properties which MAY allow casting with a random chance
                allow_cast =
                        properties.allow_cast_intr_spell_chance(
                                Verbosity::verbose);
        }

        if (allow_cast && caster->is_alive())
        {
                run_effect(caster, skill);
        }

        // Casting spells ends cloaking
        caster->properties.end_prop(PropId::cloaked);

        game_time::tick();

        TRACE_FUNC_END;
}

void Spell::on_resist(Actor& target) const
{
        const bool is_player = target.is_player();

        const bool player_see_target = map::player->can_see_actor(target);

        if (player_see_target)
        {
                msg_log::add(spell_resist_msg);

                if (is_player)
                {
                        audio::play(SfxId::spell_shield_break);
                }

                io::draw_blast_at_cells({target.pos}, colors::white());
        }

        // TODO: Only end r_spell if this is not a natural property
        target.properties.end_prop(PropId::r_spell);

        if (is_player &&
            player_bon::traits[(size_t)Trait::absorb])
        {
                map::player->restore_sp(
                        rnd::dice(1, 6),
                        false, // Not allowed above max
                        Verbosity::verbose);
        }
}

std::vector<std::string> Spell::descr(const SpellSkill skill,
                                      const SpellSrc spell_src) const
{
        auto ret = descr_specific(skill);

        if (spell_src != SpellSrc::manuscript)
        {
                if (is_noisy(skill))
                {
                        ret.push_back(
                                "Casting this spell requires making sounds.");
                }
                else // The spell is silent
                {
                        ret.push_back(
                                "This spell can be cast silently.");
                }
        }

        if (can_be_improved_with_skill())
        {
                std::string skill_str = "";

                switch (skill)
                {
                case SpellSkill::basic:
                        skill_str = "Basic";
                        break;

                case SpellSkill::expert:
                        skill_str = "Expert";
                        break;

                case SpellSkill::master:
                        skill_str = "Master";
                        break;
                }

                if (spell_src == SpellSrc::manuscript)
                {
                        skill_str += " (casting from Manuscript)";
                }

                ret.push_back("Skill level: " + skill_str);
        }

        return ret;
}

int Spell::shock_value() const
{
        const SpellShock type = shock_type();

        int value = 0;

        switch (type)
        {
        case SpellShock::mild:
                value = 4;
                break;

        case SpellShock::disturbing:
                value = 16;
                break;

        case SpellShock::severe:
                value = 24;
                break;
        }

        // Blessed/cursed affects shock
        if (map::player->properties.has(PropId::blessed))
        {
                value -= 2;
        }
        else if (map::player->properties.has(PropId::cursed))
        {
                value += 2;
        }

        return value;
}

// -----------------------------------------------------------------------------
// Aura of Decay
// -----------------------------------------------------------------------------
int SpellAuraOfDecay::max_spi_cost(const SpellSkill skill) const
{
        (void)skill;

        return 6;
}

void SpellAuraOfDecay::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        const int dmg = 1 + (int)skill;

        Range duration_range;
        duration_range.min = 20 * ((int)skill + 1);
        duration_range.max = duration_range.min * 2;

        auto prop = new PropAuraOfDecay();

        prop->set_duration(duration_range.roll());

        prop->set_dmg(dmg);

        caster->properties.apply(prop);
}

std::vector<std::string> SpellAuraOfDecay::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back(
                "The caster exudes death and decay. Creatures within a "
                "distance of two moves take damage each standard turn.");

        const int dmg = 1 + (int)skill;

        descr.push_back(
                "The spell does " +
                std::to_string(dmg) +
                " damage per creature.");

        Range duration_range;
        duration_range.min = 20 * ((int)skill + 1);
        duration_range.max = duration_range.min * 2;

        descr.push_back("The spell lasts " + duration_range.str() + " turns.");

        return descr;
}

int SpellAuraOfDecay::mon_cooldown() const
{
        return 30;
}

bool SpellAuraOfDecay::allow_mon_cast_now(Mon& mon) const
{
        return
                mon.target_ &&
                mon.is_target_seen_ &&
                !mon.properties.has(PropId::aura_of_decay);
}

// -----------------------------------------------------------------------------
// Bolt spells
// -----------------------------------------------------------------------------
Range ForceBolt::damage(const SpellSkill skill,
                        const Actor& caster) const
{
        (void)caster;

        switch (skill)
        {
        case SpellSkill::basic:
                return Range(3, 4);

        case SpellSkill::expert:
                return Range(5, 7);

        case SpellSkill::master:
                return Range(9, 12);
        }

        ASSERT(false);

        return Range(1, 1);
}

std::vector<std::string> ForceBolt::descr_specific(const SpellSkill skill) const
{
        (void)skill;

        return {};
}

Range Darkbolt::damage(const SpellSkill skill, const Actor& caster) const
{
        Range dmg;

        switch (skill)
        {
        case SpellSkill::basic:
                dmg = Range(4, 9);
                break;

        case SpellSkill::expert:
                dmg = Range(7, 14);
                break;

        case SpellSkill::master:
                dmg = Range(10, 19);
                break;
        }

        if (!caster.is_player())
        {
                const int mon_dmg_pct = 75;

                dmg.min = (dmg.min * mon_dmg_pct) / 100;
                dmg.max = (dmg.max * mon_dmg_pct) / 100;
        }

        return dmg;
}

std::vector<std::string> Darkbolt::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back(
                "Siphons power from some infernal dimension, which is focused "
                "into a bolt hurled towards a target with great force. The "
                "conjured bolt has some will on its own - the caster cannot "
                "determine exactly which creature will be struck.");

        const auto dmg_range = damage(skill, *map::player);

        descr.push_back("The impact does " + dmg_range.str() + " damage.");

        if (skill == SpellSkill::master)
        {
                descr.push_back("The target is paralyzed, and set aflame.");
        }
        else // Not master
        {
                descr.push_back("The target is paralyzed.");
        }

        return descr;
}

void Darkbolt::on_hit(Actor& actor_hit, const SpellSkill skill) const
{
        if (!actor_hit.is_alive())
        {
                return;
        }

        auto prop = new PropParalyzed();

        prop->set_duration(rnd::range(1, 2));

        actor_hit.properties.apply(prop);

        if (skill == SpellSkill::master)
        {
                auto prop = new PropBurning();

                prop->set_duration(rnd::range(2, 3));

                actor_hit.properties.apply(prop);
        }
}

void SpellBolt::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        // if (caster == map::player &&
        //     player_bon::traits[(size_t)Trait::warlock] &&
        //     rnd::percent(warlock_multi_cast_chance_pct))
        // {
        //     run_effect(caster, skill);

        //     if (!caster->is_alive())
        //     {
        //         return;
        //     }
        // }

        std::vector<Actor*> target_bucket;

        target_bucket = caster->seen_foes();

        if (target_bucket.empty())
        {
                if (caster->is_player())
                {
                        msg_log::add(
                                "A dark sphere materializes, but quickly "
                                "fizzles out.");
                }

                return;
        }

        Actor* const target =
                map::random_closest_actor(
                        caster->pos,
                        target_bucket);

        // Spell resistance?
        if (target->properties.has(PropId::r_spell))
        {
                on_resist(*target);

                // Spell reflection?
                if (target->properties.has(PropId::spell_reflect))
                {
                        if (map::player->can_see_actor(*target))
                        {
                                msg_log::add(
                                        spell_reflect_msg,
                                        colors::text(),
                                        false,
                                        MorePromptOnMsg::yes);
                        }

                        // Run effect with the target as caster instead
                        run_effect(target, skill);
                }

                return;
        }

        // Draw projectile traveling
        {
                Array2<bool> blocked(map::dims());

                map_parsers::BlocksProjectiles()
                        .run(blocked, blocked.rect());

                const auto flood = floodfill(caster->pos, blocked);

                const auto path =
                        pathfind_with_flood(
                                caster->pos,
                                target->pos,
                                flood);

                if (!path.empty())
                {
                        states::draw();

                        const int idx_0 = (int)(path.size()) - 1;

                        for (int i = idx_0; i > 0; --i)
                        {
                                const P& p = path[i];

                                if (!map::cells.at(p).is_seen_by_player ||
                                    !viewport::is_in_view(p))
                                {
                                        continue;
                                }

                                states::draw();

                                io::draw_symbol(
                                        TileId::blast1,
                                        '*',
                                        Panel::map,
                                        viewport::to_view_pos(p),
                                        colors::magenta());

                                io::update_screen();

                                sdl_base::sleep(
                                        config::delay_projectile_draw());
                        }
                }
        }

        const P& target_p = target->pos;

        const bool player_see_cell =
                map::cells.at(target_p).is_seen_by_player;

        const bool player_see_tgt = map::player->can_see_actor(*target);

        if (player_see_tgt || player_see_cell)
        {
                io::draw_blast_at_cells({target->pos}, colors::magenta());

                Color msg_clr = colors::msg_good();

                std::string str_begin = "I am";

                if (target->is_player())
                {
                        msg_clr = colors::msg_bad();
                }
                else // Target is monster
                {
                        const std::string name_the =
                                player_see_tgt
                                ? text_format::first_to_upper(
                                        target->name_the())
                                : "It";

                        str_begin = name_the + " is";

                        if (map::player->is_leader_of(target))
                        {
                                msg_clr = colors::white();
                        }
                }

                msg_log::add(
                        str_begin +
                        " " +
                        impl_->hit_msg_ending(),
                        msg_clr);
        }

        const auto dmg_range = impl_->damage(skill, *caster);

        actor::hit(
                *target,
                dmg_range.roll(),
                DmgType::physical,
                DmgMethod::END,
                AllowWound::no);

        impl_->on_hit(*target, skill);

        Snd snd("",
                SfxId::END,
                IgnoreMsgIfOriginSeen::yes,
                target->pos,
                nullptr,
                SndVol::low,
                AlertsMon::yes);

        snd_emit::run(snd);
}

bool SpellBolt::allow_mon_cast_now(Mon& mon) const
{
        // NOTE: Monsters with master spell skill level COULD cast this spell
        // without LOS to the player, but we do not allow the AI to do this,
        // since it would probably be very hard or annoying for the player to
        // deal with
        return mon.target_ && mon.is_target_seen_;
}

// -----------------------------------------------------------------------------
// Azathoths wrath
// -----------------------------------------------------------------------------
void SpellAzaWrath::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        // if (caster == map::player &&
        //     player_bon::traits[(size_t)Trait::warlock] &&
        //     rnd::percent(warlock_multi_cast_chance_pct))
        // {
        //     run_effect(caster, skill);

        //     if (!caster->is_alive())
        //     {
        //         return;
        //     }
        // }

        const auto targets = caster->seen_foes();

        if (targets.empty())
        {
                if (caster->is_player())
                {
                        msg_log::add(
                                "There is a faint rumbling sound, like "
                                "distant thunder.");
                }
        }

        io::draw_blast_at_seen_actors(targets, colors::light_red());

        for (Actor* const target : targets)
        {
                // Spell resistance?
                if (target->properties.has(PropId::r_spell))
                {
                        on_resist(*target);

                        // Spell reflection?
                        if (target->properties.has(PropId::spell_reflect))
                        {
                                if (map::player->can_see_actor(*target))
                                {
                                        msg_log::add(
                                                spell_reflect_msg,
                                                colors::white(),
                                                false,
                                                MorePromptOnMsg::yes);
                                }

                                // Run effect with the target as caster
                                run_effect(target, skill);
                        }

                        continue;
                }

                std::string str_begin = "I am";

                Color msg_clr = colors::msg_good();

                if (target->is_player())
                {
                        msg_clr = colors::msg_bad();
                }
                else // Target is monster
                {
                        str_begin =
                                text_format::first_to_upper(
                                        target->name_the()) + " is";

                        if (map::player->is_leader_of(target))
                        {
                                msg_clr = colors::white();
                        }
                }

                if (map::player->can_see_actor(*target))
                {
                        msg_log::add(
                                str_begin + " struck by a roaring blast!",
                                msg_clr);
                }

                // Skill  damage     avg
                // -----------------------
                // 0      2  - 5     3.5
                // 1      4  - 8     6.0
                // 2      6  - 11    8.5
                Range dmg_range(
                        2 + (int)skill * 2,
                        5 + (int)skill * 3);

                actor::hit(
                        *target,
                        dmg_range.roll(),
                        DmgType::physical,
                        DmgMethod::END,
                        AllowWound::no);

                if (target->is_alive())
                {
                        auto prop = new PropParalyzed();

                        prop->set_duration(1);

                        target->properties.apply(prop);
                }

                if ((skill == SpellSkill::master) &&
                    target->is_alive())
                {
                        auto prop = new PropBurning();

                        prop->set_duration(2);

                        target->properties.apply(prop);
                }

                Snd snd("",
                        SfxId::END,
                        IgnoreMsgIfOriginSeen::yes,
                        target->pos,
                        nullptr,
                        SndVol::high, AlertsMon::yes);

                snd_emit::run(snd);
        }
}

std::vector<std::string> SpellAzaWrath::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back(
                "Channels the destructive force of Azathoth unto all "
                "visible enemies.");

        Range dmg_range(
                2 + (int)skill * 2,
                5 + (int)skill * 3);

        descr.push_back(
                "The spell does " +
                dmg_range.str() +
                " damage per creature.");

        if (skill == SpellSkill::master)
        {
                descr.push_back("The target is paralyzed, and set aflame.");
        }
        else // Not master
        {
                descr.push_back("The target is paralyzed.");
        }

        return descr;
}

bool SpellAzaWrath::allow_mon_cast_now(Mon& mon) const
{
        return mon.target_ && mon.is_target_seen_;
}

// -----------------------------------------------------------------------------
// Mayhem
// -----------------------------------------------------------------------------
int SpellMayhem::max_spi_cost(const SpellSkill skill) const
{
        (void)skill;

        return 6;
}

void SpellMayhem::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        // if (caster == map::player &&
        //     player_bon::traits[(size_t)Trait::warlock] &&
        //     rnd::percent(warlock_multi_cast_chance_pct))
        // {
        //     run_effect(caster, skill);

        //     if (!caster->is_alive())
        //     {
        //         return;
        //     }
        // }

        const bool is_player = caster->is_player();

        if (map::player->can_see_actor(*caster))
        {
                std::string caster_name =
                        is_player ?
                        "me" :
                        caster->name_the();

                msg_log::add("Destruction rages around " + caster_name + "!");
        }

        const P& caster_pos = caster->pos;

        const int destr_radi = fov_radi_int + (int)skill * 2;

        const int x0 = std::max(
                1,
                caster_pos.x - destr_radi);

        const int y0 = std::max(
                1,
                caster_pos.y - destr_radi);

        const int x1 = std::min(
                map::w() - 1,
                caster_pos.x + destr_radi) - 1;

        const int y1 = std::min(
                map::h() - 1,
                caster_pos.y + destr_radi) - 1;

        // Run explosions
        std::vector<P> p_bucket;

        const int expl_radi_diff = -1;

        for (int x = x0; x <= x1; ++x)
        {
                for (int y = y0; y <= y1; ++y)
                {
                        const Rigid* const f = map::cells.at(x, y).rigid;

                        if (!f->is_walkable())
                        {
                                continue;
                        }

                        const P p(x, y);

                        const int dist = king_dist(caster_pos, p);

                        const int min_dist = expl_std_radi + 1 + expl_radi_diff;

                        if (dist >= min_dist)
                        {
                                p_bucket.push_back(p);
                        }
                }
        }

        int nr_expl = 2 + (int)skill;

        for (int i = 0; i < nr_expl; ++i)
        {
                if (p_bucket.empty())
                {
                        return;
                }

                const size_t idx = rnd::range(0, p_bucket.size() - 1);

                const P& p = rnd::element(p_bucket);

                explosion::run(p,
                               ExplType::expl,
                               EmitExplSnd::yes,
                               expl_radi_diff);

                p_bucket.erase(p_bucket.begin() + idx);
        }

        // Explode braziers
        for (int x = x0; x <= x1; ++x)
        {
                for (int y = y0; y <= y1; ++y)
                {
                        const auto id = map::cells.at(x, y).rigid->id();

                        if (id == FeatureId::brazier)
                        {
                                const P p(x, y);

                                Snd snd("I hear an explosion!",
                                        SfxId::explosion_molotov,
                                        IgnoreMsgIfOriginSeen::yes,
                                        p,
                                        nullptr,
                                        SndVol::high,
                                        AlertsMon::yes);

                                snd.run();

                                map::put(new RubbleLow(p));

                                explosion::run(
                                        p,
                                        ExplType::apply_prop,
                                        EmitExplSnd::yes,
                                        0,
                                        ExplExclCenter::yes,
                                        {new PropBurning()});
                        }
                }
        }

        // Destroy the surrounding environment
        const int nr_sweeps = 2 + (int)skill;

        for (int i = 0; i < nr_sweeps; ++i)
        {
                for (int x = x0; x <= x1; ++x)
                {
                        for (int y = y0; y <= y1; ++y)
                        {
                                if (!rnd::one_in(8))
                                {
                                        continue;
                                }

                                bool is_adj_to_walkable_cell = false;

                                for (const P& d : dir_utils::dir_list)
                                {
                                        const P p_adj(P(x, y) + d);

                                        const auto& cell = map::cells.at(p_adj);

                                        if (cell.rigid->is_walkable())
                                        {
                                                is_adj_to_walkable_cell = true;
                                        }
                                }

                                if (is_adj_to_walkable_cell)
                                {
                                        map::cells.at(x, y).rigid->hit(
                                                1, // Damage (doesn't matter)
                                                DmgType::physical,
                                                DmgMethod::explosion,
                                                nullptr);
                                }
                        }
                }
        }

        // Put blood, and set stuff on fire
        for (int x = x0; x <= x1; ++x)
        {
                for (int y = y0; y <= y1; ++y)
                {
                        auto* const f = map::cells.at(x, y).rigid;

                        if (f->can_have_blood() &&
                            rnd::one_in(10))
                        {
                                f->make_bloody();

                                if (rnd::one_in(3))
                                {
                                        f->try_put_gore();
                                }
                        }

                        if ((P(x, y) != caster->pos) &&
                            rnd::one_in(6))
                        {
                                f->hit(1, // Damage (doesn't matter)
                                       DmgType::fire,
                                       DmgMethod::elemental,
                                       nullptr);
                        }
                }
        }

        Snd snd("",
                SfxId::END,
                IgnoreMsgIfOriginSeen::yes,
                caster_pos,
                nullptr,
                SndVol::high,
                AlertsMon::yes);

        snd_emit::run(snd);
}

std::vector<std::string> SpellMayhem::descr_specific(
        const SpellSkill skill) const
{
        (void)skill;

        std::vector<std::string> descr;

        descr.push_back("Blasts the surrounding area with terrible force.");

        descr.push_back("Higher skill levels increases the magnitude of the "
                        "destruction.");

        return descr;
}

bool SpellMayhem::allow_mon_cast_now(Mon& mon) const
{
        return
                mon.target_ &&
                (mon.is_target_seen_ || rnd::one_in(20));
}

// -----------------------------------------------------------------------------
// Pestilence
// -----------------------------------------------------------------------------
void SpellPestilence::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        const size_t nr_mon = 6 + (int)skill * 6;

        Actor* leader = nullptr;

        if (caster->is_player())
        {
                leader = caster;
        }
        else // Caster is monster
        {
                Actor* const caster_leader = static_cast<Mon*>(caster)->leader_;

                leader =
                        caster_leader ?
                        caster_leader :
                        caster;
        }

        bool is_any_seen_by_player = false;

        const auto mon_summoned =
                actor_factory::spawn(
                        caster->pos,
                        {nr_mon, ActorId::rat},
                        map::rect())
                .make_aware_of_player()
                .set_leader(leader)
                .for_each([skill, &is_any_seen_by_player](Mon* const mon)
                {
                        mon->properties.apply(new PropSummoned());

                        auto prop_waiting = new PropWaiting();

                        prop_waiting->set_duration(2);

                        mon->properties.apply(prop_waiting);

                        if (map::player->can_see_actor(*mon))
                        {
                                is_any_seen_by_player = true;
                        }

                        // Haste the rats if master
                        if (skill == SpellSkill::master)
                        {
                                auto prop_hasted = new PropHasted();

                                prop_hasted->set_indefinite();

                                mon->properties.apply(
                                        prop_hasted,
                                        PropSrc::intr,
                                        true,
                                        Verbosity::silent);
                        }
                });

        if (mon_summoned.monsters.empty())
        {
                return;
        }

        if (caster->is_player() ||
            is_any_seen_by_player)
        {
                std::string caster_str = "me";

                if (!caster->is_player())
                {
                        if (map::player->can_see_actor(*caster))
                        {
                                caster_str = caster->name_the();
                        }
                        else
                        {
                                caster_str = "it";
                        }
                }

                msg_log::add("Rats appear around " + caster_str + "!");
        }
}

std::vector<std::string> SpellPestilence::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back("A pack of rats appear around the caster.");

        const size_t nr_mon = 6 + (int)skill * 6;

        descr.push_back("Summons " + std::to_string(nr_mon) + " rats.");

        if (skill == SpellSkill::master)
        {
                descr.push_back("The rats are Hasted (+100% speed).");
        }

        return descr;
}

bool SpellPestilence::allow_mon_cast_now(Mon& mon) const
{
        return
                mon.target_ &&
                (mon.is_target_seen_ || rnd::one_in(30));
}

// -----------------------------------------------------------------------------
// Animate weapons
// -----------------------------------------------------------------------------
void SpellSpectralWpns::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        TRACE_FUNC_BEGIN;

        if (!caster->is_player())
        {
                TRACE_FUNC_END;

                return;
        }

        auto is_melee_wpn = [](const Item* const item) {
                return item && item->data().type == ItemType::melee_wpn;
        };

        std::vector<const Item*> weapons;

        for (const auto& slot : caster->inv.slots)
        {
                if (is_melee_wpn(slot.item))
                {
                        weapons.push_back(slot.item);
                }
        }

        for (const auto& item : caster->inv.backpack)
        {
                if (is_melee_wpn(item))
                {
                        weapons.push_back(item);
                }
        }

        for (const Item* const item : weapons)
        {
                Item* new_item = item_factory::make(item->id());

                new_item->melee_base_dmg_ = new_item->melee_base_dmg_;

                auto spectral_wpn_init = [new_item, skill](Mon* const mon) {

                        ASSERT(!mon->inv.item_in_slot(SlotId::wpn));

                        mon->inv.put_in_slot(
                                SlotId::wpn,
                                new_item,
                                Verbosity::silent);

                        mon->properties.apply(new PropSummoned());

                        auto prop_waiting = new PropWaiting();

                        prop_waiting->set_duration(1);

                        mon->properties.apply(prop_waiting);

                        if (skill >= SpellSkill::expert)
                        {
                                auto prop = new PropSeeInvis();

                                prop->set_indefinite();

                                mon->properties.apply(
                                        prop,
                                        PropSrc::intr,
                                        true,
                                        Verbosity::silent);
                        }

                        if (skill == SpellSkill::master)
                        {
                                auto prop = new PropHasted();

                                prop->set_indefinite();

                                mon->properties.apply(
                                        prop,
                                        PropSrc::intr,
                                        true,
                                        Verbosity::silent);
                        }

                        if (map::player->can_see_actor(*mon))
                        {
                                msg_log::add(mon->name_a() + " appears!");
                        }
                };

                const auto summoned =
                        actor_factory::spawn(
                                caster->pos,
                                {ActorId::spectral_wpn},
                                map::rect())
                        .set_leader(caster)
                        .for_each(spectral_wpn_init);
        }

        TRACE_FUNC_END;
}

std::vector<std::string> SpellSpectralWpns::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back(
                "Conjures ghostly copies of all carried weapons, which will "
                "float through the air and protect their master. The weapons "
                "almost always hit their target, but they quickly "
                "dematerialize when damaged. It is only possible to create "
                "copies of basic melee weapons - \"modern\" mechanisms such as "
                "pistols or machine guns are far too complex.");

        if (skill >= SpellSkill::expert)
        {
                descr.push_back("The weapons can see invisible creatures.");
        }

        if (skill == SpellSkill::master)
        {
                descr.push_back("The weapons are hasted.");
        }

        return descr;
}

// -----------------------------------------------------------------------------
// Pharaoh staff
// -----------------------------------------------------------------------------
void SpellPharaohStaff::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        (void)skill;

        // First try to heal a friendly mummy (as per the spell description)
        for (Actor* const actor : game_time::actors)
        {
                const auto actor_id = actor->data->id;

                const bool is_actor_id_ok = actor_id == ActorId::mummy ||
                        actor_id == ActorId::croc_head_mummy;

                if (is_actor_id_ok && caster->is_leader_of(actor))
                {
                        actor->restore_hp(999);

                        return;
                }
        }

        // This point reached means no mummy controlled, summon a new one
        Actor* leader = nullptr;

        if (caster->is_player())
        {
                leader = caster;
        }
        else // Caster is monster
        {
                Actor* const caster_leader = static_cast<Mon*>(caster)->leader_;

                leader =
                        caster_leader ?
                        caster_leader :
                        caster;
        }

        const auto actor_id =
                rnd::coin_toss() ?
                ActorId::mummy :
                ActorId::croc_head_mummy;

        const auto summoned =
                actor_factory::spawn(
                        caster->pos,
                        {actor_id},
                        map::rect())
                .make_aware_of_player()
                .set_leader(leader)
                .for_each([](Mon* const mon)
                {
                        mon->properties.apply(new PropSummoned());

                        auto prop_waiting = new PropWaiting();

                        prop_waiting->set_duration(2);

                        mon->properties.apply(prop_waiting);

                        if (map::player->can_see_actor(*mon))
                        {
                                msg_log::add(
                                        text_format::first_to_upper(
                                                mon->name_a()) +
                                        " appears!");
                        }
                        else // Player cannot see monster
                        {
                                msg_log::add("I sense a new presence.");
                        }
                });
}

std::vector<std::string> SpellPharaohStaff::descr_specific(
        const SpellSkill skill) const
{
        (void)skill;

        const std::vector<std::string> descr = {
                "Summons a loyal Mummy servant which will fight for the "
                "caster.",

                "If an allied Mummy is already present, this spell will "
                "instead heal it."
        };

        return descr;
}

bool SpellPharaohStaff::allow_mon_cast_now(Mon& mon) const
{
        return mon.target_;
}

// -----------------------------------------------------------------------------
// Searching
// -----------------------------------------------------------------------------
int SpellSearching::max_spi_cost(const SpellSkill skill) const
{
        (void)skill;

        return 4;
}

void SpellSearching::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        const int range = 4 + (int)skill * 2;

        const int nr_turns = ((int)skill + 1) * 40;

        auto prop = new PropMagicSearching();

        prop->set_range(range);

        prop->set_duration(nr_turns);

        if (skill >= SpellSkill::expert)
        {
                prop->set_allow_reveal_items();
        }

        caster->properties.apply(prop);
}

std::vector<std::string> SpellSearching::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back(
                "Reveals the presence of creatures, doors, traps, stairs, and "
                "other locations of interest in the surrounding area.");

        if (skill >= SpellSkill::expert)
        {
                descr.push_back("Also reveals items.");
        }

        const int range = 4 + (int)skill * 2;

        descr.push_back(
                "The spell has a range of " +
                std::to_string(range) +
                " cells.");

        const int nr_turns = ((int)skill + 1) * 40;

        descr.push_back(
                "The spell lasts " +
                std::to_string(nr_turns) +
                " turns.");

        return descr;
}

// -----------------------------------------------------------------------------
// Opening
// -----------------------------------------------------------------------------
void SpellOpening::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        (void)caster;

        const int range = 1 + (int)skill * 3;

        const int orig_x = map::player->pos.x;
        const int orig_y = map::player->pos.y;

        const int x0 = std::max(
                0,
                orig_x - range);

        const int y0 = std::max(
                0,
                orig_y - range);

        const int x1 = std::min(
                map::w() - 1,
                orig_x + range);

        const int y1 = std::min(
                map::h() - 1,
                orig_y + range);

        bool is_any_opened = false;

        const int chance_to_open = 50 + (int)skill * 25;

        // TODO: Way too much nested scope here!

        for (int x = x0; x <= x1; ++x)
        {
                for (int y = y0; y <= y1; ++y)
                {
                        if (!rnd::percent(chance_to_open))
                        {
                                continue;
                        }

                        const auto& cell = map::cells.at(x, y);

                        auto* const f = cell.rigid;

                        auto did_open = DidOpen::no;

                        bool is_metal_door = false;

                        // Is this a metal door?
                        if (f->id() == FeatureId::door)
                        {
                                auto* const door = static_cast<Door*>(f);

                                is_metal_door =
                                        (door->type() == DoorType::metal);

                                // If at least expert skill, then metal doors
                                // are also opened
                                if (is_metal_door &&
                                    !door->is_open() &&
                                    ((int)skill >= (int)SpellSkill::expert))
                                {
                                        for (int x_lever = 0;
                                             x_lever < map::w();
                                             ++x_lever)
                                        {
                                                for (int y_lever = 0;
                                                     y_lever < map::h();
                                                     ++y_lever)
                                                {
                                                        auto* const f_lever = map::cells.at(x_lever, y_lever).rigid;

                                                        if (f_lever->id() !=
                                                            FeatureId::lever)
                                                        {
                                                                continue;
                                                        }

                                                        auto* const lever = static_cast<Lever*>(f_lever);

                                                        if (lever->is_linked_to(*f))
                                                        {
                                                                lever->toggle();

                                                                did_open = DidOpen::yes;

                                                                break;
                                                        }
                                                } // Lever y loop

                                                if (did_open == DidOpen::yes)
                                                {
                                                        break;
                                                }
                                        } // Lever x loop
                                }
                        }

                        if ((did_open != DidOpen::yes) &&
                            !is_metal_door)
                        {
                                did_open = cell.rigid->open(nullptr);
                        }

                        if (did_open == DidOpen::yes)
                        {
                                is_any_opened = true;
                        }
                }
        }

        if (is_any_opened)
        {
                map::update_vision();
        }
        else // Nothing was opened
        {
                msg_log::add("I hear faint rattling and knocking.");
        }
}

std::vector<std::string> SpellOpening::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        std::string str = "Opens all locks, lids and doors";

        if (skill == SpellSkill::basic)
        {
                str += " (except heavy doors operated externally by a switch)";
        }

        str += ".";

        descr.push_back(str);

        if ((int)skill >= (int)SpellSkill::expert)
        {
                descr.push_back(
                        "If cast within range of a door operated by a lever, "
                        "then the lever is toggled.");
        }

        const int range = 1 + (int)skill * 3;

        if (range == 1)
        {
                descr.push_back("Only adjacent objects are opened.");
        }
        else // Range > 1
        {
                descr.push_back(
                        "Opens objects within a distance of " +
                        std::to_string(range) +
                        " cells.");
        }

        const int chance_to_open = 50 + (int)skill * 25;

        descr.push_back(std::to_string(chance_to_open) + "% chance to open.");

        return descr;
}

bool SpellOpening::is_noisy(const SpellSkill skill) const
{
        return (skill == SpellSkill::basic);
}

// -----------------------------------------------------------------------------
// Ghoul frenzy
// -----------------------------------------------------------------------------
void SpellFrenzy::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        (void)skill;

        auto prop = new PropFrenzied();

        prop->set_duration(rnd::range(30, 40));

        caster->properties.apply(prop);
}

std::vector<std::string> SpellFrenzy::descr_specific(
        const SpellSkill skill) const
{
        (void)skill;

        return
        {
                "Incites a great rage in the caster, which will charge their "
                "enemies with a terrible, uncontrollable fury."
        };
}

// -----------------------------------------------------------------------------
// Bless
// -----------------------------------------------------------------------------
void SpellBless::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        auto prop = new PropBlessed();

        prop->set_duration(20 + (int)skill * 100);

        caster->properties.apply(prop);
}

std::vector<std::string> SpellBless::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back("Bends reality in favor of the caster.");

        const int nr_turns = 20 + (int)skill * 60;

        descr.push_back(
                "The spell lasts " +
                std::to_string(nr_turns) +
                " turns.");

        return descr;
}

// -----------------------------------------------------------------------------
// Light
// -----------------------------------------------------------------------------
void SpellLight::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        auto prop = new PropRadiant();

        prop->set_duration(20 + (int)skill * 20);

        caster->properties.apply(prop);

        if (skill == SpellSkill::master)
        {
                explosion::run(caster->pos,
                               ExplType::apply_prop,
                               EmitExplSnd::no,
                               0,
                               ExplExclCenter::yes,
                               {new PropBlind()},
                               colors::yellow());
        }
}

std::vector<std::string> SpellLight::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back("Illuminates the area around the caster.");

        const int nr_turns = 20 + (int)skill * 20;

        descr.push_back(
                "The spell lasts " +
                std::to_string(nr_turns) +
                " turns.");

        if (skill == SpellSkill::master)
        {
                descr.push_back(
                        "On casting, causes a blinding flash centered on the "
                        "caster (but not affecting the caster itself).");
        }

        return descr;
}

// -----------------------------------------------------------------------------
// See Invisible
// -----------------------------------------------------------------------------
void SpellSeeInvis::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        const Range duration_range =
                (skill == SpellSkill::basic)
                ? Range(5, 8)
                : (skill == SpellSkill::expert)
                ? Range(40, 80)
                : Range(400, 600);

        auto prop = new PropSeeInvis();

        prop->set_duration(duration_range.roll());

        caster->properties.apply(prop);
}

std::vector<std::string> SpellSeeInvis::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back(
                "Grants the caster the ability to see that which is "
                "normally invisible.");

        const Range duration_range =
                (skill == SpellSkill::basic)
                ? Range(5, 8)
                : (skill == SpellSkill::expert)
                ? Range(40, 80)
                : Range(400, 600);

        descr.push_back("The spell lasts " + duration_range.str() + " turns.");

        return descr;
}

bool SpellSeeInvis::allow_mon_cast_now(Mon& mon) const
{
        return
                !mon.properties.has(PropId::see_invis) &&
                (mon.aware_of_player_counter_ > 0) &&
                rnd::one_in(8);
}

// -----------------------------------------------------------------------------
// Spell Shield
// -----------------------------------------------------------------------------
int SpellSpellShield::max_spi_cost(const SpellSkill skill) const
{
        return 5 - (int)skill;
}

void SpellSpellShield::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        (void)skill;

        auto prop = new PropRSpell();

        prop->set_indefinite();

        caster->properties.apply(prop);
}

std::vector<std::string> SpellSpellShield::descr_specific(
        const SpellSkill skill) const
{
        (void)skill;

        std::vector<std::string> descr;

        descr.push_back(
                "Grants protection against harmful spells. The effect lasts "
                "until a spell is blocked.");

        descr.push_back(
                "Skill level affects the amount of Spirit one needs to spend "
                "to cast the spell.");

        return descr;
}

bool SpellSpellShield::allow_mon_cast_now(Mon& mon) const
{
        return !mon.properties.has(PropId::r_spell);
}

// -----------------------------------------------------------------------------
// Slow Time
// -----------------------------------------------------------------------------
int SpellSlowTime::max_spi_cost(const SpellSkill skill) const
{
        (void)skill;

        return 5;
}

void SpellSlowTime::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        Range duration_range;
        duration_range.min = 5 * ((int)skill + 1);
        duration_range.max = duration_range.min * 2;

        const int duration = duration_range.roll();

        auto prop = new PropHasted();

        prop->set_duration(duration);

        caster->properties.apply(prop);
}

std::vector<std::string> SpellSlowTime::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back("Time slows down from the caster's perspective.");

        Range duration_range;
        duration_range.min = 5 * ((int)skill + 1);
        duration_range.max = duration_range.min * 2;

        descr.push_back("The spell lasts " + duration_range.str() + " turns.");

        return descr;
}

int SpellSlowTime::mon_cooldown() const
{
        return 20;
}

bool SpellSlowTime::allow_mon_cast_now(Mon& mon) const
{
        return
                mon.target_ &&
                mon.is_target_seen_ &&
                !mon.properties.has(PropId::hasted);
}

// -----------------------------------------------------------------------------
// Divert Attacks
// -----------------------------------------------------------------------------
int SpellDivertAttacks::max_spi_cost(const SpellSkill skill) const
{
        (void)skill;

        return 7;
}

void SpellDivertAttacks::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        Range duration_range;
        duration_range.min = 4 + (int)skill * 4;
        duration_range.max = duration_range.min * 2;

        auto prop = new PropDivertAttacks();

        prop->set_duration(duration_range.roll());

        caster->properties.apply(prop);
}

std::vector<std::string> SpellDivertAttacks::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back(
                "Melee and ranged attacks made against the caster are slightly "
                "diverted away, making it extremely difficult to achieve a "
                "successful hit.");

        Range duration_range;
        duration_range.min = 4 + (int)skill * 4;
        duration_range.max = duration_range.min * 2;

        descr.push_back(
                "The spell lasts " +
                duration_range.str() +
                " turns.");

        return descr;
}

// -----------------------------------------------------------------------------
// Identify
// -----------------------------------------------------------------------------
int SpellIdentify::max_spi_cost(const SpellSkill skill) const
{
        (void)skill;

        return 8;
}

void SpellIdentify::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
    (void)caster;
    (void)skill;

    std::vector<ItemType> item_types_allowed;

    if (skill != SpellSkill::master)
    {
            item_types_allowed.push_back(ItemType::scroll);

            if (skill == SpellSkill::expert)
            {
                    item_types_allowed.push_back(ItemType::potion);
            }
    }

    // Run identify selection menu
    states::push(std::make_unique<SelectIdentify>(item_types_allowed));

    msg_log::more_prompt();
}

std::vector<std::string> SpellIdentify::descr_specific(
        const SpellSkill skill) const
{
        std::string descr = "Identifies ";

        switch (skill)
        {
        case SpellSkill::basic:
                descr += "Manuscripts";
                break;

        case SpellSkill::expert:
                descr += "Manuscripts and Potions";
                break;

        case SpellSkill::master:
                descr += "all items";
                break;
        }

        descr += ".";

        return {descr};
}

// -----------------------------------------------------------------------------
// Teleport
// -----------------------------------------------------------------------------
void SpellTeleport::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        if ((int)skill >= (int)SpellSkill::expert)
        {
                auto prop = new PropInvisible();

                prop->set_duration(3);

                caster->properties.apply(prop);
        }

        auto should_ctrl = ShouldCtrlTele::if_tele_ctrl_prop;

        if ((skill == SpellSkill::master) &&
            (caster == map::player))
        {
                should_ctrl = ShouldCtrlTele::always;
        }

        teleport(*caster, should_ctrl);
}

bool SpellTeleport::allow_mon_cast_now(Mon& mon) const
{
        const bool is_low_hp = mon.hp <= (actor::max_hp(mon) / 2);

        return
                (mon.aware_of_player_counter_ > 0) &&
                is_low_hp &&
                rnd::fraction(3, 4);
}

std::vector<std::string> SpellTeleport::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back(
                "Instantly moves the caster to a different position.");

        if ((int)skill >= (int)SpellSkill::expert)
        {
                const int nr_turns = 3;

                descr.push_back(
                        "On teleporting, the caster is invisible for " +
                        std::to_string(nr_turns) +
                        " turns.");
        }

        if (skill == SpellSkill::master)
        {
                descr.push_back(
                        "The caster can control the teleport destination.");
        }

        return descr;
}

// -----------------------------------------------------------------------------
// Resistance
// -----------------------------------------------------------------------------
void SpellRes::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        int nr_turns = 15 + (int)skill * 35;

        auto prop_r_fire = new PropRFire;
        auto prop_r_elec = new PropRElec;

        prop_r_fire->set_duration(nr_turns);
        prop_r_elec->set_duration(nr_turns);

        caster->properties.apply(prop_r_fire);
        caster->properties.apply(prop_r_elec);
}

std::vector<std::string> SpellRes::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back(
                "The caster is completely shielded from fire and electricity.");

        const int nr_turns = 15 + (int)skill * 35;

        descr.push_back(
                "The spell lasts " +
                std::to_string(nr_turns) +
                " turns.");

        return descr;
}

bool SpellRes::allow_mon_cast_now(Mon& mon) const
{
        const bool has_rfire = mon.properties.has(PropId::r_fire);
        const bool has_relec = mon.properties.has(PropId::r_elec);

        return
                (!has_rfire || !has_relec) &&
                mon.target_;
}

// -----------------------------------------------------------------------------
// Knockback
// -----------------------------------------------------------------------------
void SpellKnockBack::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        (void)skill;

        ASSERT(!caster->is_player());

        Color msg_clr = colors::msg_good();

        std::string target_str = "me";

        Actor* caster_used = caster;

        auto* const mon = static_cast<Mon*>(caster_used);

        Actor* target = mon->target_;

        ASSERT(target);

        ASSERT(mon->is_target_seen_);

        // Spell resistance?
        if (target->properties.has(PropId::r_spell))
        {
                on_resist(*target);

                // Spell reflection?
                if (target->properties.has(PropId::spell_reflect))
                {
                        if (map::player->can_see_actor(*target))
                        {
                                msg_log::add(
                                        spell_reflect_msg,
                                        colors::text(),
                                        false,
                                        MorePromptOnMsg::yes);
                        }

                        std::swap(caster_used, target);
                }
                else // No spell reflection, just abort
                {
                        return;
                }
        }

        if (target->is_player())
        {
                msg_clr = colors::msg_bad();
        }
        else // Target is monster
        {
                target_str = target->name_the();

                if (map::player->is_leader_of(target))
                {
                        msg_clr = colors::white();
                }
        }

        if (map::player->can_see_actor(*target))
        {
                msg_log::add("A force pushes " + target_str + "!", msg_clr);
        }

        knockback::run(*target, caster->pos, false);
}

bool SpellKnockBack::allow_mon_cast_now(Mon& mon) const
{
        return mon.target_ && mon.is_target_seen_;
}

// -----------------------------------------------------------------------------
// Enfeeble
// -----------------------------------------------------------------------------
int SpellEnfeeble::max_spi_cost(const SpellSkill skill) const
{
        (void)skill;

        return 3;
}

int SpellEnfeeble::mon_cooldown() const
{
        return 5;
}

void SpellEnfeeble::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        Range duration_range;
        duration_range.min = 15 * ((int)skill + 1);
        duration_range.max = duration_range.min * 2;

        const int duration = duration_range.roll();

        auto targets = caster->seen_foes();

        if (targets.empty())
        {
                msg_log::add(
                        "The bugs on the ground suddenly move very feebly.");

                return;
        }

        // There are targets available

        if (skill == SpellSkill::basic)
        {
                auto* const target = rnd::element(targets);

                targets.clear();

                targets.push_back(target);
        }

        io::draw_blast_at_seen_actors(targets, colors::magenta());

        for (Actor* const target : targets)
        {
                // Spell resistance?
                if (target->properties.has(PropId::r_spell))
                {
                        on_resist(*target);

                        // Spell reflection?
                        if (target->properties.has(PropId::spell_reflect))
                        {
                                if (map::player->can_see_actor(*target))
                                {
                                        msg_log::add(
                                                spell_reflect_msg,
                                                colors::text(),
                                                false,
                                                MorePromptOnMsg::yes);
                                }

                                // Run effect with target as caster
                                run_effect(target, skill);
                        }

                        continue;
                }

                auto* const prop = new PropWeakened();

                prop->set_duration(duration);

                target->properties.apply(prop);
        }
}

std::vector<std::string> SpellEnfeeble::descr_specific(
        const SpellSkill skill) const
{
        (void)skill;

        std::vector<std::string> descr;

        descr.push_back(
                "Physically enfeebles the spell's victims, causing them to "
                "only do half damage in melee combat.");

        if (skill == SpellSkill::basic)
        {
                descr.push_back("Affects one random visible hostile creature.");
        }
        else
        {
                descr.push_back("Affects all visible hostile creatures.");
        }

        Range duration_range;
        duration_range.min = 15 * ((int)skill + 1);
        duration_range.max = duration_range.min * 2;

        descr.push_back(
                "The spell lasts " +
                duration_range.str() +
                " turns.");

        return descr;
}

bool SpellEnfeeble::allow_mon_cast_now(Mon& mon) const
{
        return mon.target_ && mon.is_target_seen_;
}

// -----------------------------------------------------------------------------
// Slow
// -----------------------------------------------------------------------------
int SpellSlow::max_spi_cost(const SpellSkill skill) const
{
        (void)skill;

        return 4;
}

void SpellSlow::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        Range duration_range;
        duration_range.min = 10 * ((int)skill + 1);
        duration_range.max = duration_range.min * 2;

        const int duration = duration_range.roll();

        auto targets = caster->seen_foes();

        if (targets.empty())
        {
                msg_log::add(
                        "The bugs on the ground suddenly move very slowly.");

                return;
        }

        // There are targets available

        if (skill == SpellSkill::basic)
        {
                auto* const target = rnd::element(targets);

                targets.clear();

                targets.push_back(target);
        }

        io::draw_blast_at_seen_actors(targets, colors::magenta());

        for (Actor* const target : targets)
        {
                // Spell resistance?
                if (target->properties.has(PropId::r_spell))
                {
                        on_resist(*target);

                        // Spell reflection?
                        if (target->properties.has(PropId::spell_reflect))
                        {
                                if (map::player->can_see_actor(*target))
                                {
                                        msg_log::add(
                                                spell_reflect_msg,
                                                colors::text(),
                                                false,
                                                MorePromptOnMsg::yes);
                                }

                                // Run effect with target as caster
                                run_effect(target, skill);
                        }

                        continue;
                }

                auto* const prop = new PropSlowed();

                prop->set_duration(duration);

                target->properties.apply(prop);
        }
}

std::vector<std::string> SpellSlow::descr_specific(
        const SpellSkill skill) const
{
        (void)skill;

        std::vector<std::string> descr;

        descr.push_back(
                "Causes the spells victim's to move more slowly (-50% speed).");

        if (skill == SpellSkill::basic)
        {
                descr.push_back("Affects one random visible hostile creature.");
        }
        else
        {
                descr.push_back("Affects all visible hostile creatures.");
        }

        Range duration_range;
        duration_range.min = 10 * ((int)skill + 1);
        duration_range.max = duration_range.min * 2;

        descr.push_back(
                "The spell lasts " +
                duration_range.str() +
                " turns.");

        return descr;
}

int SpellSlow::mon_cooldown() const
{
        return 20;
}

bool SpellSlow::allow_mon_cast_now(Mon& mon) const
{
        return mon.target_ && mon.is_target_seen_;
}

// -----------------------------------------------------------------------------
// Terrify
// -----------------------------------------------------------------------------
int SpellTerrify::max_spi_cost(const SpellSkill skill) const
{
        (void)skill;

        return 5;
}

int SpellTerrify::mon_cooldown() const
{
        return 5;
}

void SpellTerrify::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        Range duration_range;
        duration_range.min = 10 * ((int)skill + 1);
        duration_range.max = duration_range.min * 2;

        const int duration = duration_range.roll();

        auto targets = caster->seen_foes();

        if (targets.empty())
        {
                msg_log::add("The bugs on the ground suddenly scatter away.");

                return;
        }

        // There are targets available

        if (skill == SpellSkill::basic)
        {
                auto* const target = rnd::element(targets);

                targets.clear();

                targets.push_back(target);
        }

        io::draw_blast_at_seen_actors(targets, colors::magenta());

        for (Actor* const target : targets)
        {
                // Spell resistance?
                if (target->properties.has(PropId::r_spell))
                {
                        on_resist(*target);

                        // Spell reflection?
                        if (target->properties.has(PropId::spell_reflect))
                        {
                                if (map::player->can_see_actor(*target))
                                {
                                        msg_log::add(
                                                spell_reflect_msg,
                                                colors::text(),
                                                false,
                                                MorePromptOnMsg::yes);
                                }

                                // Run effect with target as caster
                                run_effect(target, skill);
                        }

                        continue;
                }

                auto* const prop = new PropTerrified();

                prop->set_duration(duration);

                target->properties.apply(prop);
        }
}

std::vector<std::string> SpellTerrify::descr_specific(
        const SpellSkill skill) const
{
        (void)skill;

        std::vector<std::string> descr;

        descr.push_back(
                "Manifests an overpowering feeling of dread in the spell's "
                "victims.");

        if (skill == SpellSkill::basic)
        {
                descr.push_back("Affects one random visible hostile creature.");
        }
        else
        {
                descr.push_back("Affects all visible hostile creatures.");
        }

        Range duration_range;
        duration_range.min = 10 * ((int)skill + 1);
        duration_range.max = duration_range.min * 2;

        descr.push_back(
                "The spell lasts " +
                duration_range.str() +
                " turns.");

        return descr;
}

bool SpellTerrify::allow_mon_cast_now(Mon& mon) const
{
        return mon.target_ && mon.is_target_seen_;
}

// -----------------------------------------------------------------------------
// Disease
// -----------------------------------------------------------------------------
void SpellDisease::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        (void)skill;

        ASSERT(!caster->is_player());

        Actor* caster_used = caster;

        auto* const mon = static_cast<Mon*>(caster_used);

        Actor* target = mon->target_;

        ASSERT(target);

        ASSERT(mon->is_target_seen_);

        // Spell resistance?
        if (target->properties.has(PropId::r_spell))
        {
                on_resist(*target);

                // Spell reflection?
                if (target->properties.has(PropId::spell_reflect))
                {
                        if (map::player->can_see_actor(*target))
                        {
                                msg_log::add(
                                        spell_reflect_msg,
                                        colors::text(),
                                        false,
                                        MorePromptOnMsg::yes);
                        }

                        std::swap(caster_used, target);
                }
                else // No spell reflection, just abort
                {
                        return;
                }
        }

        std::string actor_name = "me";

        if (!target->is_player())
        {
                actor_name = target->name_the();
        }

        if (map::player->can_see_actor(*target))
        {
                msg_log::add(
                        "A horrible disease is starting to afflict " +
                        actor_name +
                        "!");
        }

        target->properties.apply(new PropDiseased());
}

bool SpellDisease::allow_mon_cast_now(Mon& mon) const
{
        return mon.target_ && mon.is_target_seen_;
}

// -----------------------------------------------------------------------------
// Summon monster
// -----------------------------------------------------------------------------
void SpellSummonMon::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        std::vector<ActorId> summon_bucket;

        if (caster->is_player())
        {
                // Player summong, pick from predefined lists
                if ((skill == SpellSkill::basic) ||
                    (skill == SpellSkill::expert))
                {
                        summon_bucket.push_back(ActorId::raven);
                        summon_bucket.push_back(ActorId::wolf);
                }

                if ((skill == SpellSkill::expert) ||
                    (skill == SpellSkill::master))
                {
                        summon_bucket.push_back(ActorId::green_spider);
                        summon_bucket.push_back(ActorId::white_spider);
                }

                if (skill == SpellSkill::master)
                {
                        summon_bucket.push_back(ActorId::leng_spider);
                        summon_bucket.push_back(ActorId::energy_hound);
                        summon_bucket.push_back(ActorId::fire_hound);
                }
        }
        else // Caster is monster
        {
                int max_dlvl_spawned =
                        (skill == SpellSkill::basic) ? dlvl_last_early_game :
                        (skill == SpellSkill::expert) ? dlvl_last_mid_game :
                        dlvl_last;

                const int nr_dlvls_ood_allowed = 3;

                max_dlvl_spawned =
                        std::min(max_dlvl_spawned,
                                 map::dlvl + nr_dlvls_ood_allowed);

                const int min_dlvl_spawned = max_dlvl_spawned - 10;

                for (size_t i = 0; i < (size_t)ActorId::END; ++i)
                {
                        const ActorData& data = actor_data::data[i];

                        if (!data.can_be_summoned_by_mon)
                        {
                                continue;
                        }

                        if ((data.spawn_min_dlvl <= max_dlvl_spawned) &&
                            (data.spawn_min_dlvl >= min_dlvl_spawned))
                        {
                                summon_bucket.push_back(ActorId(i));
                        }
                }

                // If no monsters could be found which matched the level
                // criteria, try again, but without the min level criterium
                if (summon_bucket.empty())
                {
                        for (size_t i = 0; i < (size_t)ActorId::END; ++i)
                        {
                                const ActorData& data = actor_data::data[i];

                                if (!data.can_be_summoned_by_mon)
                                {
                                        continue;
                                }

                                if (data.spawn_min_dlvl <= max_dlvl_spawned)
                                {
                                        summon_bucket.push_back(ActorId(i));
                                }
                        }
                }
        }

        if (summon_bucket.empty())
        {
                TRACE << "No elligible monsters found for spawning"
                      << std::endl;

                ASSERT(false);

                return;
        }

#ifndef NDEBUG
        if (!caster->is_player())
        {
                for (const auto id : summon_bucket)
                {
                        ASSERT(actor_data::data[(size_t)id]
                               .can_be_summoned_by_mon);
                }
        }
#endif // NDEBUG

        const ActorId mon_id = rnd::element(summon_bucket);

        Actor* leader = nullptr;

        if (caster->is_player())
        {
                leader = caster;
        }
        else // Caster is monster
        {
                Actor* const caster_leader = static_cast<Mon*>(caster)->leader_;

                leader =
                        caster_leader
                        ? caster_leader
                        : caster;
        }

        const auto summoned =
                actor_factory::spawn(caster->pos, {mon_id}, map::rect())
                .make_aware_of_player()
                .set_leader(leader)
                .for_each([](Mon* const mon)
                {
                        mon->properties.apply(new PropSummoned());

                        auto prop_waiting = new PropWaiting();

                        prop_waiting->set_duration(2);

                        mon->properties.apply(prop_waiting);
                });

        if (summoned.monsters.empty())
        {
                return;
        }

        Mon* const mon = summoned.monsters[0];

        if (map::player->can_see_actor(*mon))
        {
                msg_log::add(
                        text_format::first_to_upper(
                                mon->name_a()) +
                        " appears!");
        }

        // Player cannot see monster
        else if (caster->is_player())
        {
                msg_log::add("I sense a new presence.");
        }
}

std::vector<std::string> SpellSummonMon::descr_specific(
        const SpellSkill skill) const
{
        (void)skill;

        return
        {
                "Summons a creature to do the caster's bidding. A more skilled "
                        "sorcerer summons beings of greater might and rarity."
                        };
}

bool SpellSummonMon::allow_mon_cast_now(Mon& mon) const
{
        return
                mon.target_ &&
                (mon.is_target_seen_ || rnd::one_in(30));
}

// -----------------------------------------------------------------------------
// Summon tentacles
// -----------------------------------------------------------------------------
void SpellSummonTentacles::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        (void)skill;

        Actor* leader = nullptr;

        if (caster->is_player())
        {
                leader = caster;
        }
        else // Caster is monster
        {
                Actor* const caster_leader = static_cast<Mon*>(caster)->leader_;

                leader =
                        caster_leader
                        ? caster_leader
                        : caster;
        }

        const auto summoned =
                actor_factory::spawn(caster->pos,
                                     {ActorId::tentacles},
                                     map::rect())
                .make_aware_of_player()
                .set_leader(leader)
                .for_each([](Mon* const mon)
                {
                        mon->properties.apply(new PropSummoned());

                        auto prop_waiting = new PropWaiting();

                        prop_waiting->set_duration(2);

                        mon->properties.apply(prop_waiting);
                });

        if (summoned.monsters.empty())
        {
                return;
        }

        Mon* const mon = summoned.monsters[0];

        if (map::player->can_see_actor(*mon))
        {
                msg_log::add("Monstrous tentacles rises up from the ground!");
        }
}

bool SpellSummonTentacles::allow_mon_cast_now(Mon& mon) const
{
        return mon.target_ && mon.is_target_seen_;
}

// -----------------------------------------------------------------------------
// Healing
// -----------------------------------------------------------------------------
void SpellHeal::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        if ((int)skill >= (int)SpellSkill::expert)
        {
                caster->properties.end_prop(PropId::infected);
                caster->properties.end_prop(PropId::diseased);
                caster->properties.end_prop(PropId::weakened);
                caster->properties.end_prop(PropId::hp_sap);
        }

        if (skill == SpellSkill::master)
        {
                caster->properties.end_prop(PropId::blind);
                caster->properties.end_prop(PropId::poisoned);

                if (caster->is_player())
                {
                        Prop* const wound_prop =
                                map::player->properties.prop(PropId::wound);

                        if (wound_prop)
                        {
                                auto* const wound =
                                        static_cast<PropWound*>(wound_prop);

                                wound->heal_one_wound();
                        }
                }
        }

        const int nr_hp_restored = 8 + (int)skill * 8;

        caster->restore_hp(nr_hp_restored);
}

bool SpellHeal::allow_mon_cast_now(Mon& mon) const
{
        return mon.hp < actor::max_hp(mon);
}

std::vector<std::string> SpellHeal::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        const int nr_hp_restored = 8 + (int)skill * 8;

        descr.push_back(
                "Restores " +
                std::to_string(nr_hp_restored) +
                " Hit Points.");

        if (skill == SpellSkill::expert)
        {
                descr.push_back(
                        "Cures infections, disease, weakening, and life "
                        "sapping.");
        }
        else if (skill == SpellSkill::master)
        {
                descr.push_back(
                        "Cures infections, disease, weakening, life sapping, "
                        "blindness, and poisoning.");

                descr.push_back(
                        "Heals one wound.");
        }

        return descr;
}

// -----------------------------------------------------------------------------
// Mi-go hypnosis
// -----------------------------------------------------------------------------
void SpellMiGoHypno::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        (void)skill;

        ASSERT(!caster->is_player());

        Actor* caster_used = caster;

        auto* const mon = static_cast<Mon*>(caster_used);

        Actor* target = mon->target_;

        ASSERT(target);

        ASSERT(mon->is_target_seen_);

        // Spell resistance?
        if (target->properties.has(PropId::r_spell))
        {
                on_resist(*target);

                // Spell reflection?
                if (target->properties.has(PropId::spell_reflect))
                {
                        if (map::player->can_see_actor(*target))
                        {
                                msg_log::add(
                                        spell_reflect_msg,
                                        colors::text(),
                                        false,
                                        MorePromptOnMsg::yes);
                        }

                        std::swap(caster_used, target);
                }
                else // No spell reflection, just abort
                {
                        return;
                }
        }

        if (target->is_player())
        {
                msg_log::add("There is a sharp droning in my head!");
        }

        if (rnd::coin_toss())
        {
                auto prop_fainted = new PropFainted();

                prop_fainted->set_duration(rnd::range(2, 10));

                target->properties.apply(prop_fainted);
        }
        else
        {
                msg_log::add("I feel dizzy.");
        }
}

bool SpellMiGoHypno::allow_mon_cast_now(Mon& mon) const
{
        return
                mon.target_ &&
                mon.is_target_seen_ &&
                mon.target_->is_player();
}

// -----------------------------------------------------------------------------
// Immolation
// -----------------------------------------------------------------------------
void SpellBurn::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        ASSERT(!caster->is_player());

        Actor* caster_used = caster;

        auto* const mon = static_cast<Mon*>(caster_used);

        Actor* target = mon->target_;

        ASSERT(target);

        ASSERT(mon->is_target_seen_);

        // Spell resistance?
        if (target->properties.has(PropId::r_spell))
        {
                on_resist(*target);

                // Spell reflection?
                if (target->properties.has(PropId::spell_reflect))
                {
                        if (map::player->can_see_actor(*target))
                        {
                                msg_log::add(spell_reflect_msg,
                                             colors::text(),
                                             false,
                                             MorePromptOnMsg::yes);
                        }

                        std::swap(caster_used, target);
                }
                else // No spell reflection, just abort
                {
                        return;
                }
        }

        std::string target_str = "me";

        if (!target->is_player())
        {
                target_str = target->name_the();
        }

        if (map::player->can_see_actor(*target))
        {
                msg_log::add("Flames are rising around " + target_str + "!");
        }

        auto prop = new PropBurning();

        prop->set_duration(2 + (int)skill);

        target->properties.apply(prop);
}

bool SpellBurn::allow_mon_cast_now(Mon& mon) const
{
        return mon.target_ && mon.is_target_seen_;
}

// -----------------------------------------------------------------------------
// Deafening
// -----------------------------------------------------------------------------
void SpellDeafen::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        ASSERT(!caster->is_player());

        Actor* caster_used = caster;

        auto* const mon = static_cast<Mon*>(caster_used);

        Actor* target = mon->target_;

        ASSERT(target);

        ASSERT(mon->is_target_seen_);

        // Spell resistance?
        if (target->properties.has(PropId::r_spell))
        {
                on_resist(*target);

                // Spell reflection?
                if (target->properties.has(PropId::spell_reflect))
                {
                        if (map::player->can_see_actor(*target))
                        {
                                msg_log::add(spell_reflect_msg,
                                             colors::text(),
                                             false,
                                             MorePromptOnMsg::yes);
                        }

                        std::swap(caster_used, target);
                }
                else // No spell reflection, just abort
                {
                        return;
                }
        }

        auto prop = new PropDeaf();

        prop->set_duration(75 + (int)skill * 75);

        target->properties.apply(prop);
}

bool SpellDeafen::allow_mon_cast_now(Mon& mon) const
{
        return mon.target_ && mon.is_target_seen_;
}

// -----------------------------------------------------------------------------
// Transmutation
// -----------------------------------------------------------------------------
void SpellTransmut::run_effect(
        Actor* const caster,
        const SpellSkill skill) const
{
        (void)caster;

        const P& p = map::player->pos;

        auto& cell = map::cells.at(p);

        Item* item_before = cell.item;

        if (!item_before)
        {
                msg_log::add("There is a vague change in the air.");

                return;
        }

        // Player is standing on an item

        // Get information on the existing item(s)
        const bool is_stackable_before = item_before->data().is_stackable;

        const int nr_items_before =
                is_stackable_before
                ? item_before->nr_items_
                : 1;

        const auto item_type_before = item_before->data().type;

        const int melee_wpn_plus = item_before->melee_base_dmg_.plus;

        const auto id_before = item_before->id();

        std::string item_name_before = "The ";

        if (nr_items_before > 1)
        {
                item_name_before += item_before->name(ItemRefType::plural);
        }
        else // Single item
        {
                item_name_before += item_before->name(ItemRefType::plain);
        }

        // Remove the existing item(s)
        delete cell.item;

        cell.item = nullptr;

        if (cell.is_seen_by_player)
        {
                msg_log::add(
                        item_name_before + " disappears.",
                        colors::text(),
                        false,
                        MorePromptOnMsg::yes);
        }

        // Determine which item(s) to spawn, if any
        int pct_chance_per_item = (int)skill * 20;

        std::vector<ItemId> id_bucket;

        // Converting a potion?
        if (item_type_before == ItemType::potion)
        {
                pct_chance_per_item += 50;

                for (size_t item_id = 0;
                     (ItemId)item_id != ItemId::END;
                     ++item_id)
                {
                        if ((ItemId)item_id == id_before)
                        {
                                continue;
                        }

                        const auto& d = item_data::data[item_id];

                        if (d.type == ItemType::potion)
                        {
                                id_bucket.push_back((ItemId)item_id);
                        }
                }
        }
        // Converting a scroll?
        else if (item_type_before == ItemType::scroll)
        {
                pct_chance_per_item += 50;

                for (size_t item_id = 0;
                     (ItemId)item_id != ItemId::END;
                     ++item_id)
                {
                        if ((ItemId)item_id == id_before)
                        {
                                continue;
                        }

                        const auto& d = item_data::data[item_id];

                        if (d.type == ItemType::scroll)
                        {
                                id_bucket.push_back((ItemId)item_id);
                        }
                }
        }
        // Converting a melee weapon (with at least one "plus")?
        else if ((item_type_before == ItemType::melee_wpn) &&
                 (melee_wpn_plus >= 1))
        {
                pct_chance_per_item += (melee_wpn_plus * 10);

                for (size_t item_id = 0;
                     (ItemId)item_id != ItemId::END;
                     ++item_id)
                {
                        const auto& d = item_data::data[item_id];

                        if ((d.type == ItemType::potion) ||
                            (d.type == ItemType::scroll))
                        {
                                id_bucket.push_back((ItemId)item_id);
                        }
                }
        }

        // Never spawn Transmute scrolls, this is just dumb
        for (auto it = begin(id_bucket); it != end(id_bucket); )
        {
                if (*it == ItemId::scroll_transmut)
                {
                        it = id_bucket.erase(it);
                }
                else // Not transmute
                {
                        ++it;
                }
        }

        ItemId id_new = ItemId::END;

        if (!id_bucket.empty())
        {
                id_new = rnd::element(id_bucket);
        }

        int nr_items_new = 0;

        // How many items?
        for (int i = 0; i < nr_items_before; ++i)
        {
                if (rnd::percent(pct_chance_per_item))
                {
                        ++nr_items_new;
                }
        }

        if ((id_new == ItemId::END) ||
            (nr_items_new < 1))
        {
                msg_log::add("Nothing appears.");

                return;
        }

        // OK, items are good, and player succeeded the rolls etc

        // Spawn new item(s)
        auto* const item_new =
                item_factory::make_item_on_floor(id_new, map::player->pos);

        if (item_new->data().is_stackable)
        {
                item_new->nr_items_ = nr_items_new;
        }

        if (cell.is_seen_by_player)
        {
                const std::string item_name_new =
                        text_format::first_to_upper(
                                item_new->name(ItemRefType::plural));

                msg_log::add(item_name_new + " appears.");
        }
}

std::vector<std::string> SpellTransmut::descr_specific(
        const SpellSkill skill) const
{
        std::vector<std::string> descr;

        descr.push_back(
                "Attempts to convert items (stand over an item when casting). "
                "On failure, the item is destroyed.");

        const int skill_bon = (int)skill * 20;

        const int chance_per_pot = skill_bon + 50;

        const int chance_per_scroll = skill_bon + 50;

        const int chance_per_wpn_plus = 10;

        const int chance_wpn_plus_1 = skill_bon + chance_per_wpn_plus;
        const int chance_wpn_plus_2 = skill_bon + chance_per_wpn_plus * 2;
        const int chance_wpn_plus_3 = skill_bon + chance_per_wpn_plus * 3;

        descr.push_back(
                "Converts Potions with " +
                std::to_string(chance_per_pot) +
                "% chance.");

        descr.push_back(
                "Converts Manuscripts with " +
                std::to_string(chance_per_scroll) +
                "% chance.");

        descr.push_back(
                "Melee weapons with at least +1 damage (not counting any "
                "damage bonus from skills) are converted to a Potion or "
                "Manuscript, with " +
                std::to_string(chance_wpn_plus_1) +
                "% chance for a +1 weapon, " +
                std::to_string(chance_wpn_plus_2) +
                "% chance for a +2 weapon, " +
                std::to_string(chance_wpn_plus_3) +
                "% chance for a +3 weapon, etc.");

        return descr;
}
