// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ITEM_ATT_PROPERTY_HPP
#define ITEM_ATT_PROPERTY_HPP

#include <memory>

#include "property.hpp"

struct ItemAttProp
{
        ItemAttProp() :
                prop(nullptr),
                pct_chance_to_apply(100) {}

        ItemAttProp(Prop* const prop) :
                prop(prop),
                pct_chance_to_apply(100) {}

        ~ItemAttProp() {}

        std::shared_ptr<Prop> prop;

        int pct_chance_to_apply;
};

#endif // ITEM_ATT_PROPERTY_HPP
