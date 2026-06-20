// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "hints.hpp"

#include <cstring>
#include <string>
#include <utility>

#include "actor.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "i18n.hpp"
#include "io.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "popup.hpp"
#include "saving.hpp"
#include "state.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static bool s_hints_displayed[(size_t)hints::Id::END];

static std::pair<std::string, std::string> id_to_text(const hints::Id id)
{
    switch (id) {
    case hints::Id::altars:
        return {
            i18n::get("hints.altars.title", "Altars"),
            i18n::get(
                "hints.altars.body",
                "All spells are cast at a higher level when standing "
                "at an altar - this includes both spells cast from "
                "manuscripts and from memory.")};

    case hints::Id::fountains:
        return {
            i18n::get("hints.fountains.title", "Fountains"),
            i18n::get(
                "hints.fountains.body",
                "Drinking from a fountain usually restores a bit of "
                "health, spirit, and mental shock (but they can sometimes "
                "have other effects, both good and bad!). Fountains "
                "can be drunk from several times, but each time there "
                "is a chance that it will dry up permanently.")};

    case hints::Id::destroying_corpses:
        return {
            i18n::get("hints.destroying_corpses.title", "Destroying corpses"),
            i18n::get(
                "hints.destroying_corpses.body",
                "Corpses can be destroyed by pressing [k] or [w]. This "
                "can be very useful against certain types of monsters. "
                "Some weapons, such as Machetes, makes it easier to "
                "destroy corpses - check the item description to see "
                "if a weapon has such a bonus. Also, a well-placed "
                "stick of dynamite or Molotov Cocktail is usually an "
                "effective way of stopping persistent monsters.")};

    case hints::Id::unload_weapons:
        return {
            i18n::get("hints.unload_weapons.title", "Unloading weapons"),
            i18n::get(
                "hints.unload_weapons.body",
                "Ammunition can be unloaded from firearms on the "
                "ground by pressing [u] or [G].")};

    case hints::Id::infected:
        return {
            i18n::get("hints.infected.title", "Infected"),
            i18n::get(
                "hints.infected.body",
                "Infections should be treated as soon as possible. "
                "The common way of doing this is by using the "
                "Medical Bag. It only requires a small number of turns "
                "and resources, but if the work is interrupted, the "
                "effort is wasted (no medical resources are lost "
                "on interruption however)."

                "\n\nAn untreated infection will eventually turn into a "
                "disease (50% maximum hit points), "
                "which can only be removed through special means such as "
                "drinking certain potions.")};

    case hints::Id::overburdened:
        return {
            i18n::get("hints.overburdened.title", "Overburdened"),
            i18n::get(
                "hints.overburdened.body",
                "Carrying too much weight makes movement take twice "
                "as much time. This is a very dangerous and "
                "detrimental situation.")};

    case hints::Id::high_shock:
        return {
            i18n::get("hints.high_shock.title", "High shock"),
            i18n::get(
                "hints.high_shock.body",
                "Being in a state of extreme mental shock (stress, paranoia) "
                "will cause a sanity hit. One way to reduce shock, "
                "and thereby avoiding or prolonging the sanity hit, "
                "is to find a source of light - for example through "
                "activating an Electric Lantern or igniting a Flare.")};

    case hints::Id::status_effects:
        return {
            i18n::get("hints.status_effects.title", "Status effects"),
            i18n::get(
                "hints.status_effects.body",
                "A status effect has been applied. "
                "Status effects are various positive, negative or neutral effects "
                "applied on a creature. "
                "Some examples are confusion, burning, invisibility, or "
                "electricity resistance. "
                "A simple list of active status effects is shown in the normal "
                "game screen. "

                "\n\nIn the character screen (accessed by pressing [C] or [@]), a more "
                "detailed list can be seen, including a description of each effect. "

                "\n\nStatus effects shown with CAPITAL LETTERS are \"permanent\", "
                "and are only removed if some special action is taken, for example "
                "using the medical bag to treat a wound.")};

    case hints::Id::study_inscription:
        return {
            i18n::get("hints.study_inscription.title", "Inscriptions"),
            i18n::get(
                "hints.study_inscription.body",
                "There is an inscription here, studying it will yield some experience."

                "\n\nIt may also recall a spell that you have forgotten, "
                "or reveal something about carried manuscripts or potions. "
                "The chance to reveal information about such items is higher with "
                "more unknown items carried.")};

    case hints::Id::kick_brazier:
        return {
            i18n::get("hints.kick_brazier.title", "Kicking braziers"),
            i18n::get(
                "hints.kick_brazier.body",
                "Braziers can be kicked over to set creatures on fire "
                "in a small area.")};

    case hints::Id::kick_statue:
        return {
            i18n::get("hints.kick_statue.title", "Kicking statues"),
            i18n::get(
                "hints.kick_statue.body",
                "Statues can be kicked over to "
                "damage and stun a creature on the other side.")};

    case hints::Id::temporary_and_permanent_shock:
        return {
            i18n::get(
                "hints.temporary_and_permanent_shock.title",
                "Temporary and permanent mental shock"),

            i18n::get(
                "hints.temporary_and_permanent_shock.body",
                "Some situations cause \"temporary\" mental shock, "
                "which is removed when the situation changes. "
                "Entering a dark area or standing next to bloodsplatter will "
                "cause your shock to spike until you move away, for example."

                "\n\nStanding in bright light will similarly reduce your shock "
                "until you return to the ambient subterranean gloom."

                "\n\nSeeing monsters, casting spells, spending time, etc cause "
                "\"permanent\" shock, which will not go away until "
                "the next floor is reached, insanity rises (due to shock at 100%), "
                "or the shock is cured somehow.")};

    default:
        ASSERT(false);
        return {"", ""};
    }
}

static bool should_display_hint(const hints::Id id)
{
    const auto hints_mode = config::hints_mode();

    switch (hints_mode) {
    case HintsMode::once_per_game:
        return !s_hints_displayed[(size_t)id];

    case HintsMode::once:
        return !config::has_seen_hint_global(id);

    case HintsMode::never:
        return false;

    case HintsMode::END:
        break;
    }

    ASSERT(false);

    return false;
}

// -----------------------------------------------------------------------------
// hints
// -----------------------------------------------------------------------------
namespace hints
{
void init()
{
    memset(s_hints_displayed, 0, sizeof(s_hints_displayed));
}

void save()
{
    for (size_t i = 0; i < (size_t)Id::END; ++i) {
        saving::put_bool(s_hints_displayed[i]);
    }
}

void load()
{
    for (size_t i = 0; i < (size_t)Id::END; ++i) {
        s_hints_displayed[i] = saving::get_bool();
    }
}

void display(const Id id)
{
    if (!should_display_hint(id)) {
        return;
    }

    if (!actor::is_alive(*map::g_player)) {
        return;
    }

    msg_log::more_prompt();

    states::draw();
    io::update_screen();

    io::sleep(100);

    const auto text = id_to_text(id);

    if (text.second.empty()) {
        ASSERT(false);

        return;
    }

    popup::Popup(popup::AddToMsgHistory::yes)
        .set_title(i18n::get("hints.title_prefix", "Hint: ") + text.first)
        .set_msg(text.second)
        .run();

    s_hints_displayed[(size_t)id] = true;

    config::set_hint_seen_global(id);
}

}  // namespace hints
