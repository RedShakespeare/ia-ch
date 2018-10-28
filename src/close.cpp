#include "close.hpp"

#include "actor.hpp"
#include "actor_player.hpp"
#include "common_text.hpp"
#include "feature.hpp"
#include "feature_door.hpp"
#include "io.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "property_handler.hpp"
#include "query.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static void player_try_close_or_jam_feature(Feature* const feature)
{
        if (feature->id() != FeatureId::door)
        {
                const bool player_can_see =
                        map::player->properties.allow_see();

                if (player_can_see)
                {
                        msg_log::add(
                                "I see nothing there to close or jam shut.");
                }
                else // Player cannot see
                {
                        msg_log::add(
                                "I find nothing there to close or jam shut.");
                }

                return;
        }

        // This is a door

        Door* const door = static_cast<Door*>(feature);

        if (door->is_open())
        {
                // Door is open, try to close it
                door->try_close(map::player);
        }
        else // Door is closed - try to jam it
        {
                const bool has_spike =
                        map::player->inv.has_item_in_backpack(
                                ItemId::iron_spike);

                if (has_spike)
                {
                        const bool did_spike_door =
                                door->try_jam(map::player);

                        if (did_spike_door)
                        {
                                map::player->inv.decr_item_type_in_backpack(
                                        ItemId::iron_spike);

                                const int nr_spikes_left =
                                        map::player->inv
                                        .item_stack_size_in_backpack(
                                                ItemId::iron_spike);

                                if (nr_spikes_left == 0)
                                {
                                        msg_log::add(
                                                "I have no iron spikes left.");
                                }
                                else // Has spikes left
                                {
                                        msg_log::add(
                                                "I have " +
                                                std::to_string(nr_spikes_left) +
                                                " iron spikes left.");
                                }
                        }
                }
                else // Has no spikes to jam with
                {
                        msg_log::add("I have nothing to jam the door with.");
                }
        }
} // player_try_close_or_jam_feature

// -----------------------------------------------------------------------------
// close_door
// -----------------------------------------------------------------------------
namespace close_door
{

void player_try_close_or_jam()
{
        msg_log::clear();

        msg_log::add(
                "Which direction? " + common_text::cancel_hint,
                colors::light_white());

        const Dir input_dir = query::dir(AllowCenter::no);

        msg_log::clear();

        if ((input_dir != Dir::END) && (input_dir != Dir::center))
        {
                // Valid direction
                const P p(map::player->pos + dir_utils::offset(input_dir));

                player_try_close_or_jam_feature(map::cells.at(p).rigid);
        }
}

} // close
