// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef INVENTORY_HPP
#define INVENTORY_HPP

#include <vector>
#include <string>

#include "global.hpp"


namespace item
{
class Item;
enum class Id;
}

namespace actor
{
class Actor;
}

struct P;


enum class SlotId
{
        wpn,
        wpn_alt,
        body,
        head,
        END
};

struct InvSlot
{
        InvSlot(SlotId id_, std::string name_) :
                id(id_),
                name(name_),
                item(nullptr) {}

        InvSlot() :
                id(SlotId::wpn),
                name(""),
                item(nullptr) {}

        SlotId id;
        std::string name;
        item::Item* item;
};

class Inventory
{
public:
        Inventory(actor::Actor* const owning_actor);

        ~Inventory();

        void save() const;
        void load();

        // Equip item from backpack
        void equip_backpack_item(
                const size_t backpack_idx,
                const SlotId slot_id);

        void equip_backpack_item(
                const item::Item* const item,
                const SlotId slot_id);

        size_t unequip_slot(const SlotId id);

        // NOTE: The "put_in_*" functions should NEVER be called on items
        // already in the inventory. The purpose of these methods are to place
        // new/external items in the inventory
        void put_in_slot(
                const SlotId id,
                item::Item* item,
                Verbosity verbosity);

        void put_in_backpack(item::Item* item);

        void put_in_intrinsics(item::Item* item);

        void print_equip_message(
                const SlotId slot_id,
                const item::Item& item);

        void print_unequip_message(
                const SlotId slot_id,
                const item::Item& item);

        void drop_all_non_intrinsic(const P& pos);

        void swap_wielded_and_prepared();

        bool has_item_in_slot(SlotId id) const;

        bool has_ammo_for_firearm_in_inventory() const;

        item::Item* item_in_backpack(const item::Id id) const;

        int backpack_idx(const item::Id item_id) const;

        item::Item* item_in_slot(const SlotId id) const;

        // All "decr_..." functions which operates on a single item returns the
        // decremented item stack if it still exists, otherwise returns nullptr
        item::Item* decr_item_in_slot(SlotId slot_id);
        item::Item* decr_item_in_backpack(const size_t idx);
        item::Item* decr_item(item::Item* const item);

        void decr_item_type_in_backpack(const item::Id item_id);

        item::Item* remove_item(
                item::Item* const item,
                const bool delete_item);

        item::Item* remove_item_in_slot(
                const SlotId slot_id,
                const bool delete_item);

        item::Item* remove_item_in_backpack_with_idx(
                const size_t idx,
                const bool delete_item);

        item::Item* remove_item_in_backpack_with_ptr(
                item::Item* const item,
                const bool delete_item);

        int intrinsics_size() const
        {
                return m_intrinsics.size();
        }

        item::Item* intrinsic_in_element(const int idx) const;

        bool has_item_in_backpack(const item::Id id) const;

        int item_stack_size_in_backpack(const item::Id id) const;

        void sort_backpack();

        int total_item_weight() const;

        InvSlot m_slots[(size_t)SlotId::END];

        std::vector<item::Item*> m_backpack;

        std::vector<item::Item*> m_intrinsics;

private:
        // Puts the item in the slot, and prints messages
        void equip(
                const SlotId id,
                item::Item* const item,
                Verbosity verbosity);

        void equip_from_backpack(const SlotId id, const size_t backpack_idx);

        size_t move_from_slot_to_backpack(const SlotId id);

        // Checks if the item is stackable, and if so attempts to stack it with
        // another item of the same type in the backpack. The item pointer is
        // still valid if a stack occurs (it is the other item that gets
        // destroyed)
        bool try_stack_in_backpack(item::Item* item);

        actor::Actor* const m_owning_actor;
};

#endif // INVENTORY_HPP
