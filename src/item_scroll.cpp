#include "item_scroll.hpp"

#include <string>

#include "init.hpp"
#include "actor_player.hpp"
#include "map.hpp"
#include "player_bon.hpp"
#include "msg_log.hpp"
#include "inventory.hpp"
#include "player_spells.hpp"
#include "io.hpp"
#include "item_factory.hpp"
#include "saving.hpp"
#include "game.hpp"
#include "property_handler.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
std::vector<std::string> false_names_;

static SpellSkill get_player_skill_for_scroll(SpellId spell_id)
{
        const auto player_spell_skill = map::player->spell_skill(spell_id);

        return (SpellSkill)std::max(
                player_spell_skill,
                SpellSkill::expert);
}

// -----------------------------------------------------------------------------
// Scroll
// -----------------------------------------------------------------------------
const std::string Scroll::real_name() const
{
        Spell* spell =
                spell_factory::make_spell_from_id(
                        data_->spell_cast_from_scroll);

        const std::string scroll_name = spell->name();

        delete spell;

        return scroll_name;
}

std::vector<std::string> Scroll::descr() const
{
        if (data_->is_identified)
        {
                const std::unique_ptr<Spell> spell(make_spell());

                const auto skill = get_player_skill_for_scroll(spell->id());

                const auto descr = spell->descr(skill, SpellSrc::manuscript);

                return descr;
        }
        else // Not identified
        {
                return data_->base_descr;
        }
}

ConsumeItem Scroll::activate(Actor* const actor)
{
        TRACE_FUNC_BEGIN;

        auto& properties = actor->properties();

        // Check properties which NEVER allows reading or speaking
        if (!properties.allow_read_absolute(Verbosity::verbose) ||
            !properties.allow_speak(Verbosity::verbose))
        {
                return ConsumeItem::no;
        }

        const P& player_pos(map::player->pos);

        if (map::dark.at(player_pos) && !map::light.at(player_pos))
        {
                msg_log::add("It's too dark to read here.");

                TRACE_FUNC_END;

                return ConsumeItem::no;
        }

        // OK, we can try to cast

        const bool is_identified_before = data_->is_identified;

        if (is_identified_before)
        {
                const std::string scroll_name =
                        name(ItemRefType::a, ItemRefInf::none);

                msg_log::add("I read " + scroll_name + "...");
        }
        else // Not already identified
        {
                msg_log::add(
                        "I recite the forbidden incantations on the "
                        "manuscript...");

                data_->is_tried = true;
        }

        const std::string crumble_str = "The Manuscript crumbles to dust.";

        // Check properties which MAY allow reading, with a random chance
        if (!properties.allow_read_chance(Verbosity::verbose))
        {
                msg_log::add(crumble_str);

                TRACE_FUNC_END;

                return ConsumeItem::yes;
        }

        // OK, we are fully allowed to read the scroll - cast the spell

        const std::unique_ptr<Spell> spell(make_spell());

        const SpellId id = spell->id();

        const auto skill = get_player_skill_for_scroll(id);

        spell->cast(
                map::player,
                skill,
                SpellSrc::manuscript);

        msg_log::add(crumble_str);

        identify(Verbosity::verbose);

        // Learn spell
        if (spell->player_can_learn())
        {
                player_spells::learn_spell(id, Verbosity::verbose);
        }

        TRACE_FUNC_END;

        return ConsumeItem::yes;
}

Spell* Scroll::make_spell() const
{
        return spell_factory::make_spell_from_id(
                data_->spell_cast_from_scroll);
}

void Scroll::identify(const Verbosity verbosity)
{
        if (data_->is_identified)
        {
                return;
        }

        data_->is_identified = true;

        if (verbosity == Verbosity::verbose)
        {
                const std::string name_after =
                        name(ItemRefType::a, ItemRefInf::none);

                msg_log::add("I have identified " + name_after + ".");

                game::add_history_event("Identified " + name_after + ".");
        }
}

std::string Scroll::name_inf_str() const
{
        return (data_->is_tried && !data_->is_identified) ? "{Tried}" : "";
}

namespace scroll_handling
{

void init()
{
        TRACE_FUNC_BEGIN;

        // Init possible fake names
        false_names_.clear();
        false_names_.push_back("Cruensseasrjit");
        false_names_.push_back("Rudsceleratus");
        false_names_.push_back("Rudminuox");
        false_names_.push_back("Cruo stragara-na");
        false_names_.push_back("Praya navita");
        false_names_.push_back("Pretia Cruento");
        false_names_.push_back("Pestis Cruento");
        false_names_.push_back("Cruento Pestis");
        false_names_.push_back("Domus-bhaava");
        false_names_.push_back("Acerbus-shatruex");
        false_names_.push_back("Pretaanluxis");
        false_names_.push_back("Praansilenux");
        false_names_.push_back("Quodpipax");
        false_names_.push_back("Lokemundux");
        false_names_.push_back("Profanuxes");
        false_names_.push_back("Shaantitus");
        false_names_.push_back("Geropayati");
        false_names_.push_back("Vilomaxus");
        false_names_.push_back("Bhuudesco");
        false_names_.push_back("Durbentia");
        false_names_.push_back("Bhuuesco");
        false_names_.push_back("Maravita");
        false_names_.push_back("Infirmux");

        std::vector<std::string> cmb;
        cmb.clear();
        cmb.push_back("Cruo");
        cmb.push_back("Cruonit");
        cmb.push_back("Cruentu");
        cmb.push_back("Marana");
        cmb.push_back("Domus");
        cmb.push_back("Malax");
        cmb.push_back("Caecux");
        cmb.push_back("Eximha");
        cmb.push_back("Vorox");
        cmb.push_back("Bibox");
        cmb.push_back("Pallex");
        cmb.push_back("Profanx");
        cmb.push_back("Invisuu");
        cmb.push_back("Invisux");
        cmb.push_back("Odiosuu");
        cmb.push_back("Odiosux");
        cmb.push_back("Vigra");
        cmb.push_back("Crudux");
        cmb.push_back("Desco");
        cmb.push_back("Esco");
        cmb.push_back("Gero");
        cmb.push_back("Klaatu");
        cmb.push_back("Barada");
        cmb.push_back("Nikto");

        const size_t nr_cmb_parts = cmb.size();

        for (size_t i = 0; i < nr_cmb_parts; ++i)
        {
                for (size_t ii = 0; ii < nr_cmb_parts; ii++)
                {
                        if (i != ii)
                        {
                                false_names_.push_back(cmb[i] + " " + cmb[ii]);
                        }
                }
        }

        for (auto& d : item_data::data)
        {
                if (d.type == ItemType::scroll)
                {
                        // False name
                        const size_t idx = rnd::idx(false_names_);

                        const std::string& title = false_names_[idx];

                        d.base_name_un_id.names[(size_t)ItemRefType::plain] =
                                "Manuscript titled " + title;

                        d.base_name_un_id.names[(size_t)ItemRefType::plural] =
                                "Manuscripts titled " + title;

                        d.base_name_un_id.names[(size_t)ItemRefType::a] =
                                "a Manuscript titled " + title;

                        false_names_.erase(false_names_.begin() + idx);

                        // True name
                        const Scroll* const scroll =
                                static_cast<const Scroll*>(
                                        item_factory::make(d.id, 1));

                        const std::string real_type_name = scroll->real_name();

                        delete scroll;

                        const std::string real_name =
                                "Manuscript of " + real_type_name;

                        const std::string real_name_plural =
                                "Manuscripts of " + real_type_name;

                        const std::string real_name_a =
                                "a Manuscript of " + real_type_name;

                        d.base_name.names[(size_t)ItemRefType::plain] =
                                real_name;

                        d.base_name.names[(size_t)ItemRefType::plural] =
                                real_name_plural;

                        d.base_name.names[(size_t)ItemRefType::a] =
                                real_name_a;
                }
        }

        TRACE_FUNC_END;
}

void save()
{
        for (size_t i = 0; i < (size_t)ItemId::END; ++i)
        {
                if (item_data::data[i].type != ItemType::scroll)
                {
                        continue;
                }

                auto& names = item_data::data[i].base_name_un_id.names;

                saving::put_str(names[(size_t)ItemRefType::plain]);
                saving::put_str(names[(size_t)ItemRefType::plural]);
                saving::put_str(names[(size_t)ItemRefType::a]);
        }
}

void load()
{
        for (size_t i = 0; i < (size_t)ItemId::END; ++i)
        {
                if (item_data::data[i].type != ItemType::scroll)
                {
                        continue;
                }

                auto& names = item_data::data[i].base_name_un_id.names;

                names[(size_t)ItemRefType::plain] = saving::get_str();
                names[(size_t)ItemRefType::plural] = saving::get_str();
                names[(size_t)ItemRefType::a] = saving::get_str();
        }
}

} // scroll_handling
