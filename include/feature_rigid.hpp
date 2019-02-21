// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef FEATURE_RIGID_HPP
#define FEATURE_RIGID_HPP

#include <memory>

#include "feature.hpp"


namespace item
{

class Item;

} // item


class Lever;


enum class BurnState
{
        not_burned,
        burning,
        has_burned
};

enum class WasDestroyed
{
        no,
        yes
};

enum class DidTriggerTrap
{
        no,
        yes
};

enum class DidOpen
{
        no,
        yes
};

enum class DidClose
{
        no,
        yes
};

class ItemContainer
{
public:
        ItemContainer();

        ~ItemContainer();

        void init(const FeatureId feature_id, const int nr_items_to_attempt);

        void open(const P& feature_pos, actor::Actor* const actor_opening);

        void destroy_single_fragile();

        std::vector<item::Item*> m_items;
};

class Rigid: public Feature
{
public:
        Rigid(const P& p);

        Rigid() = delete;

        virtual ~Rigid() {}

        virtual FeatureId id() const override = 0;

        virtual std::string name(const Article article) const override = 0;

        virtual AllowAction pre_bump(actor::Actor& actor_bumping) override;

        virtual void on_new_turn() override final;

        Color color() const override final;

        virtual Color color_bg() const override final;

        virtual void hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor = nullptr) override;

        int shock_when_adj() const;

        void try_put_gore();

        TileId gore_tile() const
        {
                return m_gore_tile;
        }

        char gore_character() const
        {
                return m_gore_character;
        }

        void clear_gore();

        virtual DidOpen open(actor::Actor* const actor_opening);

        virtual DidClose close(actor::Actor* const actor_closing);

        virtual void on_lever_pulled(Lever* const lever)
        {
                (void)lever;
        }

        void add_light(Array2<bool>& light) const override final;

        void make_bloody()
        {
                m_is_bloody = true;
        }

        void corrupt_color();

        ItemContainer m_item_container {};

        BurnState m_burn_state {BurnState::not_burned};

        bool m_started_burning_this_turn {false};

protected:
        virtual void on_new_turn_hook() {}

        virtual void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) = 0;

        virtual Color color_default() const = 0;

        virtual Color color_bg_default() const
        {
                return colors::black();
        }

        void try_start_burning(const bool is_msg_allowed);

        virtual WasDestroyed on_finished_burning();

        virtual DidTriggerTrap trigger_trap(actor::Actor* const actor);

        virtual void add_light_hook(Array2<bool>& light) const
        {
                (void)light;
        }

        virtual int base_shock_when_adj() const;

        TileId m_gore_tile {TileId::END};
        char m_gore_character {0};

private:
        bool m_is_bloody {false};

        // Corrupted by a Strange Color monster
        int m_nr_turns_color_corrupted {-1};
};

enum class FloorType
{
        common,
        cave,
        stone_path
};

class Floor: public Rigid
{
public:
        Floor(const P& p);

        Floor() = delete;

        FeatureId id() const override
        {
                return FeatureId::floor;
        }

        TileId tile() const override;

        std::string name(const Article article) const override;

        FloorType m_type;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class Carpet: public Rigid
{
public:
        Carpet(const P& p);

        Carpet() = delete;

        FeatureId id() const override
        {
                return FeatureId::carpet;
        }

        std::string name(const Article article) const override;

        WasDestroyed on_finished_burning() override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

enum class GrassType
{
        common,
        withered
};

class Grass: public Rigid
{
public:
        Grass(const P& p);

        Grass() = delete;

        FeatureId id() const override
        {
                return FeatureId::grass;
        }

        TileId tile() const override;
        std::string name(const Article article) const override;

        GrassType m_type;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class Bush: public Rigid
{
public:
        Bush(const P& p);

        Bush() = delete;

        FeatureId id() const override
        {
                return FeatureId::bush;
        }

        std::string name(const Article article) const override;
        WasDestroyed on_finished_burning() override;

        GrassType m_type;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class Vines: public Rigid
{
public:
        Vines(const P& p);

        Vines() = delete;

        FeatureId id() const override
        {
                return FeatureId::vines;
        }

        std::string name(const Article article) const override;
        WasDestroyed on_finished_burning() override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class Chains: public Rigid
{
public:
        Chains(const P& p);

        Chains() = delete;

        FeatureId id() const override
        {
                return FeatureId::chains;
        }

        std::string name(const Article article) const override;

        void bump(actor::Actor& actor_bumping) override;

private:
        Color color_default() const override;

        Color color_bg_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class Grate: public Rigid
{
public:
        Grate(const P& p);

        Grate() = delete;

        FeatureId id() const override
        {
                return FeatureId::grate;
        }

        std::string name(const Article article) const override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class Brazier: public Rigid
{
public:
        Brazier(const P& p) : Rigid(p) {}

        Brazier() = delete;

        FeatureId id() const override
        {
                return FeatureId::brazier;
        }

        std::string name(const Article article) const override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        void add_light_hook(Array2<bool>& light) const override;
};

enum class WallType
{
        common,
        common_alt,
        cave,
        egypt,
        cliff,
        leng_monestary
};

class Wall: public Rigid
{
public:
        Wall(const P& p);

        Wall() = delete;

        FeatureId id() const override
        {
                return FeatureId::wall;
        }

        std::string name(const Article article) const override;
        char character() const override;
        TileId front_wall_tile() const;
        TileId top_wall_tile() const;

        void set_rnd_common_wall();
        void set_moss_grown();

        WallType m_type;
        bool m_is_mossy;

        static bool is_wall_front_tile(const TileId tile);
        static bool is_wall_top_tile(const TileId tile);

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class RubbleLow: public Rigid
{
public:
        RubbleLow(const P& p);

        RubbleLow() = delete;

        FeatureId id() const override
        {
                return FeatureId::rubble_low;
        }

        std::string name(const Article article) const override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class Bones: public Rigid
{
public:
        Bones(const P& p);

        Bones() = delete;

        FeatureId id() const override
        {
                return FeatureId::bones;
        }

        std::string name(const Article article) const override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class RubbleHigh: public Rigid
{
public:
        RubbleHigh(const P& p);

        RubbleHigh() = delete;

        FeatureId id() const override
        {
                return FeatureId::rubble_high;
        }

        std::string name(const Article article) const override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class GraveStone: public Rigid
{
public:
        GraveStone(const P& p);

        GraveStone() = delete;

        FeatureId id() const override
        {
                return FeatureId::gravestone;
        }

        std::string name(const Article article) const override;

        void set_inscription(const std::string& str)
        {
                m_inscr = str;
        }

        void bump(actor::Actor& actor_bumping) override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        std::string m_inscr;
};

class ChurchBench: public Rigid
{
public:
        ChurchBench(const P& p);

        ChurchBench() = delete;

        FeatureId id() const override
        {
                return FeatureId::church_bench;
        }

        std::string name(const Article article) const override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

enum class StatueType
{
        common,
        ghoul
};

class Statue: public Rigid
{
public:
        Statue(const P& p);
        Statue() = delete;

        FeatureId id() const override
        {
                return FeatureId::statue;
        }

        std::string name(const Article article) const override;

        TileId tile() const override;

        StatueType m_type;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        int base_shock_when_adj() const override;
};

class Stalagmite: public Rigid
{
public:
        Stalagmite(const P& p);
        Stalagmite() = delete;

        FeatureId id() const override
        {
                return FeatureId::stalagmite;
        }

        std::string name(const Article article) const override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class Stairs: public Rigid
{
public:
        Stairs(const P& p);
        Stairs() = delete;

        FeatureId id() const override
        {
                return FeatureId::stairs;
        }

        std::string name(const Article article) const override;

        void bump(actor::Actor& actor_bumping) override;

        void on_new_turn_hook() override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class Bridge : public Rigid
{
public:
        Bridge(const P& p) :
                Rigid(p),
                m_axis(Axis::hor) {}
        Bridge() = delete;

        FeatureId id() const override
        {
                return FeatureId::bridge;
        }

        std::string name(const Article article) const override;
        TileId tile() const override;
        char character() const override;

        void set_axis(const Axis axis)
        {
                m_axis = axis;
        }

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        Axis m_axis;
};

class LiquidShallow: public Rigid
{
public:
        LiquidShallow(const P& p);
        LiquidShallow() = delete;

        FeatureId id() const override
        {
                return FeatureId::liquid_shallow;
        }

        std::string name(const Article article) const override;

        void bump(actor::Actor& actor_bumping) override;

        LiquidType m_type;

private:
        Color color_default() const override;

        Color color_bg_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class LiquidDeep: public Rigid
{
public:
        LiquidDeep(const P& p);
        LiquidDeep() = delete;

        FeatureId id() const override
        {
                return FeatureId::liquid_deep;
        }

        std::string name(const Article article) const override;

        AllowAction pre_bump(actor::Actor& actor_bumping) override;

        void bump(actor::Actor& actor_bumping) override;

        void on_leave(actor::Actor& actor_leaving) override;

        bool can_move(const actor::Actor& actor) const override;

        LiquidType m_type;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        bool must_swim_on_enter(const actor::Actor& actor) const;
};

class Chasm: public Rigid
{
public:
        Chasm(const P& p);
        Chasm() = delete;

        FeatureId id() const override
        {
                return FeatureId::chasm;
        }

        std::string name(const Article article) const override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

class Lever: public Rigid
{
public:
        Lever(const P& p);

        Lever() = delete;

        FeatureId id() const override
        {
                return FeatureId::lever;
        }

        std::string name(const Article article) const override;

        TileId tile() const override;

        void toggle();

        void bump(actor::Actor& actor_bumping) override;

        bool is_left_pos() const
        {
                return m_is_left_pos;
        }

        bool is_linked_to(const Rigid& feature) const
        {
                return m_linked_feature == &feature;
        }

        void set_linked_feature(Rigid& feature)
        {
                m_linked_feature = &feature;
        }

        void unlink()
        {
                m_linked_feature = nullptr;
        }

        // Levers linked to the same feature
        void add_sibbling(Lever* const lever)
        {
                m_sibblings.push_back(lever);
        }

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        bool m_is_left_pos;

        Rigid* m_linked_feature;

        std::vector<Lever*> m_sibblings;
};

class Altar: public Rigid
{
public:
        Altar(const P& p);

        Altar() = delete;

        FeatureId id() const override
        {
                return FeatureId::altar;
        }

        std::string name(const Article article) const override;

        void bump(actor::Actor& actor_bumping) override;

        void disable_pact()
        {
                m_can_offer_pact = false;
        }

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        bool m_can_offer_pact {true};
};

class Tree: public Rigid
{
public:
        Tree(const P& p);
        Tree() = delete;

        FeatureId id() const override
        {
                return FeatureId::tree;
        }

        std::string name(const Article article) const override;

        WasDestroyed on_finished_burning() override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;
};

// NOTE: In some previous versions, it was possible to inspect the tomb and get
// a hint about its trait ("It has an aura of unrest", "There are foreboding
// carved signs", etc). This is currently not possible - you open the tomb and
// any "trap" it has will trigger. Therefore the TombTrait type could be
// removed, and instead an effect is just randomized when the tomb is
// opened. But it should be kept the way it is; it could be useful. Maybe some
// sort of hint will be re-implemented (e.g. via the "Detect Traps" spell).
enum class TombTrait
{
        ghost,
        other_undead,   // Zombies, Mummies, ...
        stench,         // Fumes, Ooze-type monster
        cursed,
        END
};

enum class TombAppearance
{
        common,     // Common items
        ornate,     // Minor treasure
        marvelous,  // Major treasure
        END
};

class Tomb: public Rigid
{
public:
        Tomb(const P& pos);
        Tomb() = delete;

        FeatureId id() const override
        {
                return FeatureId::tomb;
        }

        std::string name(const Article article) const override;
        TileId tile() const override;
        void bump(actor::Actor& actor_bumping) override;
        DidOpen open(actor::Actor* const actor_opening) override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        DidTriggerTrap trigger_trap(actor::Actor* const actor) override;

        void player_loot();

        bool m_is_open;
        bool m_is_trait_known;

        int m_push_lid_one_in_n;
        TombAppearance m_appearance;
        TombTrait m_trait;
};

enum class ChestMatl
{
        wood,
        iron,
        END
};

class Chest: public Rigid
{
public:
        Chest(const P& pos);
        Chest() = delete;

        FeatureId id() const override
        {
                return FeatureId::chest;
        }

        std::string name(const Article article) const override;

        TileId tile() const override;

        void bump(actor::Actor& actor_bumping) override;

        DidOpen open(actor::Actor* const actor_opening) override;

        void hit(const int dmg,
                 const DmgType dmg_type,
                 const DmgMethod dmg_method,
                 actor::Actor* const actor) override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        void player_loot();

        bool m_is_open;
        bool m_is_locked;

        ChestMatl m_matl;
};

class Cabinet: public Rigid
{
public:
        Cabinet(const P& pos);
        Cabinet() = delete;

        FeatureId id() const override
        {
                return FeatureId::cabinet;
        }

        std::string name(const Article article) const override;

        TileId tile() const override;

        void bump(actor::Actor& actor_bumping) override;

        DidOpen open(actor::Actor* const actor_opening) override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        void player_loot();

        bool m_is_open;
};

class Bookshelf: public Rigid
{
public:
        Bookshelf(const P& pos);
        Bookshelf() = delete;

        FeatureId id() const override
        {
                return FeatureId::bookshelf;
        }

        std::string name(const Article article) const override;

        TileId tile() const override;

        void bump(actor::Actor& actor_bumping) override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        void player_loot();

        bool m_is_looted;
};

class AlchemistBench: public Rigid
{
public:
        AlchemistBench(const P& pos);
        AlchemistBench() = delete;

        FeatureId id() const override
        {
                return FeatureId::alchemist_bench;
        }

        std::string name(const Article article) const override;

        TileId tile() const override;

        void bump(actor::Actor& actor_bumping) override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        void player_loot();

        bool m_is_looted;
};

enum class FountainEffect
{
        refreshing,
        xp,

        START_OF_BAD_EFFECTS,
        curse,
        disease,
        poison,
        frenzy,
        paralyze,
        blind,
        faint,
        END
};

class Fountain: public Rigid
{
public:
        Fountain(const P& pos);

        Fountain() = delete;

        FeatureId id() const override
        {
                return FeatureId::fountain;
        }

        std::string name(const Article article) const override;

        void bump(actor::Actor& actor_bumping) override;

        bool has_drinks_left() const
        {
                return m_has_drinks_left;
        }

        FountainEffect effect() const
        {
                return m_fountain_effect;
        }

        void set_effect(const FountainEffect effect)
        {
                m_fountain_effect = effect;
        }

        void bless();

        void curse();

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        FountainEffect m_fountain_effect;
        bool m_has_drinks_left;
};

class Cocoon: public Rigid
{
public:
        Cocoon(const P& pos);

        Cocoon() = delete;

        FeatureId id() const override
        {
                return FeatureId::cocoon;
        }

        std::string name(const Article article) const override;

        TileId tile() const override;

        void bump(actor::Actor& actor_bumping) override;

        DidOpen open(actor::Actor* const actor_opening) override;

private:
        Color color_default() const override;

        void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor) override;

        void player_loot();

        DidTriggerTrap trigger_trap(actor::Actor* const actor) override;

        bool m_is_trapped;
        bool m_is_open;
};

#endif // FEATURE_RIGID_HPP
