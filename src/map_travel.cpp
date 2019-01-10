// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "map_travel.hpp"

#include "init.hpp"

#include <list>

#include "actor_factory.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "draw_map.hpp"
#include "feature_rigid.hpp"
#include "io.hpp"
#include "map.hpp"
#include "map_builder.hpp"
#include "map_controller.hpp"
#include "mapgen.hpp"
#include "minimap.hpp"
#include "msg_log.hpp"
#include "populate_items.hpp"
#include "property.hpp"
#include "property_handler.hpp"
#include "saving.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static void trigger_insanity_sympts_for_descent()
{
        // Phobia of deep places
        if (insanity::has_sympt(InsSymptId::phobia_deep))
        {
                msg_log::add("I am plagued by my phobia of deep places!");

                map::player->properties.apply(new PropTerrified());
        }

        // Babbling
        for (const auto* const sympt : insanity::active_sympts())
        {
                if (sympt->id() == InsSymptId::babbling)
                {
                        static_cast<const InsBabbling*>(sympt)->babble();
                }
        }
}

// -----------------------------------------------------------------------------
// map_travel
// -----------------------------------------------------------------------------
namespace map_travel
{

std::vector<MapType> map_list;

void init()
{
        // Forest + dungeon + boss + trapezohedron
        const size_t nr_lvl_tot = dlvl_last + 3;

        map_list = std::vector<MapType>(nr_lvl_tot, MapType::std);

        if (rnd::one_in(3))
        {
                const int deep_one_lvl_nr =
                        rnd::range(
                                dlvl_first_mid_game,
                                dlvl_last_mid_game - 1);

                map_list[deep_one_lvl_nr] = MapType::deep_one_lair;
        }

        if (rnd::one_in(8))
        {
                map_list[dlvl_first_late_game - 1] = MapType::rat_cave;
        }

        // "Pharaoh chamber" is the first late game level
        map_list[dlvl_first_late_game] = MapType::egypt;

        map_list[dlvl_last + 1] = MapType::high_priest;

        map_list[dlvl_last + 2] = MapType::trapez;
}

void save()
{
        saving::put_int(map_list.size());

        for (const MapType type : map_list)
        {
                saving::put_int((int)type);
        }
}

void load()
{
        const int nr_maps = saving::get_int();

        map_list.resize((size_t)nr_maps);

        for (auto& type : map_list)
        {
                type = (MapType)saving::get_int();
        }
}

void go_to_nxt()
{
        TRACE_FUNC_BEGIN;

        io::clear_screen();
        io::update_screen();

        draw_map::clear();

        minimap::clear();

        map_list.erase(map_list.begin());

        const MapType map_type = map_list.front();

        ++map::dlvl;

        const auto map_builder = map_builder::make(map_type);

        map_builder->build();

        if (map::player->properties.has(PropId::descend))
        {
                msg_log::add("My sinking feeling disappears.");

                map::player->properties.end_prop_silent(PropId::descend);
        }

        game_time::is_magic_descend_nxt_std_turn = false;

        map::player->tgt_ = nullptr;

        map::update_vision();

        map::player->restore_shock(999, true);

        msg_log::add("I have discovered a new area.");

        // NOTE: When the "intro level" is skipped, "go_to_nxt" is called when
        // the game starts - so no XP is missed in that case (same thing when
        // loading the game)
        game::incr_player_xp(5, Verbosity::verbose);

        map::player->on_new_dlvl_reached();

        game::add_history_event(
                "Reached dungeon level " +
                std::to_string(map::dlvl));

        trigger_insanity_sympts_for_descent();

        if (map_control::controller)
        {
                map_control::controller->on_start();
        }

        TRACE_FUNC_END;
}

} // map_travel
