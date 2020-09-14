// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
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

        ItemAttProp(Prop* const property) :
                prop(property),
                pct_chance_to_apply(100) {}

        ~ItemAttProp() = default;

        std::shared_ptr<Prop> prop;

        int pct_chance_to_apply;
};

#endif  // ITEM_ATT_PROPERTY_HPP
