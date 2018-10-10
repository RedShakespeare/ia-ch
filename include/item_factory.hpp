#ifndef ITEM_FACTORY_HPP
#define ITEM_FACTORY_HPP

#include <string>

#include "item.hpp"

namespace item_factory
{

Item* make(const ItemId item_id, const int nr_items = 1);

// TODO: Shouldn't this be a virtual function for the Item class? Something like
// "init_randomized()"?
void set_item_randomized_properties(Item* item);

Item* make_item_on_floor(const ItemId item_id, const P& pos);

Item* copy_item(const Item& item_to_copy);

} // item_factory

#endif
