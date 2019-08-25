// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ITEM_HPP
#define ITEM_HPP

#include "colors.hpp"
#include "dmg_range.hpp"
#include "gfx.hpp"
#include "inventory_handling.hpp"
#include "item_att_property.hpp"
#include "item_curse.hpp"
#include "random.hpp"


namespace actor
{
class Actor;
}


class Prop;
class Spell;


namespace item
{

enum class Id;
struct ItemData;


enum class ItemActivateRetType
{
        keep,
        destroyed
};


class Item
{
public:
        Item(ItemData* item_data);

        Item(Item& other) = delete;

        Item& operator=(const Item& other);

        virtual ~Item();

        Id id() const;

        void save();

        void load();

        ItemData& data() const;

        virtual Color color() const;

        char character() const;

        TileId tile() const;

        virtual LgtSize lgt_size() const
        {
                return LgtSize::none;
        }

        std::string name(
                const ItemRefType ref_type,
                const ItemRefInf inf = ItemRefInf::yes,
                const ItemRefAttInf att_inf = ItemRefAttInf::none) const;

        std::vector<std::string> descr() const;

        std::string hit_mod_str(const ItemRefAttInf att_inf) const;

        std::string dmg_str(
                const ItemRefAttInf att_inf,
                const ItemRefDmg dmg_value) const;

        // E.g. "{Off}" for Lanterns, or "4/7" for Pistols
        virtual std::string name_inf_str() const
        {
                return "";
        }

        virtual void identify(const Verbose verbose)
        {
                (void)verbose;
        }

        int weight() const;

        std::string weight_str() const;

        virtual ConsumeItem activate(actor::Actor* const actor);

        virtual Color interface_color() const
        {
                return colors::dark_yellow();
        }

        void on_std_turn_in_inv(const InvType inv_type);

        void on_actor_turn_in_inv(const InvType inv_type);

        void on_pickup(actor::Actor& actor);

        // "on_pickup()" should be called before this
        void on_equip(const Verbose verbose);

        void on_unequip();

        // This is the opposite of "on_pickup()". If this is a wielded item,
        // "on_unequip()" should be called first.
        void on_removed_from_inv();

        // Called when:
        // * Player walks into the same cell as the item,
        // * The item is dropped into the same cell as the player,
        // * The item is picked up,
        // * The item is found in an item container, but not picked up
        void on_player_found();

        void on_player_reached_new_dlvl();

        virtual void on_projectile_blocked(
                const P& prev_pos,
                const P& current_pos)
        {
                (void)prev_pos;
                (void)current_pos;
        }

        virtual void on_melee_hit(actor::Actor& actor_hit, const int dmg)
        {
                (void)actor_hit;
                (void)dmg;
        }

        void set_melee_base_dmg(const DmgRange& range)
        {
                m_melee_base_dmg = range;
        }

        void set_ranged_base_dmg(const DmgRange& range)
        {
                m_ranged_base_dmg = range;
        }

        void set_melee_plus(const int plus)
        {
                m_melee_base_dmg.set_plus(plus);
        }

        void set_random_melee_plus();

        DmgRange melee_base_dmg() const
        {
                return m_melee_base_dmg;
        }

        DmgRange melee_dmg(const actor::Actor* const attacker) const;
        DmgRange ranged_dmg(const actor::Actor* const attacker) const;
        DmgRange thrown_dmg(const actor::Actor* const attacker) const;

        ItemAttProp& prop_applied_on_melee(
                const actor::Actor* const attacker) const;

        ItemAttProp& prop_applied_on_ranged(
                const actor::Actor* const attacker) const;

        virtual void on_melee_kill(actor::Actor& actor_killed)
        {
                (void)actor_killed;
        }

        virtual void on_ranged_hit(actor::Actor& actor_hit)
        {
                (void)actor_hit;
        }

        void add_carrier_prop(Prop* const prop, const Verbose verbose);

        void clear_carrier_props();

        virtual int hp_regen_change(const InvType inv_type) const
        {
                (void)inv_type;
                return 0;
        }

        // Used when attempting to fire or throw an item
        bool is_in_effective_range_lmt(const P& p0, const P& p1) const;

        actor::Actor* actor_carrying()
        {
                return m_actor_carrying;
        }

        void clear_actor_carrying()
        {
                m_actor_carrying = nullptr;
        }

        const std::vector<Prop*>& carrier_props() const
        {
                return m_carrier_props;
        }

        virtual bool is_curse_allowed(item_curse::Id id) const
        {
                (void)id;

                return true;
        }

        bool is_cursed() const
        {
                return (m_curse.id() != item_curse::Id::END);
        }

        item_curse::Curse& current_curse()
        {
                return m_curse;
        }

        void set_curse(item_curse::Curse&& curse)
        {
                m_curse = std::move(curse);
        }

        void remove_curse()
        {
                m_curse = item_curse::Curse();
        }

        int m_nr_items {1};

protected:
        virtual void save_hook() const {}

        virtual void load_hook() {}

        virtual std::vector<std::string> descr_hook() const;

        virtual void on_std_turn_in_inv_hook(const InvType inv_type)
        {
                (void)inv_type;
        }

        virtual void on_actor_turn_in_inv_hook(const InvType inv_type)
        {
                (void)inv_type;
        }

        virtual void on_pickup_hook() {}

        virtual void on_equip_hook(const Verbose verbose)
        {
                (void)verbose;
        }

        virtual void on_unequip_hook() {}

        virtual void on_removed_from_inv_hook() {}

        virtual void on_player_reached_new_dlvl_hook() {}

        virtual void specific_dmg_mod(
                DmgRange& range,
                const actor::Actor* const actor) const
        {
                (void)range;
                (void)actor;
        }

        ItemAttProp* prop_applied_intr_attack(
                const actor::Actor* const attacker) const;

        ItemData* m_data;

        actor::Actor* m_actor_carrying {nullptr};

        // Base damage (not including actor properties, player traits, etc)
        DmgRange m_melee_base_dmg;
        DmgRange m_ranged_base_dmg;

private:
        // Properties to apply on owning actor (when e.g. wearing the item, or
        // just keeping it in the inventory)
        std::vector<Prop*> m_carrier_props {};

        item_curse::Curse m_curse {};
};

class Armor: public Item
{
public:
        Armor(ItemData* const item_data);

        ~Armor() {}

        void save_hook() const override;
        void load_hook() override;

        Color interface_color() const override
        {
                return colors::gray();
        }

        std::string name_inf_str() const override;

        int armor_points() const;

        int durability() const
        {
                return m_dur;
        }

        void set_max_durability()
        {
                m_dur = 100;
        }

        bool is_destroyed() const
        {
                return armor_points() <= 0;
        }

        void hit(const int dmg);

protected:
        int m_dur;
};

class ArmorAsbSuit: public Armor
{
public:
        ArmorAsbSuit(ItemData* const item_data) :
                Armor(item_data) {}

        ~ArmorAsbSuit() {}

        void on_equip_hook(const Verbose verbose) override;

private:
        void on_unequip_hook() override;
};

class ArmorMiGo: public Armor
{
public:
        ArmorMiGo(ItemData* const item_data) :
                Armor(item_data) {}

        ~ArmorMiGo() {}

        void on_equip_hook(const Verbose verbose) override;
};

class Wpn: public Item
{
public:
        Wpn(ItemData* const item_data);

        virtual ~Wpn() {}

        Wpn& operator=(const Wpn& other) = delete;

        void save_hook() const override;
        void load_hook() override;

        Color color() const override;

        Color interface_color() const override
        {
                return colors::gray();
        }

        std::string name_inf_str() const override;

        const ItemData& ammo_data()
        {
                return *m_ammo_data;
        }

        int m_ammo_loaded;

protected:
        ItemData* m_ammo_data;
};

class SpikedMace : public Wpn
{
public:
        SpikedMace(ItemData* const item_data) :
                Wpn(item_data) {}

private:
        void on_melee_hit(actor::Actor& actor_hit, const int dmg) override;
};

class PlayerGhoulClaw : public Wpn
{
public:
        PlayerGhoulClaw(ItemData* const item_data) :
                Wpn(item_data) {}

private:
        void on_melee_hit(actor::Actor& actor_hit, const int dmg) override;

        void on_melee_kill(actor::Actor& actor_killed) override;
};

class ZombieDust : public Wpn
{
public:
        ZombieDust(ItemData* const item_data) :
                Wpn(item_data) {}

        void on_ranged_hit(actor::Actor& actor_hit);
};

class Incinerator: public Wpn
{
public:
        Incinerator(ItemData* const item_data);
        ~Incinerator() {}

        void on_projectile_blocked(
                const P& prev_pos,
                const P& current_pos);
};

class MiGoGun: public Wpn
{
public:
        MiGoGun(ItemData* const item_data);
        ~MiGoGun() {}

protected:
        void specific_dmg_mod(
                DmgRange& dice,
                const actor::Actor* const actor) const override;
};

class RavenPeck : public Wpn
{
public:
        RavenPeck(ItemData* const item_data) :
                Wpn(item_data) {}

        void on_melee_hit(actor::Actor& actor_hit, const int dmg) override;
};

class VampiricBite : public Wpn
{
public:
        VampiricBite(ItemData* const item_data) :
                Wpn(item_data) {}

        void on_melee_hit(actor::Actor& actor_hit, const int dmg) override;
};

class MindLeechSting : public Wpn
{
public:
        MindLeechSting(ItemData* const item_data) :
                Wpn(item_data) {}

        void on_melee_hit(actor::Actor& actor_hit, const int dmg) override;
};

class DustEngulf : public Wpn
{
public:
        DustEngulf(ItemData* const item_data) :
                Wpn(item_data) {}

        void on_melee_hit(actor::Actor& actor_hit, const int dmg) override;
};

class SnakeVenomSpit : public Wpn
{
public:
        SnakeVenomSpit(ItemData* const item_data) :
                Wpn(item_data) {}

        void on_ranged_hit(actor::Actor& actor_hit) override;
};

class Ammo: public Item
{
public:
        Ammo(ItemData* const item_data) :
                Item(item_data) {}

        virtual ~Ammo() {}

        Color interface_color() const override
        {
                return colors::white();
        }
};

class AmmoMag: public Ammo
{
public:
        AmmoMag(ItemData* const item_data);

        ~AmmoMag() {}

        std::string name_inf_str() const override
        {
                return "{" + std::to_string(m_ammo) + "}";
        }

        void set_full_ammo();

        void save_hook() const override;

        void load_hook() override;

        int m_ammo;
};

enum class MedBagAction
{
        treat_wound,
        sanitize_infection,
        END
};

class MedicalBag: public Item
{
public:
        MedicalBag(ItemData* const item_data);

        ~MedicalBag() {}

        void save_hook() const override;

        void load_hook() override;

        Color interface_color() const override
        {
                return colors::green();
        }

        std::string name_inf_str() const override
        {
                return "{" + std::to_string(m_nr_supplies) + "}";
        }

        void on_pickup_hook() override;

        ConsumeItem activate(actor::Actor* const actor) override;

        void continue_action();

        void interrupted();

        void finish_current_action();

        int m_nr_supplies;

protected:
        MedBagAction choose_action() const;

        int tot_suppl_for_action(const MedBagAction action) const;

        int tot_turns_for_action(const MedBagAction action) const;

        int m_nr_turns_left_action;

        MedBagAction m_current_action;
};

class Headwear: public Item
{
public:
        Headwear(ItemData* item_data) :
                Item(item_data) {}

        Color interface_color() const override
        {
                return colors::brown();
        }
};

class GasMask: public Headwear
{
public:
        GasMask(ItemData* item_data) :
                Headwear        (item_data),
                m_nr_turns_left  (60) {}

        std::string name_inf_str() const override
        {
                return "{" + std::to_string(m_nr_turns_left) + "}";
        }

        void on_equip_hook(const Verbose verbose) override;

        void on_unequip_hook() override;

        void decr_turns_left(Inventory& carrier_inv);

protected:
        int m_nr_turns_left;
};

class Explosive : public Item
{
public:
        virtual ~Explosive() {}

        Explosive() = delete;

        ConsumeItem activate(actor::Actor* const actor) override final;

        Color interface_color() const override final
        {
                return colors::light_red();
        }

        virtual void on_std_turn_player_hold_ignited() = 0;
        virtual void on_thrown_ignited_landing(const P& p) = 0;
        virtual void on_player_paralyzed() = 0;
        virtual Color ignited_projectile_color() const = 0;
        virtual std::string str_on_player_throw() const = 0;

protected:
        Explosive(ItemData* const item_data) :
                Item(item_data),
                m_fuse_turns(-1) {}

        virtual int std_fuse_turns() const = 0;
        virtual void on_player_ignite() const = 0;

        int m_fuse_turns;
};

class Dynamite: public Explosive
{
public:
        Dynamite(ItemData* const item_data) :
                Explosive(item_data) {}

        void on_thrown_ignited_landing(const P& p) override;
        void on_std_turn_player_hold_ignited() override;
        void on_player_paralyzed() override;

        Color ignited_projectile_color() const override
        {
                return colors::light_red();
        }

        std::string str_on_player_throw() const override
        {
                return "I throw a lit dynamite stick.";
        }

protected:
        int std_fuse_turns() const override
        {
                return 6;
        }

        void on_player_ignite() const override;
};

class Molotov: public Explosive
{
public:
        Molotov(ItemData* const item_data) :
                Explosive(item_data) {}

        void on_thrown_ignited_landing(const P& p) override;
        void on_std_turn_player_hold_ignited() override;
        void on_player_paralyzed() override;

        Color ignited_projectile_color() const override
        {
                return colors::yellow();
        }

        std::string str_on_player_throw() const override
        {
                return "I throw a lit Molotov Cocktail.";
        }

protected:
        int std_fuse_turns() const override
        {
                return 12;
        }
        void on_player_ignite() const override;
};

class Flare: public Explosive
{
public:
        Flare(ItemData* const item_data) :
                Explosive(item_data) {}

        void on_thrown_ignited_landing(const P& p) override;
        void on_std_turn_player_hold_ignited() override;
        void on_player_paralyzed() override;

        Color ignited_projectile_color() const override
        {
                return colors::yellow();
        }

        std::string str_on_player_throw() const override
        {
                return "I throw a lit flare.";
        }

protected:
        int std_fuse_turns() const override
        {
                return 200;
        }
        void on_player_ignite() const override;
};

class SmokeGrenade: public Explosive
{
public:
        SmokeGrenade(ItemData* const item_data) :
                Explosive(item_data) {}

        void on_thrown_ignited_landing(const P& p) override;
        void on_std_turn_player_hold_ignited() override;
        void on_player_paralyzed() override;

        Color ignited_projectile_color() const override;

        std::string str_on_player_throw() const override
        {
                return "I throw a smoke grenade.";
        }

protected:
        int std_fuse_turns() const override
        {
                return 12;
        }

        void on_player_ignite() const override;
};

} // item

#endif // ITEM_HPP
