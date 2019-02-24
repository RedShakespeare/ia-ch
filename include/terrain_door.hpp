// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef TERRAIN_DOOR_HPP
#define TERRAIN_DOOR_HPP

#include "terrain.hpp"


enum class DoorSpawnState
{
        open,
        closed,
        stuck,
        secret,
        secret_and_stuck,
        any
};

enum class DoorType
{
        wood,
        metal,
        gate
};


namespace terrain
{

class Door: public Terrain
{
public:
        Door(const P& terrain_pos,

             // NOTE: This should always be nullptr if type is "gate"
             const Wall* const mimic_terrain,

             DoorType type = DoorType::wood,

             // NOTE: For gates, this should never be any "secret" variant
             DoorSpawnState spawn_state = DoorSpawnState::any);

        Door(const P& terrain_pos) :
                Terrain(terrain_pos) {}

        Door() = delete;

        ~Door();

        Id id() const override
        {
                return Id::door;
        }

        // Sometimes we want to refer to a door as just a "door", instead of
        // something verbose like "the open wooden door".
        std::string base_name() const; // E.g. "wooden door"

        std::string base_name_short() const; // E.g. "door"

        std::string name(const Article article) const override;

        WasDestroyed on_finished_burning() override;

        char character() const override;

        TileId tile() const override;

        void bump(actor::Actor& actor_bumping) override;

        bool is_walkable() const override;

        bool can_move(const actor::Actor& actor) const override;

        bool is_los_passable() const override;

        bool is_projectile_passable() const override;

        bool is_smoke_passable() const override;

        void try_open(actor::Actor* actor_trying);

        void try_close(actor::Actor* actor_trying);

        bool try_jam(actor::Actor* actor_trying);

        void on_lever_pulled(Lever* const lever) override;

        bool is_open() const
        {
                return m_is_open;
        }

        bool is_secret() const
        {
                return m_is_secret;
        }

        bool is_stuck() const
        {
                return m_is_stuck;
        }

        Matl matl() const override;

        void reveal(const Verbosity verbosity) override;

        void set_secret();

        void set_stuck()
        {
                m_is_open = false;
                m_is_stuck = true;
        }

        DidOpen open(actor::Actor* const actor_opening) override;

        DidClose close(actor::Actor* const actor_closing) override;

        static bool is_tile_any_door(const TileId tile)
        {
                return
                        tile == TileId::door_closed ||
                        tile == TileId::door_open;
        }

        const Wall* mimic() const
        {
                return m_mimic_terrain;
        }

        DoorType type() const
        {
                return m_type;
        }

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        const Wall* const m_mimic_terrain {nullptr};

        int m_nr_spikes {0};

        bool m_is_open {false};
        bool m_is_stuck {false};
        bool m_is_secret {false};

        DoorType m_type {DoorType::wood};

}; // Door

} // terrain

#endif // TERRAIN_DOOR_HPP
