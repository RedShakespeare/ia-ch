// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef INV_HANDLING_HPP
#define INV_HANDLING_HPP

#include "inventory.hpp"
#include "state.hpp"
#include "browser.hpp"

class Color;

enum class InvScr
{
        inv,
        equip,
        apply,
        none
};

struct FilteredInvEntry
{
        // Index relatie to slot list or relative to backpack list
        size_t relative_idx {0};
        bool is_slot {false};
};

class InvState: public State
{
public:
        InvState();

        virtual ~InvState() {}

        StateId id() override;

protected:
        void draw_slot(
                const SlotId id,
                const int y,
                const char key,
                const bool is_marked,
                const ItemRefAttInf att_info) const;

        void draw_backpack_item(
                const size_t backpack_idx,
                const int y,
                const char key,
                const bool is_marked,
                const ItemRefAttInf att_info) const;

        void activate(const size_t backpack_idx);

        MenuBrowser m_browser;

        void draw_weight_pct_and_dots(
                const P item_pos,
                const size_t item_name_len,
                const item::Item& item,
                const Color& item_name_color_id,
                const bool is_marked) const;

        // void draw_item_symbol(const item::Item& item, const P& p) const;

        void draw_detailed_item_descr(
                const item::Item* const item,
                const ItemRefAttInf att_inf) const;
};

class BrowseInv: public InvState
{
public:
        BrowseInv() :
                InvState() {}

        void on_start() override;

        void draw() override;

        void update() override;

private:
        void on_body_slot_item_selected() const;

        void on_backpack_weapon_selected(
                const size_t backpack_idx) const;
};

class Apply: public InvState
{
public:
        Apply() :
                InvState() {}

        void on_start() override;

        void draw() override;

        void update() override;

private:
        std::vector<size_t> m_filtered_backpack_indexes {};
};

class Drop: public InvState
{
public:
        Drop() :
                InvState() {}

        void on_start() override;

        void draw() override;

        void update() override;
};

class Equip: public InvState
{
public:
        Equip(InvSlot& slot) :
                InvState(),
                m_slot_to_equip(slot) {}

        void on_start() override;

        void draw() override;

        void update() override;

private:
        std::vector<size_t> m_filtered_backpack_indexes {};

        InvSlot& m_slot_to_equip;
};

class SelectThrow: public InvState
{
public:
        SelectThrow() :
                InvState() {}

        void on_start() override;

        void draw() override;

        void update() override;

private:
        std::vector<FilteredInvEntry> m_filtered_inv {};
};

class SelectIdentify: public InvState
{
public:
        SelectIdentify(std::vector<ItemType> item_types_allowed = {}) :
                InvState(),
                m_item_types_allowed(item_types_allowed) {}

        void on_start() override;

        void draw() override;

        void update() override;

private:
        const std::vector<ItemType> m_item_types_allowed;
        std::vector<FilteredInvEntry> m_filtered_inv {};
};

#endif // INV_HANDLING_HPP
