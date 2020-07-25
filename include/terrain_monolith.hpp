// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef TERRAIN_MONOLITH_HPP
#define TERRAIN_MONOLITH_HPP

#include "terrain.hpp"

namespace terrain {

class Monolith : public Terrain {
public:
        Monolith(const P& p);
        Monolith() = delete;
        ~Monolith() = default;

        Id id() const override
        {
                return Id::monolith;
        }

        std::string name(Article article) const override;

        void bump(actor::Actor& actor_bumping) override;

private:
        Color color_default() const override;

        void on_hit(
                DmgType dmg_type,
                actor::Actor* actor,
                int dmg = -1) override;

        void activate();

        bool m_is_activated;
};

} // namespace terrain

#endif // TERRAIN_MONOLITH_HPP
