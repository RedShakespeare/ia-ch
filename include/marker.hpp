// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef MARKER_HPP
#define MARKER_HPP

#include <climits>

#include "array2.hpp"
#include "global.hpp"
#include "io.hpp"
#include "state.hpp"


namespace item
{
class Item;
class Wpn;
}

struct InputData;


// -----------------------------------------------------------------------------
// Abstract marker state base class
// -----------------------------------------------------------------------------
class MarkerState: public State
{
public:
        MarkerState(const P& origin) :
                State(),
                m_marker_render_data(P(0, 0)),
                m_origin(origin),
                m_pos() {}

        virtual ~MarkerState() {}

        void on_start() override final;

        void on_popped() override final;

        void draw() override final;

        bool draw_overlayed() const override final
        {
                return true;
        }

        void update() override final;

        StateId id() override final;

protected:
        virtual void on_start_hook() {}

        void draw_marker(
                const std::vector<P>& trail,
                const int orange_from_king_dist,
                const int red_from_king_dist,
                const int red_from_idx);

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

        virtual int orange_from_king_dist() const
        {
                return -1;
        }

        virtual int red_from_king_dist() const
        {
                return -1;
        }

        // Necessary e.g. for marker states drawing overlayed graphics
        Array2<CellRenderData> m_marker_render_data;

        const P m_origin;

        P m_pos;

private:
        void move(const Dir dir, const int nr_steps = 1);

        bool try_go_to_tgt();

        void try_go_to_closest_enemy();
};

// -----------------------------------------------------------------------------
// View marker state
// -----------------------------------------------------------------------------
class Viewing: public MarkerState
{
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
class Aiming: public MarkerState
{
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

        int orange_from_king_dist() const override;

        int red_from_king_dist() const override;

        item::Wpn& m_wpn;
};

// -----------------------------------------------------------------------------
// Throw attack marker state
// -----------------------------------------------------------------------------
class Throwing: public MarkerState
{
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

        int orange_from_king_dist() const override;

        int red_from_king_dist() const override;

        item::Item* m_inv_item;
};

// -----------------------------------------------------------------------------
// Throw explosive marker state
// -----------------------------------------------------------------------------
class ThrowingExplosive: public MarkerState
{
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

        int red_from_king_dist() const override;

        const item::Item& m_explosive;
};

// -----------------------------------------------------------------------------
// Teleport control marker state
// -----------------------------------------------------------------------------
class CtrlTele: public MarkerState
{
public:
        CtrlTele(const P& origin, const Array2<bool>& blocked);

protected:
        void on_start_hook() override;

        void on_moved() override;

        void handle_input(const InputData& input) override;

private:
        int chance_of_success_pct(const P& tgt) const;

        Array2<bool> m_blocked;
};

#endif // MARKER_HPP
