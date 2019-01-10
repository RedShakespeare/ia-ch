// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef PLAYER_SPELLS_HPP
#define PLAYER_SPELLS_HPP

#include <vector>

#include "spells.hpp"
#include "state.hpp"
#include "browser.hpp"

class Spell;

namespace player_spells
{

void init();
void cleanup();

void save();
void load();

void learn_spell(const SpellId id, const Verbosity verbosity);

void incr_spell_skill(const SpellId id);

SpellSkill spell_skill(const SpellId id);

void set_spell_skill(const SpellId id, const SpellSkill val);

bool is_spell_learned(const SpellId id);

} // player_spells

class BrowseSpell: public State
{
public:
        BrowseSpell() :
                State() {}

        void on_start() override;

        void draw() override;

        void update() override;

        StateId id() override;

private:
        MenuBrowser browser_ {};
};

#endif // PLAYER_SPELLS_HPP
