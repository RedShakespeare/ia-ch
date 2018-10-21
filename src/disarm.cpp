#include "disarm.hpp"

#include "game_time.hpp"
#include "msg_log.hpp"
#include "io.hpp"
#include "actor_player.hpp"
#include "query.hpp"
#include "map.hpp"
#include "feature_trap.hpp"
#include "inventory.hpp"
#include "property_factory.hpp"

namespace disarm
{

void player_disarm()
{
        if (!map::player->properties.allow_see())
        {
                msg_log::add("Not while blind.");

                return;
        }

        if (map::player->properties.has(PropId::entangled))
        {
                msg_log::add("Not while entangled.");

                return;
        }

        msg_log::add(
                "Which direction?" + cancel_info_str,
                colors::light_white());

        const auto input_dir = query::dir(AllowCenter::yes);

        msg_log::clear();

        if (input_dir == Dir::END)
        {
                ASSERT(false);

                return;
        }

        const auto pos = map::player->pos + dir_utils::offset(input_dir);

        if (!map::cells.at(pos).is_seen_by_player)
        {
                msg_log::add("I cannot see there.");

                return;
        }

        auto* const feature = map::cells.at(pos).rigid;

        Trap* trap = nullptr;

        if (feature->id() == FeatureId::trap)
        {
                trap = static_cast<Trap*>(feature);
        }

        if (!trap || trap->is_hidden())
        {
                msg_log::add(msg_disarm_no_trap);

                states::draw();

                return;
        }

        // There is a known and seen trap here

        const auto* const actor_on_trap = map::actor_at_pos(pos);

        if (actor_on_trap && !actor_on_trap->is_player())
        {
                if (map::player->can_see_actor(*actor_on_trap))
                {
                        msg_log::add("It's blocked.");
                }
                else
                {
                        msg_log::add("Something is blocking it.");
                }

                return;
        }

        trap->disarm();

        game_time::tick();

} // player_disarm

} // disarm
