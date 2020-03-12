// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef EXPLOSION_HPP
#define EXPLOSION_HPP

#include <vector>

#include "audio.hpp"
#include "colors.hpp"
#include "global.hpp"
#include "rect.hpp"


class Prop;


enum class ExplType
{
        expl,
        apply_prop
};

enum class EmitExplSnd
{
        no,
        yes
};

enum class ExplExclCenter
{
        no,
        yes
};

enum class ExplIsGas
{
        no,
        yes
};


namespace explosion
{

// TODO: The signature of this function is really ugly! add a data struct
// instead to pass as config. Perhaps also a second convenience function which
// just runs a good old regular explosion.

// NOTE: If "emit_expl_sound" is set to "no", this typically means that the
// caller should emit a custom sound before running the explosion (e.g. molotov
// explosion sound).
void run(const P& origin,
         ExplType expl_type,
         EmitExplSnd emit_expl_snd = EmitExplSnd::yes,
         int radi_change = 0,
         ExplExclCenter exclude_center = ExplExclCenter::no,
         std::vector<Prop*> properties_applied = {},
         Color color_override = Color(),
         ExplIsGas is_gas = ExplIsGas::no);

void run_smoke_explosion_at(const P& origin, int radi_change = 0);

R explosion_area(const P& c, int radi);

} // namespace explosion

#endif
