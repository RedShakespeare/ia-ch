#include "knockback.hpp"

#include <algorithm>
#include <vector>

#include "attack.hpp"
#include "actor_player.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "config.hpp"
#include "game_time.hpp"
#include "io.hpp"
#include "map_parsing.hpp"
#include "sdl_base.hpp"
#include "feature_rigid.hpp"
#include "feature_trap.hpp"
#include "feature_mob.hpp"
#include "text_format.hpp"
#include "property.hpp"
#include "property_data.hpp"

namespace knockback
{

void run(Actor& defender,
         const P& attacked_from_pos,
         const bool is_spike_gun,
         const Verbosity verbosity,
         const int paralyze_extra_turns)
{
        TRACE_FUNC_BEGIN;

        ASSERT(paralyze_extra_turns >= 0);

        const bool is_defender_player = defender.is_player();

        const auto& defender_data = defender.data();

        if (defender_data.prevent_knockback ||
            (defender_data.actor_size >= ActorSize::giant) ||
            defender.has_prop(PropId::entangled) ||
            defender.has_prop(PropId::ethereal) ||
            defender.has_prop(PropId::ooze) ||
            // Do not knock back player if bot is playing
            (is_defender_player && config::is_bot_playing()))
        {
                // Defender is not knockable

                TRACE_FUNC_END;

                return;
        }

        const P d = (defender.pos - attacked_from_pos).signs();

        const P new_pos = defender.pos + d;

        const bool is_cell_blocked =
                map_parsers::BlocksActor(defender, ParseActors::yes)
                .cell(new_pos);

        const bool is_cell_bottomless =
                map::cells.at(new_pos).rigid->is_bottomless();

        if (is_cell_blocked && !is_cell_bottomless)
        {
                // Defender nailed to a wall from a spike gun?
                if (is_spike_gun)
                {
                        Rigid* const f = map::cells.at(new_pos).rigid;

                        if (!f->is_los_passable())
                        {
                                auto prop = new PropNailed();

                                prop->set_indefinite();

                                defender.apply_prop(prop);
                        }
                }

                TRACE_FUNC_END;

                return;
        }

        const bool player_see_defender =
                is_defender_player ?
                true :
                map::player->can_see_actor(defender);

        if (verbosity == Verbosity::verbose &&
            player_see_defender)
        {
                if (is_defender_player)
                {
                        msg_log::add("I am knocked back!");
                }
                else
                {
                        const std::string name_the =
                                text_format::first_to_upper(
                                        defender.name_the());

                        msg_log::add(name_the + " is knocked back!");
                }
        }

        auto prop = new PropParalyzed();

        prop->set_duration(1 + paralyze_extra_turns);

        defender.apply_prop(prop);

        // Leave current cell
        map::cells.at(defender.pos).rigid->on_leave(defender);

        defender.pos = new_pos;

        if (is_cell_bottomless &&
            !defender.has_prop(PropId::flying)  &&
            player_see_defender)
        {
                if (is_defender_player)
                {
                        msg_log::add("I plummet down the depths!",
                                     colors::msg_bad());
                }
                else
                {
                        const std::string name_the =
                                text_format::first_to_upper(
                                        defender.name_the());

                        msg_log::add(name_the +
                                     " plummets down the depths.",
                                     colors::msg_good());
                }

                defender.die(true, false, false);

                TRACE_FUNC_END;
                return;
        }

        // Bump target cell
        std::vector<Mob*> mobs;

        game_time::mobs_at_pos(defender.pos, mobs);

        for (Mob* const mob : mobs)
        {
                mob->bump(defender);
        }

        if (!defender.is_alive())
        {
                TRACE_FUNC_END;
                return;
        }

        map::cells.at(defender.pos).rigid->bump(defender);

        TRACE_FUNC_END;
}

} // knockback
