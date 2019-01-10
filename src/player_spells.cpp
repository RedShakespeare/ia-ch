// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "player_spells.hpp"

#include <vector>
#include <algorithm>

#include "actor_player.hpp"
#include "browser.hpp"
#include "common_text.hpp"
#include "init.hpp"
#include "inventory.hpp"
#include "io.hpp"
#include "item_factory.hpp"
#include "item_scroll.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "panel.hpp"
#include "player_bon.hpp"
#include "property_handler.hpp"
#include "query.hpp"
#include "saving.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static std::vector<Spell*> learned_spells_;

static SpellSkill spell_skills_[(size_t)SpellId::END];

static void try_cast(Spell* const spell)
{
        const auto& props = map::player->properties;

        bool allow_cast =
                props.allow_cast_intr_spell_absolute(
                        Verbosity::verbose);

        if (allow_cast)
        {
                allow_cast =
                        allow_cast &&
                        props.allow_speak(Verbosity::verbose);
        }

        if (!allow_cast)
        {
                return;
        }

        msg_log::clear();

        const SpellSkill skill = map::player->spell_skill(spell->id());

        const Range spi_cost_range = spell->spi_cost(skill, map::player);

        if (spi_cost_range.max >= map::player->sp)
        {
                msg_log::add(
                        std::string(
                                "Low spirit, try casting spell anyway? " +
                                common_text::yes_or_no_hint),
                        colors::light_white());

                if (query::yes_or_no() == BinaryAnswer::no)
                {
                        msg_log::clear();

                        return;
                }

                msg_log::clear();
        }

        msg_log::add("I cast " + spell->name() + "!");

        if (map::player->is_alive())
        {
                spell->cast(map::player, skill, SpellSrc::learned);
        }
}

// -----------------------------------------------------------------------------
// player_spells
// -----------------------------------------------------------------------------
namespace player_spells
{

void init()
{
        cleanup();
}

void cleanup()
{
        for (Spell* spell : learned_spells_)
        {
                delete spell;
        }

        learned_spells_.clear();

        for (size_t i = 0; i < (size_t)SpellId::END; ++i)
        {
                spell_skills_[i] = (SpellSkill)0;
        }
}

void save()
{
        saving::put_int(learned_spells_.size());

        for (Spell* s : learned_spells_)
        {
                saving::put_int((int)s->id());
        }

        for (size_t i = 0; i < (size_t)SpellId::END; ++i)
        {
                saving::put_int((int)spell_skills_[i]);
        }
}

void load()
{
        const int nr_spells = saving::get_int();

        for (int i = 0; i < nr_spells; ++i)
        {
                const SpellId id = (SpellId)saving::get_int();

                learned_spells_.push_back(
                        spell_factory::make_spell_from_id(id));
        }

        for (size_t i = 0; i < (size_t)SpellId::END; ++i)
        {
                spell_skills_[i] = (SpellSkill)saving::get_int();
        }
}

bool is_spell_learned(const SpellId id)
{
        for (auto* s : learned_spells_)
        {
                if (s->id() == id)
                {
                        return true;
                }
        }

        return false;
}

void learn_spell(const SpellId id, const Verbosity verbosity)
{
        if (is_spell_learned(id))
        {
                // Spell already known
                return;
        }

        Spell* const spell = spell_factory::make_spell_from_id(id);

        const bool player_can_learn = spell->player_can_learn();

        ASSERT(player_can_learn);

        // Robustness for release mode
        if (!player_can_learn)
        {
                return;
        }

        if (verbosity == Verbosity::verbose)
        {
                msg_log::add("I can now cast this incantation from memory.");
        }

        learned_spells_.push_back(spell);
}

void incr_spell_skill(const SpellId id)
{
        TRACE << "Increasing spell skill for spell id: "
              << (int)id
              << std::endl;

        auto& skill = spell_skills_[(size_t)id];

        TRACE << "skill before: " << (int)skill << std::endl;

        if (skill == SpellSkill::basic)
        {
                skill = SpellSkill::expert;
        }
        else if (skill == SpellSkill::expert)
        {
                skill = SpellSkill::master;
        }

        TRACE << "skill after: " << (int)skill << std::endl;
}

SpellSkill spell_skill(const SpellId id)
{
        ASSERT(id != SpellId::END);

        if (id == SpellId::END)
        {
                return SpellSkill::basic;
        }

        return spell_skills_[(size_t)id];
}

void set_spell_skill(const SpellId id, const SpellSkill val)
{
        ASSERT(id != SpellId::END);

        if (id == SpellId::END)
        {
                return;
        }

        spell_skills_[(size_t)id] = val;
}

} // player_spells

// -----------------------------------------------------------------------------
// BrowseSpell
// -----------------------------------------------------------------------------
void BrowseSpell::on_start()
{
        if (learned_spells_.empty())
        {
                // Exit screen
                states::pop();

                msg_log::add("I do not know any spells.");
                return;
        }

        browser_.reset(learned_spells_.size());
}

void BrowseSpell::draw()
{
        const int nr_spells = learned_spells_.size();

        io::draw_text_center(
                "Use which power?",
                Panel::screen,
                P(panels::center_x(Panel::screen), 0),
                colors::title());

        P p(0, 1);

        std::string key_str = "a) ";

        for (int i = 0; i < nr_spells; ++i)
        {
                const int current_idx = i;

                const bool is_idx_marked = browser_.is_at_idx(current_idx);

                auto* const spell = learned_spells_[i];

                const auto name = spell->name();

                const int spi_label_x = 24;

                const int skill_label_x = spi_label_x + 10;

                p.x = 0;

                const Color color =
                        is_idx_marked
                        ? colors::menu_highlight()
                        : colors::menu_dark();

                io::draw_text(
                        key_str,
                        Panel::item_menu,
                        p,
                        color);

                ++key_str[0];

                p.x = key_str.size();

                io::draw_text(
                        name,
                        Panel::item_menu,
                        p,
                        color);

                std::string fill_str = "";

                const size_t fill_size = spi_label_x - p.x - name.size();

                for (size_t ii = 0; ii < fill_size; ++ii)
                {
                        fill_str.push_back('.');
                }

                const Color fill_color = colors::gray().fraction(3.0);

                io::draw_text(
                        fill_str,
                        Panel::item_menu,
                        P(p.x + name.size(), p.y),
                        fill_color);

                p.x = spi_label_x;

                std::string str = "SP: ";

                io::draw_text(
                        str,
                        Panel::item_menu,
                        p,
                        colors::dark_gray());

                p.x += str.size();

                const SpellId id = spell->id();

                const auto skill = player_spells::spell_skill(id);

                const Range spi_cost = spell->spi_cost(skill, map::player);

                const std::string lower_str = std::to_string(spi_cost.min);
                const std::string upper_str = std::to_string(spi_cost.max);

                str =
                        (spi_cost.max == 1)
                        ? "1"
                        : (lower_str +  "-" + upper_str);

                io::draw_text(
                        str,
                        Panel::item_menu,
                        p,
                        colors::white());

                if (spell->can_be_improved_with_skill())
                {
                        p.x = skill_label_x;

                        str = "Skill: ";

                        io::draw_text(
                                str,
                                Panel::item_menu,
                                p,
                                colors::dark_gray());

                        p.x += str.size();

                        switch (skill)
                        {
                        case SpellSkill::basic:
                                str = "I";
                                break;

                        case SpellSkill::expert:
                                str = "II";
                                break;

                        case SpellSkill::master:
                                str = "III";
                                break;
                        }

                        io::draw_text(
                                str,
                                Panel::item_menu,
                                p,
                                colors::white());
                }

                if (is_idx_marked)
                {
                        const auto skill = map::player->spell_skill(id);

                        const auto descr =
                                spell->descr(skill, SpellSrc::learned);

                        std::vector<ColoredString> lines;

                        for (const auto& line : descr)
                        {
                                lines.push_back(
                                        ColoredString(
                                                line,
                                                colors::light_white()));
                        }

                        if (!lines.empty())
                        {
                                io::draw_descr_box(lines);
                        }
                }

                ++p.y;
        }
}

void BrowseSpell::update()
{
        auto input = io::get();

        const MenuAction action =
                browser_.read(
                        input,
                        MenuInputMode::scrolling_and_letters);

        switch (action)
        {
        case MenuAction::selected:
        {
                auto* const spell = learned_spells_[browser_.y()];

                // Exit screen
                states::pop();

                try_cast(spell);

                return;
        }
        break;

        case MenuAction::esc:
        case MenuAction::space:
        {
                // Exit screen
                states::pop();
                return;
        }
        break;

        default:
                break;
        }
}
