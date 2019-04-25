// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef TERRAIN_HPP
#define TERRAIN_HPP

#include <memory>
#include <vector>

#include "colors.hpp"
#include "terrain_data.hpp"
#include "gfx.hpp"
#include "pos.hpp"


namespace actor
{
class Actor;
}

namespace item
{
class Item;
}

template<typename T>
class Array2;

enum class AllowAction;
enum class Article;
enum class DmgMethod;
enum class DmgType;
enum class Matl;
enum class Verbosity;


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


namespace terrain
{

class Terrain;
enum class Id;
struct TerrainData;
class Lever;


class ItemContainer
{
public:
        ItemContainer();

        ~ItemContainer();

        void init(const Id terrain_id, const int nr_items_to_attempt);

        void open(const P& terrain_pos, actor::Actor* const actor_opening);

        void destroy_single_fragile();

        std::vector<item::Item*> m_items;
};


class Terrain
{
public:
        Terrain(const P& p) :
                m_pos(p) {}

        Terrain() = delete;

        virtual ~Terrain() {}

        virtual Id id() const = 0;

        const TerrainData& data() const
        {
                return ::terrain::data(id());
        }

        P pos() const
        {
                return m_pos;
        }

        virtual std::string name(const Article article) const = 0;

        virtual Color color() const;

        virtual Color color_bg() const;

        virtual bool is_walkable() const
        {
                return data().move_rules.is_walkable();
        }

        virtual bool can_move(const actor::Actor& actor) const
        {
                return data().move_rules.can_move(actor);
        }

        virtual bool is_sound_passable() const
        {
                return data().is_sound_passable;
        }

        virtual bool is_los_passable() const
        {
                return data().is_los_passable;
        }

        virtual bool is_projectile_passable() const
        {
                return data().is_projectile_passable;
        }

        virtual bool is_smoke_passable() const
        {
                return data().is_smoke_passable;
        }

        virtual char character() const
        {
                return data().character;
        }

        virtual TileId tile() const
        {
                return data().tile;
        }

        virtual bool can_have_corpse() const
        {
                return data().can_have_corpse;
        }

        virtual bool can_have_terrain() const
        {
                return data().can_have_terrain;
        }

        virtual bool can_have_blood() const
        {
                return data().can_have_blood;
        }

        virtual bool can_have_gore() const
        {
                return data().can_have_gore;
        }

        virtual bool can_have_item() const
        {
                return data().can_have_item;
        }

        virtual Matl matl() const
        {
                return data().matl_type;
        }

        bool is_hidden() const
        {
                return m_is_hidden;
        }

        void try_put_gore();

        void make_bloody()
        {
                m_is_bloody = true;
        }

        TileId gore_tile() const
        {
                return m_gore_tile;
        }

        char gore_character() const
        {
                return m_gore_character;
        }

        void clear_gore();

        void corrupt_color();

        virtual void on_placed()
        {

        }

        virtual void on_new_turn();

        virtual void hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor = nullptr);

        virtual void reveal(const Verbosity verbosity)
        {
                (void)verbosity;
        }

        virtual void on_revealed_from_searching() {}

        virtual AllowAction pre_bump(actor::Actor& actor_bumping);

        virtual void bump(actor::Actor& actor_bumping);

        virtual void on_leave(actor::Actor& actor_leaving)
        {
                (void)actor_leaving;
        }

        virtual DidOpen open(actor::Actor* const actor_opening)
        {
                (void)actor_opening;

                return DidOpen::no;
        }

        virtual DidClose close(actor::Actor* const actor_closing)
        {
                (void)actor_closing;

                return DidClose::no;
        }

        virtual void on_lever_pulled(Lever* const lever)
        {
                (void)lever;
        }

        int shock_when_adj() const;

        virtual void add_light(Array2<bool>& light) const;

        ItemContainer m_item_container {};

        BurnState m_burn_state {BurnState::not_burned};

        bool m_started_burning_this_turn {false};

protected:
        virtual void on_new_turn_hook() {}

        virtual void on_hit(
                const int dmg,
                const DmgType dmg_type,
                const DmgMethod dmg_method,
                actor::Actor* const actor)
        {
                (void)dmg;
                (void)dmg_type;
                (void)dmg_method;
                (void)actor;
        }

        virtual Color color_default() const
        {
                return colors::white();
        }

        virtual Color color_bg_default() const;

        void try_start_burning(const bool is_msg_allowed);

        virtual WasDestroyed on_finished_burning();

        virtual DidTriggerTrap trigger_trap(actor::Actor* const actor)
        {
                (void)actor;

                return DidTriggerTrap::no;
        }

        virtual void add_light_hook(Array2<bool>& light) const
        {
                (void)light;
        }

        virtual int base_shock_when_adj() const;

        bool m_is_hidden {false};
        TileId m_gore_tile {TileId::END};
        char m_gore_character {0};
        P m_pos;

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

class Floor: public Terrain
{
public:
        Floor(const P& p);

        Floor() = delete;

        Id id() const override
        {
                return Id::floor;
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

class Carpet: public Terrain
{
public:
        Carpet(const P& p);

        Carpet() = delete;

        Id id() const override
        {
                return Id::carpet;
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

class Grass: public Terrain
{
public:
        Grass(const P& p);

        Grass() = delete;

        Id id() const override
        {
                return Id::grass;
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

class Bush: public Terrain
{
public:
        Bush(const P& p);

        Bush() = delete;

        Id id() const override
        {
                return Id::bush;
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

class Vines: public Terrain
{
public:
        Vines(const P& p);

        Vines() = delete;

        Id id() const override
        {
                return Id::vines;
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

class Chains: public Terrain
{
public:
        Chains(const P& p);

        Chains() = delete;

        Id id() const override
        {
                return Id::chains;
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

class Grate: public Terrain
{
public:
        Grate(const P& p);

        Grate() = delete;

        Id id() const override
        {
                return Id::grate;
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

class Brazier: public Terrain
{
public:
        Brazier(const P& p) : Terrain(p) {}

        Brazier() = delete;

        Id id() const override
        {
                return Id::brazier;
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

class Wall: public Terrain
{
public:
        Wall(const P& p);

        Wall() = delete;

        Id id() const override
        {
                return Id::wall;
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

class RubbleLow: public Terrain
{
public:
        RubbleLow(const P& p);

        RubbleLow() = delete;

        Id id() const override
        {
                return Id::rubble_low;
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

class Bones: public Terrain
{
public:
        Bones(const P& p);

        Bones() = delete;

        Id id() const override
        {
                return Id::bones;
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

class RubbleHigh: public Terrain
{
public:
        RubbleHigh(const P& p);

        RubbleHigh() = delete;

        Id id() const override
        {
                return Id::rubble_high;
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

class GraveStone: public Terrain
{
public:
        GraveStone(const P& p);

        GraveStone() = delete;

        Id id() const override
        {
                return Id::gravestone;
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

class ChurchBench: public Terrain
{
public:
        ChurchBench(const P& p);

        ChurchBench() = delete;

        Id id() const override
        {
                return Id::church_bench;
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

class Statue: public Terrain
{
public:
        Statue(const P& p);
        Statue() = delete;

        Id id() const override
        {
                return Id::statue;
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

class Stalagmite: public Terrain
{
public:
        Stalagmite(const P& p);
        Stalagmite() = delete;

        Id id() const override
        {
                return Id::stalagmite;
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

class Stairs: public Terrain
{
public:
        Stairs(const P& p);
        Stairs() = delete;

        Id id() const override
        {
                return Id::stairs;
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

class Bridge : public Terrain
{
public:
        Bridge(const P& p) :
                Terrain(p),
                m_axis(Axis::hor) {}
        Bridge() = delete;

        Id id() const override
        {
                return Id::bridge;
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

class LiquidShallow: public Terrain
{
public:
        LiquidShallow(const P& p);
        LiquidShallow() = delete;

        Id id() const override
        {
                return Id::liquid_shallow;
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

class LiquidDeep: public Terrain
{
public:
        LiquidDeep(const P& p);
        LiquidDeep() = delete;

        Id id() const override
        {
                return Id::liquid_deep;
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

class Chasm: public Terrain
{
public:
        Chasm(const P& p);
        Chasm() = delete;

        Id id() const override
        {
                return Id::chasm;
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

class Lever: public Terrain
{
public:
        Lever(const P& p);

        Lever() = delete;

        Id id() const override
        {
                return Id::lever;
        }

        std::string name(const Article article) const override;

        TileId tile() const override;

        void toggle();

        void bump(actor::Actor& actor_bumping) override;

        bool is_left_pos() const
        {
                return m_is_left_pos;
        }

        bool is_linked_to(const Terrain& terrain) const
        {
                return m_linked_terrain == &terrain;
        }

        void set_linked_terrain(Terrain& terrain)
        {
                m_linked_terrain = &terrain;
        }

        void unlink()
        {
                m_linked_terrain = nullptr;
        }

        // Levers linked to the same terrain
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

        Terrain* m_linked_terrain;

        std::vector<Lever*> m_sibblings;
};

class Altar: public Terrain
{
public:
        Altar(const P& p);

        Altar() = delete;

        Id id() const override
        {
                return Id::altar;
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

class Tree: public Terrain
{
public:
        Tree(const P& p);
        Tree() = delete;

        Id id() const override
        {
                return Id::tree;
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

class Tomb: public Terrain
{
public:
        Tomb(const P& pos);
        Tomb() = delete;

        Id id() const override
        {
                return Id::tomb;
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

class Chest: public Terrain
{
public:
        Chest(const P& pos);
        Chest() = delete;

        Id id() const override
        {
                return Id::chest;
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

class Cabinet: public Terrain
{
public:
        Cabinet(const P& pos);
        Cabinet() = delete;

        Id id() const override
        {
                return Id::cabinet;
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

class Bookshelf: public Terrain
{
public:
        Bookshelf(const P& pos);
        Bookshelf() = delete;

        Id id() const override
        {
                return Id::bookshelf;
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

class AlchemistBench: public Terrain
{
public:
        AlchemistBench(const P& pos);
        AlchemistBench() = delete;

        Id id() const override
        {
                return Id::alchemist_bench;
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

class Fountain: public Terrain
{
public:
        Fountain(const P& pos);

        Fountain() = delete;

        Id id() const override
        {
                return Id::fountain;
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

class Cocoon: public Terrain
{
public:
        Cocoon(const P& pos);

        Cocoon() = delete;

        Id id() const override
        {
                return Id::cocoon;
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

} // terrain

#endif // TERRAIN_HPP
