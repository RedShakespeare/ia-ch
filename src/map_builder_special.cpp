// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "map_builder.hpp"

#include "actor_factory.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "feature_door.hpp"
#include "feature_door.hpp"
#include "feature_event.hpp"
#include "feature_monolith.hpp"
#include "feature_rigid.hpp"
#include "game_time.hpp"
#include "highscore.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "map_controller.hpp"
#include "populate_items.hpp"
#include "property.hpp"

// -----------------------------------------------------------------------------
// MapBuilderDeepOneLair
// -----------------------------------------------------------------------------
MapBuilderDeepOneLair::MapBuilderDeepOneLair() :
        MapBuilderTemplateLevel(),
        m_passage_symbol('1' + rnd::range(0, 1))
{

}

void MapBuilderDeepOneLair::handle_template_pos(const P& p, const char c)
{
        switch (c)
        {
        case '@':
        case '.':
        case 'd':
        case '%': // TODO: Just put random blood/gore on the level instead?
        case 'B':
        {
                map::put(new Floor(p));

                if (c == '@')
                {
                        map::g_player->m_pos = p;
                }
                else if (c == 'd')
                {
                        actor::make(actor::Id::deep_one, p);
                }
                else if (c == 'B')
                {
                        actor::make(actor::Id::niduza, p);
                }
                else if (c == '%')
                {
                        map::make_blood(p);
                        map::make_gore(p);
                }
        }
        break;

        case '&': // TODO: Just put random bones on the level instead?
        {
                map::put(new Bones(p));
        }
        break;

        case '#':
        case '1':
        case '2':
        {
                Rigid* f;

                if (c == m_passage_symbol)
                {
                        f = new Floor(p);
                }
                else
                {
                        f = new Wall(p);

                        static_cast<Wall*>(f)->m_type = WallType::cave;
                }

                map::put(f);
        }
        break;

        case '*':
        {
                auto* water = new LiquidShallow(p);

                water->m_type = LiquidType::water;

                map::put(water);
        }
        break;

        case '~':
        {
                auto* water = new LiquidDeep(p);

                water->m_type = LiquidType::water;

                map::put(water);
        }
        break;

        case 'x':
        {
                Door* const door =
                        new Door(p,
                                 nullptr,
                                 DoorType::gate,
                                 DoorSpawnState::closed);

                map::put(door);
        }
        break;

        case '>':
        {
                map::put(new Stairs(p));
        }
        break;

        case '|':
        {
                map::put(new Monolith(p));
        }
        break;

        case '-':
        {
                map::put(new Altar(p));
        }
        break;

        case ':':
        {
                map::put(new Stalagmite(p));
        }
        break;
        }
}

void MapBuilderDeepOneLair::on_template_built()
{
        populate_items::make_items_on_floor();
}

// -----------------------------------------------------------------------------
// MapBuilderIntroForest
// -----------------------------------------------------------------------------
void MapBuilderIntroForest::handle_template_pos(const P&p, const char c)
{
        switch (c)
        {
        case '@':
        case '=':
        {
                Floor* const floor = new Floor(p);

                floor->m_type = FloorType::stone_path;

                map::put(floor);

                if (c == '@')
                {
                        map::g_player->m_pos = p;
                }
        }
        break;

        case '_':
        {
                Grass* const grass = new Grass(p);
                grass->m_type = GrassType::withered;
                map::put(grass);
        }
        break;

        case '.':
        {
                if (rnd::one_in(6))
                {
                        if (rnd::one_in(6))
                        {
                                map::put(new Bush(p));
                        }
                        else
                        {
                                map::put(new Grass(p));
                        }
                }
                else // Normal stone floor
                {
                        map::put(new Floor(p));
                }
        }
        break;

        case '#':
        {
                bool is_door_adj = false;

                for (const P& d : dir_utils::g_dir_list)
                {
                        const char adj_c = get_template().at(p + d);

                        if (adj_c == '+')
                        {
                                is_door_adj = true;
                                break;
                        }
                }

                Rigid* rigid = nullptr;

                if (!is_door_adj)
                {
                        if (rnd::one_in(16))
                        {
                                rigid = map::put(new RubbleLow(p));
                        }
                        else if (rnd::one_in(4))
                        {
                                rigid = map::put(new RubbleHigh(p));
                        }
                }

                if (!rigid)
                {
                        Wall* const wall = new Wall(p);

                        if (rnd::one_in(20))
                        {
                                wall->set_moss_grown();
                        }

                        map::put(wall);
                }
        }
        break;

        case '&':
        {
                // Store this position for placing player graves
                m_possible_grave_positions.push_back(p);
        }
        // fallthrough
        case ',':
        {
                if (rnd::one_in(12))
                {
                        map::put(new Bush(p));
                }
                else
                {
                        map::put(new Grass(p));
                }
        }
        break;

        case '~':
        {
                auto liquid = new LiquidShallow(p);

                liquid->m_type = LiquidType::water;

                map::put(liquid);
        }
        break;

        case 't':
        {
                map::put(new Tree(p));
        }
        break;

        case 'v':
        {
                map::put(new Brazier(p));
        }
        break;

        case '[':
        {
                map::put(new ChurchBench(p));
        }
        break;

        case '-':
        {
                auto* const altar = new Altar(p);

                altar->disable_pact();

                map::put(altar);
        }
        break;

        case '*':
        {
                map::put(new Carpet(p));
        }
        break;

        case '>':
        {
                map::put(new Stairs(p));
        }
        break;

        case '+':
        {
                Door* const door =
                        new Door(p,
                                 new Wall(p),
                                 DoorType::wood,
                                 DoorSpawnState::closed);

                map::put(door);
        }
        break;

        default:
        {
                ASSERT(false);
        }
        break;
        }
}

void MapBuilderIntroForest::on_template_built()
{
        std::vector<HighscoreEntry> entries =
                highscore::entries_sorted();

        const int nr_non_win =
                count_if(begin(entries),
                         end(entries),
                         [](const HighscoreEntry & e)
                         {
                                 return (e.is_win() == IsWin::no);
                         });

        if (nr_non_win <= 0)
        {
                return;
        }

        size_t entry_idx = 0;

        int nr_placed = 0;

        // NOTE: This assumes that the grave positions are added from left to
        // right when building the template. Here we iterate backwards over the
        // positions, starting with the highest score.
        for (auto it = std::rbegin(m_possible_grave_positions);
             it != std::rend(m_possible_grave_positions);
             ++it)
        {
                const P& pos = *it;

                GraveStone* grave = new GraveStone(pos);

                HighscoreEntry entry = entries[entry_idx];

                // Skip winning entries
                while (entry.is_win() == IsWin::yes)
                {
                        ++entry_idx;

                        entry = entries[entry_idx];
                }

                const std::string name = entry.name();

                const std::string date_str = entry.date();

                const std::string score_str = std::to_string(entry.score());

                const std::string class_str = player_bon::bg_title(entry.bg());

                grave->set_inscription(
                        "RIP " +
                        name + ", " +
                        class_str + ", " +
                        date_str + ", " +
                        "Score: " + score_str);

                map::put(grave);

                ++nr_placed;
                ++entry_idx;

                if (nr_placed == nr_non_win)
                {
                        break;
                }
        }
}

// -----------------------------------------------------------------------------
// MapBuilderEgypt
// -----------------------------------------------------------------------------
MapBuilderEgypt::MapBuilderEgypt() :
        MapBuilderTemplateLevel(),
        m_stair_symbol('1' + rnd::range(0, 3))
{

}

void MapBuilderEgypt::handle_template_pos(const P&p, const char c)
{
        switch (c)
        {
        case '.':
        case '@':
        case 'P':
        case 'M':
        case 'C':
        case '1':
        case '2':
        case '3':
        case '4':
        {
                if (c == '@')
                {
                        map::g_player->m_pos = p;
                }

                if (c == m_stair_symbol)
                {
                        map::put(new Stairs(p));
                }
                else
                {
                        map::put(new Floor(p));
                }

                auto actor_id = actor::Id::END;

                switch (c)
                {
                case 'P':
                        actor_id = actor::Id::khephren;
                        break;

                case 'M':
                        actor_id = actor::Id::mummy;
                        break;

                case 'C':
                        actor_id = actor::Id::croc_head_mummy;
                        break;
                }

                if (actor_id != actor::Id::END)
                {
                        auto* const actor = actor::make(actor_id, p);

                        static_cast<actor::Mon*>(actor)->m_is_roaming_allowed =
                                MonRoamingAllowed::no;
                }
        }
        break;

        case '#':
        {
                Wall* wall = new Wall(p);

                wall->m_type = WallType::egypt;

                map::put(wall);
        }
        break;

        case 'v':
        {
                map::put(new Brazier(p));
        }
        break;

        case 'S':
        {
                map::put(new Statue(p));
        }
        break;

        case '+':
        {
                Door* const door =
                        new Door(p,
                                 new Wall(p),
                                 DoorType::wood,
                                 DoorSpawnState::closed);

                map::put(door);
        }
        break;

        case '~':
        {
                auto liquid = new LiquidShallow(p);

                liquid->m_type = LiquidType::water;

                map::put(liquid);
        }
        break;
        }
}

void MapBuilderEgypt::on_template_built()
{
        populate_items::make_items_on_floor();
}

// -----------------------------------------------------------------------------
// MapBuilderRatCave
// -----------------------------------------------------------------------------
void MapBuilderRatCave::handle_template_pos(const P& p, const char c)
{
        switch (c)
        {
        case '@':
        case '.':
        case ',':
        case '&':
        case 'r':
        case '1':
        {
                if ((c == '&') ||
                    ((c == ',' || c == 'r') && rnd::coin_toss()))
                {
                        map::put(new Bones(p));
                }
                else
                {
                        map::put(new Floor(p));
                }

                if (c == '@')
                {
                        map::g_player->m_pos = p;
                }
                else if (c == '1')
                {
                        // TODO: Should be handled by map controller instead
                        game_time::add_mob(
                                new EventRatsInTheWallsDiscovery(p));
                }
                else if (c == 'r')
                {
                        actor::Actor* actor = nullptr;

                        if (rnd::one_in(6))
                        {
                                actor = actor::make(actor::Id::rat_thing, p);
                        }
                        else
                        {
                                actor = actor::make(actor::Id::rat, p);
                        }

                        auto prop = new PropFrenzied();

                        prop->set_indefinite();

                        actor->m_properties.apply(
                                prop,
                                PropSrc::intr,
                                false,
                                Verbosity::silent);
                }
        }
        break;

        case '#':
        {
                auto* wall = new Wall(p);

                wall->m_type = WallType::cave;

                map::put(wall);
        }
        break;

        case 'x':
        {
                if (rnd::one_in(3))
                {
                        map::put(new RubbleLow(p));
                }
                else if (rnd::one_in(5))
                {
                        map::put(new RubbleHigh(p));
                }
                else
                {
                        auto* wall = new Wall(p);

                        wall->m_type = WallType::common;

                        map::put(wall);
                }
        }
        break;

        case '>':
        {
                map::put(new Stairs(p));
        }
        break;

        case '|':
        {
                map::put(new Monolith(p));
        }
        break;

        case ':':
        {
                map::put(new Stalagmite(p));
        }
        break;
        }
}

void MapBuilderRatCave::on_template_built()
{
        // Set all actors to non-roaming (they will be set to roaming later)
        for (auto* const actor : game_time::g_actors)
        {
                if (!actor->is_player())
                {
                        static_cast<actor::Mon*>(actor)->m_is_roaming_allowed =
                                MonRoamingAllowed::no;
                }
        }

        populate_items::make_items_on_floor();
}

// -----------------------------------------------------------------------------
// MapBuilderBoss
// -----------------------------------------------------------------------------
void MapBuilderBoss::handle_template_pos(const P& p, const char c)
{
        switch (c)
        {
        case '@':
        case 'P':
        case 'W':
        case 'R':
        case 'G':
        case '.':
        {
                map::put(new Floor(p));

                if (c == '@')
                {
                        map::g_player->m_pos = p;
                }
                else if (c == 'P')
                {
                        actor::make(actor::Id::the_high_priest, p);
                }
                else if (c == 'W')
                {
                        actor::make(actor::Id::high_priest_guard_war_vet, p);
                }
                else if (c == 'R')
                {
                        actor::make(actor::Id::high_priest_guard_rogue, p);
                }
                else if (c == 'G')
                {
                        actor::make(actor::Id::high_priest_guard_ghoul, p);
                }
        }
        break;

        case '#':
        {
                Wall* const wall = new Wall(p);

                wall->m_type = WallType::egypt;

                map::put(wall);
        }
        break;

        case 'v':
        {
                map::put(new Brazier(p));
        }
        break;

        case '>':
        {
                map::put(new Stairs(p));
        }
        break;
        }
}

void MapBuilderBoss::on_template_built()
{
        // Make the High Priest leader of all other monsters
        actor::Actor* high_priest = nullptr;

        for (auto* const actor : game_time::g_actors)
        {
                if (actor->id() == actor::Id::the_high_priest)
                {
                        high_priest = actor;

                        break;
                }
        }

        for (auto* const actor : game_time::g_actors)
        {
                if (!actor->is_player() && (actor != high_priest))
                {
                        static_cast<actor::Mon*>(actor)->m_leader = high_priest;
                }
        }
}

std::unique_ptr<MapController> MapBuilderBoss::map_controller() const
{
        return std::make_unique<MapControllerBoss>();
}

// -----------------------------------------------------------------------------
// MapBuilderTrapez
// -----------------------------------------------------------------------------
void MapBuilderTrapez::handle_template_pos(const P& p, const char c)
{
        map::g_dark.at(p) = true;

        switch (c)
        {
        case '@':
        case '.':
        case 'o':
        {
                map::put(new Floor(p));

                if (c == '@')
                {
                        map::g_player->m_pos = p;
                }
                else if (c == 'o')
                {
                        item::make_item_on_floor(item::Id::trapez, p);
                }
        }
        break;

        case '#':
        {
                Wall* const wall = new Wall(p);

                wall->m_type = WallType::egypt;

                map::put(wall);
        }
        break;

        case 'v':
        {
                map::put(new Brazier(p));
        }
        break;
        }
}
