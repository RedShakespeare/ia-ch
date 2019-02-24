// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef TERRAIN_MONOLITH_HPP
#define TERRAIN_MONOLITH_HPP

#include "terrain.hpp"


namespace terrain
{

class Monolith: public Terrain
{
public:
    Monolith(const P& p);
    Monolith() = delete;
    ~Monolith() {}

    Id id() const override
    {
        return Id::monolith;
    }

    std::string name(const Article article) const override;

    void bump(actor::Actor& actor_bumping) override;

private:
    Color color_default() const override;

    void on_hit(const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

    void activate();

    bool m_is_activated;
};

} // terrain

#endif // TERRAIN_MONOLITH_HPP
