// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MARKER_HPP
#define MARKER_HPP

#include <climits>

#include "array2.hpp"
#include "global.hpp"
#include "io.hpp"
#include "random.hpp"
#include "state.hpp"

namespace item {
class Item;
class Wpn;
} // namespace item

struct InputData;

// -----------------------------------------------------------------------------
// Abstract marker state base class
// -----------------------------------------------------------------------------
class MarkerState : public State {
public:
        MarkerState(const P& origin) :

                m_marker_render_data(P(0, 0)),
                m_origin(origin)
        {}

        virtual ~MarkerState() = default;

        void on_start() final;

        void on_popped() final;

        void draw() final;

        bool draw_overlayed() const final
        {
                return true;
        }

        void update() final;

        StateId id() final;

protected:
        virtual void on_start_hook() {}

        void draw_marker(
                const std::vector<P>& line,
                int orange_until_including_king_dist,
                int orange_from_king_dist,
                int red_from_king_dist,
                int red_from_idx);

        // Fire etc
        virtual void handle_input(const InputData& input) = 0;

        // Print messages
        virtual void on_moved() = 0;

        // Used for overlays, etc - it should be pretty rare that this is needed
        virtual void on_draw() {}

        virtual bool use_player_tgt() const
        {
                return false;
        }

        virtual bool show_blocked() const
        {
                return false;
        }

        virtual Range effective_king_dist_range() const
        {
                return {-1, -1};
        }

        virtual int max_king_dist() const
        {
                return -1;
        }

        // Necessary e.g. for marker states drawing overlayed graphics
        Array2<CellRenderData> m_marker_render_data;

        const P m_origin;

        P m_pos;

private:
        void move(Dir dir, int nr_steps = 1);

        bool try_go_to_tgt();

        void try_go_to_closest_enemy();
};

// -----------------------------------------------------------------------------
// View marker state
// -----------------------------------------------------------------------------
class Viewing : public MarkerState {
public:
        Viewing(const P& origin) :
                MarkerState(origin) {}

protected:
        void on_moved() override;

        void handle_input(const InputData& input) override;

        bool use_player_tgt() const override
        {
                return true;
        }

        bool show_blocked() const override
        {
                return false;
        }
};

// -----------------------------------------------------------------------------
// Aim (and fire) marker state
// -----------------------------------------------------------------------------
class Aiming : public MarkerState {
public:
        Aiming(const P& origin, item::Wpn& wpn) :
                MarkerState(origin),
                m_wpn(wpn) {}

protected:
        void on_moved() override;

        void handle_input(const InputData& input) override;

        bool use_player_tgt() const override
        {
                return true;
        }

        bool show_blocked() const override
        {
                return true;
        }

        Range effective_king_dist_range() const override;

        int max_king_dist() const override;

        item::Wpn& m_wpn;
};

// -----------------------------------------------------------------------------
// Throw attack marker state
// -----------------------------------------------------------------------------
class Throwing : public MarkerState {
public:
        Throwing(const P& origin, item::Item& inv_item) :
                MarkerState(origin),
                m_inv_item(&inv_item) {}

protected:
        void on_moved() override;

        void handle_input(const InputData& input) override;

        bool use_player_tgt() const override
        {
                return true;
        }

        bool show_blocked() const override
        {
                return true;
        }

        Range effective_king_dist_range() const override;

        int max_king_dist() const override;

        item::Item* m_inv_item;
};

// -----------------------------------------------------------------------------
// Throw explosive marker state
// -----------------------------------------------------------------------------
class ThrowingExplosive : public MarkerState {
public:
        ThrowingExplosive(const P& origin, const item::Item& explosive) :
                MarkerState(origin),
                m_explosive(explosive) {}

protected:
        void on_draw() override;

        void on_moved() override;

        void handle_input(const InputData& input) override;

        bool use_player_tgt() const override
        {
                return false;
        }

        bool show_blocked() const override
        {
                return true;
        }

        int max_king_dist() const override;

        const item::Item& m_explosive;
};

// -----------------------------------------------------------------------------
// Teleport control marker state
// -----------------------------------------------------------------------------
class CtrlTele : public MarkerState {
public:
        CtrlTele(const P& origin, Array2<bool> blocked, int max_dist = -1);

protected:
        void on_start_hook() override;

        void on_moved() override;

        void handle_input(const InputData& input) override;

private:
        int chance_of_success_pct(const P& tgt) const;

        P m_origin;
        int m_max_dist;
        Array2<bool> m_blocked;
};

#endif // MARKER_HPP
