#include "inventory.hpp"

#include <algorithm>
#include <vector>

#include "init.hpp"
#include "item.hpp"
#include "drop.hpp"
#include "actor_player.hpp"
#include "msg_log.hpp"
#include "io.hpp"
#include "item_factory.hpp"
#include "create_character.hpp"
#include "game.hpp"
#include "player_bon.hpp"
#include "map.hpp"
#include "saving.hpp"

Inventory::Inventory(Actor* const owning_actor) :
        owning_actor_(owning_actor)
{
        auto set_slot = [&](const SlotId id, const std::string & name) {
                slots[(size_t)id] = {id, name};
        };

        set_slot(SlotId::wpn, "Wielded");
        set_slot(SlotId::wpn_alt, "Prepared");
        set_slot(SlotId::body, "Body");
        set_slot(SlotId::head, "Head");
}

Inventory::~Inventory()
{
        for (size_t i = 0; i < (size_t)SlotId::END; ++i)
        {
                auto& slot = slots[i];

                if (slot.item)
                {
                        delete slot.item;
                }
        }

        for (Item* item : backpack)
        {
                delete item;
        }

        for (Item* item : intrinsics)
        {
                delete item;
        }
}

void Inventory::save() const
{
        for (const InvSlot& slot : slots)
        {
                Item* const item = slot.item;

                if (item)
                {
                        saving::put_int((int)item->id());
                        saving::put_int(item->nr_items_);

                        item->save();
                }
                else // No item in this slot
                {
                        saving::put_int((int)ItemId::END);
                }
        }

        saving::put_int(backpack.size());

        for (size_t i = 0; i < backpack.size(); ++i)
        {
                const auto* const item = backpack[i];

                saving::put_int(int(item->id()));
                saving::put_int(item->nr_items_);

                item->save();
        }
}

void Inventory::load()
{
        for (InvSlot& slot : slots)
        {
                // Any previous item is destroyed
                Item* item = slot.item;

                delete item;

                slot.item = nullptr;

                const ItemId item_id = (ItemId)saving::get_int();

                if (item_id != ItemId::END)
                {
                        item = item_factory::make(item_id);

                        item->nr_items_ = saving::get_int();

                        item->load();

                        slot.item = item;

                        // "Wear" the item to apply properties from wearing and
                        // set owning actor
                        ASSERT(owning_actor_);

                        item->on_pickup(*owning_actor_);

                        item->on_equip(Verbosity::silent);
                }
        }

        while (!backpack.empty())
        {
                remove_item_in_backpack_with_idx(0, true);
        }

        const int backpack_size = saving::get_int();

        for (int i = 0; i < backpack_size; ++i)
        {
                const ItemId id = (ItemId)saving::get_int();

                Item* item = item_factory::make(id);

                item->nr_items_ = saving::get_int();

                item->load();

                backpack.push_back(item);

                // Pick up the item again to set owning actor
                ASSERT(owning_actor_);

                item->on_pickup(*owning_actor_);
        }
}

bool Inventory::has_item_in_backpack(const ItemId id) const
{
        auto it = std::find_if(
                begin(backpack),
                end(backpack),
                [id](Item * item)
        {
                return item->id() == id;
        });

        return it != end(backpack);
}

int Inventory::item_stack_size_in_backpack(const ItemId id) const
{
        for (size_t i = 0; i < backpack.size(); ++i)
        {
                if (backpack[i]->data().id == id)
                {
                        if (backpack[i]->data().is_stackable)
                        {
                                return backpack[i]->nr_items_;
                        }
                        else // Not stackable
                        {
                                return 1;
                        }
                }
        }

        return 0;
}

bool Inventory::try_stack_in_backpack(Item* item)
{
        // If item stacks, see if there are other items of same type
        if (!item->data().is_stackable)
        {
                return false;
        }

        for (size_t i = 0; i < backpack.size(); ++i)
        {
                Item* const other = backpack[i];

                if (other->id() != item->id())
                {
                        continue;
                }

                // NOTE: We are keeping the new item and destroying old one (so
                // the parameter pointer is still valid)
                item->nr_items_ += other->nr_items_;

                delete other;

                backpack[i] = item;

                if (owning_actor_->is_player() &&
                    (map::player->last_thrown_item_ == other))
                {
                        map::player->last_thrown_item_ = item;
                }

                return true;
        }

        return false;
}

void Inventory::put_in_backpack(Item* item)
{
        ASSERT(!item->actor_carrying());

        bool is_stacked = try_stack_in_backpack(item);

        if (!is_stacked)
        {
                backpack.push_back(item);

                sort_backpack();
        }

        // NOTE: This may destroy the item (e.g. combining with another item)
        item->on_pickup(*owning_actor_);
}

void Inventory::drop_all_non_intrinsic(const P& pos)
{
        TRACE_FUNC_BEGIN_VERBOSE;

        // Drop from slots
        for (InvSlot& slot : slots)
        {
                Item* const item = remove_item_in_slot(slot.id, false);

                if (item)
                {
                        item_drop::drop_item_on_map(pos, *item);
                }
        }

        // Drop from backpack
        while (!backpack.empty())
        {
                Item* const item = remove_item_in_backpack_with_idx(0, false);

                item_drop::drop_item_on_map(pos, *item);
        }

        TRACE_FUNC_END_VERBOSE;
}

bool Inventory::has_ammo_for_firearm_in_inventory() const
{
        Wpn* weapon = static_cast<Wpn*>(item_in_slot(SlotId::wpn));

        // If weapon found
        if (weapon)
        {
                // Should not happen
                ASSERT(!weapon->data().ranged.has_infinite_ammo);

                // If weapon is a firearm
                if (weapon->data().ranged.is_ranged_wpn)
                {
                        // Get weapon ammo type
                        const ItemId ammo_id =
                                weapon->data().ranged.ammo_item_id;

                        // Look for that ammo type in inventory
                        for (size_t i = 0; i < backpack.size(); ++i)
                        {
                                if (backpack[i]->data().id == ammo_id)
                                {
                                        return true;
                                }
                        }
                }
        }

        return false;
}

Item* Inventory::decr_item_in_slot(SlotId slot_id)
{
        auto& item = *item_in_slot(slot_id);

        auto delete_item = true;

        if (item.data().is_stackable)
        {
                --item.nr_items_;

                delete_item = item.nr_items_ <= 0;
        }

        if (delete_item)
        {
                remove_item_in_slot(slot_id, true);

                return nullptr;
        }
        else
        {
                return &item;
        }
}

Item* Inventory::remove_item_in_slot(
        const SlotId slot_id,
        const bool delete_item)
{
        ASSERT(slot_id != SlotId::END);

        auto& slot = slots[(size_t)slot_id];

        Item* item = slot.item;

        if (!item)
        {
                return nullptr;
        }

        slot.item = nullptr;

        item->on_unequip();

        item->on_removed_from_inv();

        if (delete_item)
        {
                delete item;

                item = nullptr;
        }

        return item;
}

Item* Inventory::remove_item_in_backpack_with_idx(
        const size_t idx,
        const bool delete_item)
{
        ASSERT(idx < backpack.size());

        Item* item = backpack[idx];

        if (owning_actor_->is_player() &&
            (item == map::player->last_thrown_item_))
        {
                map::player->last_thrown_item_ = nullptr;
        }

        backpack.erase(begin(backpack) + idx);

        item->on_removed_from_inv();

        if (delete_item)
        {
                delete item;

                item = nullptr;
        }

        return item;
}

Item* Inventory::remove_item(Item* const item,
                             const bool delete_item)
{
        Item* item_returned = nullptr;

        for (InvSlot& slot : slots)
        {
                if (slot.item == item)
                {
                        item_returned = remove_item_in_slot(
                                slot.id, delete_item);

                        return item_returned;
                }
        }

        for (size_t i = 0; i < backpack.size(); ++i)
        {
                if (backpack[i] == item)
                {
                        item_returned = remove_item_in_backpack_with_idx(
                                i, delete_item);

                        return item_returned;
                }
        }

        return item_returned;
}

Item* Inventory::remove_item_in_backpack_with_ptr(
        Item* const item,
        const bool delete_item)
{
        for (size_t i = 0; i < backpack.size(); ++i)
        {
                const Item* const current_item = backpack[i];

                if (current_item == item)
                {
                        return remove_item_in_backpack_with_idx(i, delete_item);
                }
        }

        TRACE << "Parameter item not in backpack" << std::endl;
        ASSERT(false);

        return nullptr;
}

Item* Inventory::decr_item_in_backpack(const size_t idx)
{
        Item& item = *backpack[idx];

        bool delete_item = true;

        if (item.data().is_stackable)
        {
                --item.nr_items_;

                delete_item = item.nr_items_ <= 0;
        }

        if (delete_item)
        {
                remove_item_in_backpack_with_idx(idx, true);

                return nullptr;
        }
        else
        {
                return &item;
        }
}

void Inventory::decr_item_type_in_backpack(const ItemId id)
{
        for (size_t i = 0; i < backpack.size(); ++i)
        {
                if (backpack[i]->data().id == id)
                {
                        decr_item_in_backpack(i);
                }
        }
}

Item* Inventory::decr_item(Item* const item)
{
        for (InvSlot& slot : slots)
        {
                if (slot.item == item)
                {
                        auto* const item = decr_item_in_slot(slot.id);

                        return item;
                }
        }

        for (size_t i = 0; i < backpack.size(); ++i)
        {
                if (backpack[i] == item)
                {
                        auto* const item = decr_item_in_backpack(i);

                        return item;
                }
        }

        return nullptr;
}

size_t Inventory::move_from_slot_to_backpack(const SlotId id)
{
        ASSERT(id != SlotId::END);

        auto& slot = slots[(size_t)id];

        Item* const item = slot.item;

        if (item)
        {
                item->on_unequip();

                slot.item = nullptr;

                bool is_stacked = try_stack_in_backpack(item);

                if (!is_stacked)
                {
                        backpack.push_back(item);

                        sort_backpack();
                }
        }

        for (size_t i = 0; i < backpack.size(); ++i)
        {
                if (item == backpack[i])
                {
                        return i;
                }
        }

        ASSERT(false);

        return 0;
}

void Inventory::equip_backpack_item(const size_t backpack_idx,
                                    const SlotId slot_id)
{
        ASSERT(slot_id != SlotId::END);

        ASSERT(owning_actor_);

        const bool backpack_slot_exists = backpack_idx < backpack.size();

        ASSERT(backpack_slot_exists);

        // Robustness for release mode
        if (!backpack_slot_exists)
        {
                return;
        }

        Item* item = backpack[backpack_idx];

        backpack.erase(begin(backpack) + backpack_idx);

        equip(slot_id,
              item,
              Verbosity::verbose);

        sort_backpack();
}

void Inventory::equip_backpack_item(const Item* const item,
                                    const SlotId slot_id)
{
        for (size_t i = 0; i < backpack.size(); ++i)
        {
                if (backpack[i] == item)
                {
                        equip_backpack_item(i, slot_id);
                }
        }

        ASSERT(false);
}

size_t Inventory::unequip_slot(const SlotId id)
{
        auto& slot = slots[(size_t)id];

        Item* item = slot.item;

        ASSERT(item);

        const size_t item_backpack_idx = move_from_slot_to_backpack(slot.id);

        if (owning_actor_->is_player())
        {
                print_unequip_message(slot.id, *item);
        }

        return item_backpack_idx;
}

void Inventory::swap_wielded_and_prepared()
{
        auto& slot1 = slots[(size_t)SlotId::wpn];
        auto& slot2 = slots[(size_t)SlotId::wpn_alt];

        Item* item1 = slot1.item;
        Item* item2 = slot2.item;

        slot1.item = item2;
        slot2.item = item1;
}

bool Inventory::has_item_in_slot(SlotId id) const
{
        ASSERT(id != SlotId::END && "Illegal slot id");
        return slots[int(id)].item;
}

Item* Inventory::item_in_backpack(const ItemId id) const
{
        auto it = std::find_if(
                begin(backpack),
                end(backpack),
                [id](Item * item)
        {
                return item->id() == id;
        });

        if (it == end(backpack))
        {
                return nullptr;
        }

        return *it;
}

int Inventory::backpack_idx(const ItemId id) const
{
        for (size_t i = 0; i < backpack.size(); ++i)
        {
                if (backpack[i]->id() == id)
                {
                        return i;
                }
        }

        return -1;
}

Item* Inventory::item_in_slot(SlotId id) const
{
        ASSERT(id != SlotId::END);

        return slots[(size_t)id].item;
}

Item* Inventory::intrinsic_in_element(int idx) const
{
        if (intrinsics_size() > idx)
        {
                return intrinsics[idx];
        }

        return nullptr;
}

void Inventory::put_in_intrinsics(Item* item)
{
        ASSERT(item->data().type == ItemType::melee_wpn_intr ||
               item->data().type == ItemType::ranged_wpn_intr);

        intrinsics.push_back(item);

        item->on_pickup(*owning_actor_);
}

void Inventory::equip(const SlotId id, Item* const item, Verbosity verbosity)
{
        ASSERT(id != SlotId::END);

        InvSlot* slot = nullptr;

        for (InvSlot& current_slot : slots)
        {
                if (current_slot.id == id)
                {
                        slot = &current_slot;
                }
        }

        ASSERT(slot);

        // Robustness for release mode
        if (!slot)
        {
                return;
        }

        // The slot should not already be occupied
        ASSERT(!slot->item);

        // Robustness for release mode
        if (slot->item)
        {
                return;
        }

        slot->item = item;

        if (owning_actor_->is_player() &&
            verbosity == Verbosity::verbose)
        {
                print_equip_message(id, *item);
        }

        item->on_equip(verbosity);
}

void Inventory::put_in_slot(const SlotId id,
                            Item* item,
                            Verbosity verbosity)
{
        item->on_pickup(*owning_actor_);

        equip(id, item, verbosity);
}

void Inventory::print_equip_message(
        const SlotId slot_id,
        const Item& item)
{
        const std::string name = item.name(ItemRefType::plural);

        std::string msg = "";

        switch (slot_id)
        {
        case SlotId::wpn:
                msg = "I am now wielding " + name + ".";
                break;

        case SlotId::wpn_alt:
                msg =
                        "I am now using " +
                        name +
                        " as a prepared weapon.";
                break;

        case SlotId::body:
                msg = "I am now wearing " + name + ".";
                break;

        case SlotId::head:
                msg = "I am now wearing " + name + ".";
                break;

        case SlotId::END: {}
                break;
        }

        msg_log::add(
                msg,
                colors::text(),
                false,
                MorePromptOnMsg::no);
}

void Inventory::print_unequip_message(
        const SlotId slot_id,
        const Item& item)
{
        // The message should be of the form "... my [item]" - we never
        // want the name to be "A [item]". Therefore we use plural form
        // for stacks, and plain form for single items.
        const auto item_ref_type =
                (item.nr_items_ > 1)
                ? ItemRefType::plural
                : ItemRefType::plain;

        const std::string name = item.name(item_ref_type);

        std::string msg = "";

        switch (slot_id)
        {
        case SlotId::wpn:
                msg = "I put away my " + name + ".";
                break;

        case SlotId::wpn_alt:
                msg = "I put away my " + name + ".";
                break;

        case SlotId::body:
                msg = "I have taken off my " + name + ".";
                break;

        case SlotId::head:
                msg = "I have taken off my " + name + ".";
                break;

        case SlotId::END:
                break;
        }

        msg_log::add(msg,
                     colors::text(),
                     false,
                     MorePromptOnMsg::no);
}

int Inventory::total_item_weight() const
{
        int weight = 0;

        for (const auto& slot : slots)
        {
                if (slot.item)
                {
                        weight += slot.item->weight();
                }
        }

        for (const auto item : backpack)
        {
                weight += item->weight();
        }

        return weight;
}

// Function for lexicographically comparing two items
struct LexicograhicalCompareItems
{
        bool operator()(const Item* const item1, const Item* const item2)
        {
                const std::string& item_name1 = item1->name(ItemRefType::plain);
                const std::string& item_name2 = item2->name(ItemRefType::plain);

                return lexicographical_compare(
                        item_name1.begin(),
                        item_name1.end(),
                        item_name2.begin(),
                        item_name2.end());
        }
};

void Inventory::sort_backpack()
{
        LexicograhicalCompareItems lex_cmp;

        // First, take out prioritized items
        std::vector<Item*> prio_items;

        for (auto it = begin(backpack); it != end(backpack); )
        {
                Item* const item = *it;

                if (item->data().is_prio_in_backpack_list)
                {
                        prio_items.push_back(item);

                        it = backpack.erase(it);
                }
                else // Item not prioritized
                {
                        ++it;
                }
        }

        // Sort the prioritized items lexicographically
        if (!prio_items.empty())
        {
                std::sort(begin(prio_items), end(prio_items), lex_cmp);
        }

        // Categorize the remaining items
        std::vector< std::vector<Item*> > sort_buffer;

        // Sort according to item interface color first
        for (Item* item : backpack)
        {
                bool is_added_to_buffer = false;

                // Check if item should be added to any existing color group
                for (auto& group : sort_buffer)
                {
                        const Color color_current_group =
                                group[0]->interface_color();

                        if (item->interface_color() == color_current_group)
                        {
                                group.push_back(item);

                                is_added_to_buffer = true;

                                break;
                        }
                }

                if (is_added_to_buffer)
                {
                        continue;
                }

                // Item is a new color, create a new color group
                std::vector<Item*> new_group;
                new_group.push_back(item);
                sort_buffer.push_back(new_group);
        }

        // Sort lexicographically secondarily
        for (auto& group : sort_buffer)
        {
                std::sort(begin(group), end(group), lex_cmp);
        }

        // Add the sorted items to the backpack
        // NOTE: prio_items may be empty
        backpack = prio_items;

        for (size_t i = 0; i < sort_buffer.size(); ++i)
        {
                for (size_t ii = 0; ii < sort_buffer[i].size(); ii++)
                {
                        backpack.push_back(sort_buffer[i][ii]);
                }
        }
}
