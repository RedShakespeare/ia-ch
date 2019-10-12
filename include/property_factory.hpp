// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef PROPERTY_FACTORY_H
#define PROPERTY_FACTORY_H

#include "property_data.hpp"


class Prop;


namespace property_factory
{

Prop* make(const PropId id);

} // namespace property_factory

#endif // PROPERTY_FACTORY_H
