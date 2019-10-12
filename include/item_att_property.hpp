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
        ItemAttProp() = default;

        explicit ItemAttProp(Prop* const property) :
                prop(property) {}

        ~ItemAttProp() = default;

        std::shared_ptr<Prop> prop {nullptr};

        int pct_chance_to_apply {100};
};

#endif // ITEM_ATT_PROPERTY_HPP
