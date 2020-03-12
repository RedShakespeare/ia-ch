// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "item_factory.hpp"

#include "debug.hpp"
#include "drop.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "item_artifact.hpp"
#include "item_curse.hpp"
#include "item_data.hpp"
#include "item_device.hpp"
#include "item_potion.hpp"
#include "item_rod.hpp"
#include "item_scroll.hpp"


namespace item
{

Item* make(const Id item_id, const int nr_items)
{
        Item* r = nullptr;

        auto* d = &g_data[(size_t)item_id];

        // Sanity check
        ASSERT(d->id == item_id);

        switch (item_id)
        {
        case Id::trapez:
                r = new Item(d);
                break;

        case Id::sawed_off:
        case Id::pump_shotgun:
        case Id::machine_gun:
        case Id::pistol:
        case Id::revolver:
        case Id::rifle:
        case Id::flare_gun:
        case Id::spike_gun:
        case Id::rock:
        case Id::thr_knife:
        case Id::iron_spike:
        case Id::dagger:
        case Id::hatchet:
        case Id::club:
        case Id::hammer:
        case Id::machete:
        case Id::axe:
        case Id::pitch_fork:
        case Id::sledge_hammer:
        case Id::trap_dart:
        case Id::trap_dart_poison:
        case Id::trap_spear:
        case Id::trap_spear_poison:
        case Id::player_kick:
        case Id::player_stomp:
        case Id::player_punch:
        case Id::intr_bite:
        case Id::intr_claw:
        case Id::intr_strike:
        case Id::intr_punch:
        case Id::intr_acid_spit:
        case Id::intr_fire_breath:
        case Id::intr_energy_breath:
        case Id::intr_strangle:
        case Id::intr_ghost_touch:
        case Id::intr_sting:
        case Id::intr_spear_thrust:
        case Id::intr_net_throw:
        case Id::intr_maul:
        case Id::intr_pus_spew:
        case Id::intr_acid_touch:
        case Id::intr_fire_engulf:
        case Id::intr_energy_engulf:
        case Id::intr_spores:
        case Id::intr_web_bola:
                r = new Wpn(d);
                break;

        case Id::incinerator:
                r = new Incinerator(d);
                break;

        case Id::mi_go_gun:
                r = new MiGoGun(d);
                break;

        case Id::spiked_mace:
                r = new SpikedMace(d);
                break;

        case Id::revolver_bullet:
        case Id::rifle_bullet:
        case Id::shotgun_shell:
                r = new Ammo(d);
                break;

        case Id::drum_of_bullets:
        case Id::pistol_mag:
        case Id::incinerator_ammo:
                r = new AmmoMag(d);
                break;

        case Id::zombie_dust:
                r = new ZombieDust(d);
                break;

        case Id::dynamite:
                r = new Dynamite(d);
                break;

        case Id::flare:
                r = new Flare(d);
                break;

        case Id::molotov:
                r = new Molotov(d);
                break;

        case Id::smoke_grenade:
                r = new SmokeGrenade(d);
                break;

        case Id::player_ghoul_claw:
                r = new PlayerGhoulClaw(d);
                break;

        case Id::intr_raven_peck:
                r = new RavenPeck(d);
                break;

        case Id::intr_vampiric_bite:
                r = new VampiricBite(d);
                break;

        case Id::intr_mind_leech_sting:
                r = new MindLeechSting(d);
                break;

        case Id::intr_dust_engulf:
                r = new DustEngulf(d);
                break;

        case Id::intr_snake_venom_spit:
                r = new SnakeVenomSpit(d);
                break;

        case Id::armor_flak_jacket:
        case Id::armor_leather_jacket:
        case Id::armor_iron_suit:
                r = new Armor(d);
                break;

        case Id::armor_asb_suit:
                r = new ArmorAsbSuit(d);
                break;

        case Id::armor_mi_go:
                r = new ArmorMiGo(d);
                break;

        case Id::gas_mask:
                r = new GasMask(d);
                break;

        case Id::scroll_aura_of_decay:
        case Id::scroll_mayhem:
        case Id::scroll_telep:
        case Id::scroll_pest:
        case Id::scroll_enfeeble:
        case Id::scroll_slow:
        case Id::scroll_terrify:
        case Id::scroll_searching:
        case Id::scroll_bless:
        case Id::scroll_darkbolt:
        case Id::scroll_aza_wrath:
        case Id::scroll_opening:
        case Id::scroll_res:
        case Id::scroll_summon_mon:
        case Id::scroll_light:
        case Id::scroll_spectral_wpns:
        case Id::scroll_transmut:
        case Id::scroll_heal:
        case Id::scroll_see_invis:
        case Id::scroll_premonition:
        case Id::scroll_haste:
        case Id::scroll_spell_shield:
                r = new scroll::Scroll(d);
                break;

        case Id::potion_vitality:
                r = new potion::Vitality(d);
                break;

        case Id::potion_spirit:
                r = new potion::Spirit(d);
                break;

        case Id::potion_blindness:
                r = new potion::Blindness(d);
                break;

        case Id::potion_fortitude:
                r = new potion::Fortitude(d);
                break;

        case Id::potion_paralyze:
                r = new potion::Paral(d);
                break;

        case Id::potion_r_elec:
                r = new potion::RElec(d);
                break;

        case Id::potion_conf:
                r = new potion::Conf(d);
                break;

        case Id::potion_poison:
                r = new potion::Poison(d);
                break;

        case Id::potion_insight:
                r = new potion::Insight(d);
                break;

        case Id::potion_r_fire:
                r = new potion::RFire(d);
                break;

        case Id::potion_curing:
                r = new potion::Curing(d);
                break;

        case Id::potion_descent:
                r = new potion::Descent(d);
                break;

        case Id::potion_invis:
                r = new potion::Invis(d);
                break;

        case Id::device_blaster:
                r = new device::Blaster(d);
                break;

        case Id::device_rejuvenator:
                r = new device::Rejuvenator(d);
                break;

        case Id::device_translocator:
                r = new device::Translocator(d);
                break;

        case Id::device_sentry_drone:
                r = new device::SentryDrone(d);
                break;

        case Id::device_deafening:
                r = new device::Deafening(d);
                break;

        case Id::device_force_field:
                r = new device::ForceField(d);
                break;

        case Id::lantern:
                r = new device::Lantern(d);
                break;

        case Id::rod_curing:
                r = new rod::Curing(d);
                break;

        case Id::rod_opening:
                r = new rod::Opening(d);
                break;

        case Id::rod_bless:
                r = new rod::Bless(d);
                break;

        case Id::rod_cloud_minds:
                r = new rod::CloudMinds(d);
                break;

        case Id::rod_shockwave:
                r = new rod::Shockwave(d);
                break;

        case Id::medical_bag:
                r = new MedicalBag(d);
                break;

        case Id::pharaoh_staff:
                r = new PharaohStaff(d);
                break;

        case Id::refl_talisman:
                r = new ReflTalisman(d);
                break;

        case Id::resurrect_talisman:
                r = new ResurrectTalisman(d);
                break;

        case Id::tele_ctrl_talisman:
                r = new TeleCtrlTalisman(d);
                break;

        case Id::horn_of_malice:
                r = new HornOfMalice(d);
                break;

        case Id::horn_of_banishment:
                r = new HornOfBanishment(d);
                break;

        case Id::clockwork:
                r = new Clockwork(d);
                break;

        case Id::spirit_dagger:
                r = new SpiritDagger(d);
                break;

        case Id::orb_of_life:
                r = new OrbOfLife(d);
                break;

        case Id::END:
                break;
        }

        if (!r)
        {
                return nullptr;
        }

        // Sanity check number of items (non-stackable items should never be set
        // to anything other than one item)
        if (!r->data().is_stackable && (nr_items != 1))
        {
                TRACE << "Specified number of items ("
                      << nr_items
                      << ") != 1 for "
                      << "non-stackable item: "
                      << (int)d->id << ", "
                      << r->name(ItemRefType::plain)
                      << std::endl;

                ASSERT(false);
        }

        r->m_nr_items = nr_items;

        if (d->is_unique)
        {
                d->allow_spawn = false;
        }

        return r;
}

void set_item_randomized_properties(Item& item)
{
        const auto& d = item.data();

        ASSERT(d.type != ItemType::melee_wpn_intr &&
               d.type != ItemType::ranged_wpn_intr);

        // If it is a pure, common melee weapon, and "plus" damage is not
        // already specified, randomize the extra damage
        if (d.melee.is_melee_wpn &&
            !d.ranged.is_ranged_wpn &&
            !d.is_unique &&
            (item.melee_base_dmg().plus() == 0))
        {
                static_cast<Wpn&>(item).set_random_melee_plus();
        }

        // If firearm, spawn with random amount of ammo
        if (d.ranged.is_ranged_wpn && !d.ranged.has_infinite_ammo)
        {
                auto& wpn = static_cast<Wpn&>(item);

                if (wpn.data().ranged.max_ammo == 1)
                {
                        wpn.m_ammo_loaded = rnd::coin_toss() ? 1 : 0;
                }
                else // Weapon ammo capacity > 1
                {
                        const int ammo_cap = wpn.data().ranged.max_ammo;

                        if (d.ranged.is_machine_gun)
                        {
                                // Number of machine gun bullets loaded needs to
                                // be a multiple of the number of projectiles
                                // fired in each burst

                                const int cap_scaled =
                                        ammo_cap / g_nr_mg_projectiles;

                                const int min_scaled =
                                        cap_scaled / 4;

                                wpn.m_ammo_loaded =
                                        rnd::range(min_scaled, cap_scaled) *
                                        g_nr_mg_projectiles;
                        }
                        else // Not machinegun
                        {
                                wpn.m_ammo_loaded =
                                        rnd::range(ammo_cap / 4, ammo_cap);
                        }
                }
        }

        if (d.is_stackable)
        {
                item.m_nr_items = rnd::range(1, d.max_stack_at_spawn);
        }

        // Vary number of Medical supplies
        if (d.id == Id::medical_bag)
        {
                auto& medbag = static_cast<MedicalBag&>(item);

                const int nr_supplies_max = medbag.m_nr_supplies;

                const int nr_supplies_min =
                        nr_supplies_max - (nr_supplies_max / 3);

                medbag.m_nr_supplies =
                        rnd::range(nr_supplies_min, nr_supplies_max);
        }

        // Vary Lantern duration
        if (d.id == Id::lantern)
        {
                auto& lantern = static_cast<device::Lantern&>(item);

                const int duration_max = lantern.nr_turns_left;

                const int duration_min = duration_max / 2;

                lantern.nr_turns_left = rnd::range(duration_min, duration_max);
        }

        // Item curse
        const int cursed_one_in_n = 3;

        if (d.is_unique &&
            (d.value >= Value::supreme_treasure) &&
            rnd::one_in(cursed_one_in_n))
        {
                auto curse = item_curse::try_make_random_free_curse(item);

                if (curse.id() != item_curse::Id::END)
                {
                        item.set_curse(std::move(curse));
                }
        }
}

Item* make_item_on_floor(const Id item_id, const P& pos)
{
        auto* item = make(item_id);

        set_item_randomized_properties(*item);

        item_drop::drop_item_on_map(pos, *item);

        return item;
}

Item* copy_item(const Item& item_to_copy)
{
        auto* new_item = make(item_to_copy.id());

        *new_item = item_to_copy;

        return new_item;
}

} // namespace item
