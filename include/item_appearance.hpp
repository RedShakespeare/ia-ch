// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ITEM_APPEARANCE_HPP
#define ITEM_APPEARANCE_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "debug.hpp"
#include "item_data.hpp"

namespace item
{

// Rebuild the fake-appearance text cached on each ItemData entry of the
// given type, using a deterministic pool rebuilt by `build_pool`. The
// caller-supplied `apply_un_id` writes the unidentified names (and color,
// if applicable) from the pool entry at d.session.fake_appearance_idx, then
// erases that entry from the pool so the next matching item takes the next slot.
// `apply_real_name` constructs the typed item and writes the identified
// names onto d.text.base_name.
//
// Used by scroll/potion/rod refresh_localized_text() to re-localize the cached
// appearance text without disturbing session state (the fake_appearance_idx
// is preserved across the rebuild, so the per-session item<->appearance
// mapping is stable).
template <typename PoolEntry>
void rebuild_fake_appearances(
    ItemType type,
    const std::function<void(std::vector<PoolEntry>&)>& build_pool,
    const std::function<void(ItemData& d, const PoolEntry& entry)>& apply_un_id,
    const std::function<void(ItemData& d)>& apply_real_name)
{
    TRACE_FUNC_BEGIN;

    std::vector<PoolEntry> pool;
    build_pool(pool);

    for (ItemData& d : g_data) {
        if (d.type != type) {
            continue;
        }

        const int idx = d.session.fake_appearance_idx;

        if ((idx < 0) || (idx >= (int)pool.size())) {
            continue;
        }

        apply_un_id(d, pool[(size_t)idx]);

        pool.erase(std::begin(pool) + idx);

        apply_real_name(d);
    }

    TRACE_FUNC_END;
}

}  // namespace item

#endif  // ITEM_APPEARANCE_HPP
