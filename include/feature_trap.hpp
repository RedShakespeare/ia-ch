// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef FEATURE_TRAPS_HPP
#define FEATURE_TRAPS_HPP

#include "feature.hpp"
#include "ability_values.hpp"
#include "gfx.hpp"
#include "feature_rigid.hpp"
#include "global.hpp"

class TrapImpl;

enum class TrapId
{
        // Mechanical traps
        blinding,
        deafening,
        dart,
        gas_confusion,
        gas_fear,
        gas_paralyze,
        smoke,
        fire,
        alarm,
        spear,
        web,

        // Magical traps
        teleport,
        summon,
        spi_drain,
        slow,
        curse,

        END,

        any
};

enum class TrapPlacementValid
{
        no,
        yes
};

class Trap: public Rigid
{
public:
        Trap(const P& p, Rigid* const mimic_feature, TrapId id);

        Trap(const P& p) :
                Rigid(p) {}

        Trap() = delete;

        ~Trap();

        FeatureId id() const override
        {
                return FeatureId::trap;
        }

        bool valid()
        {
                // Trap is valid if we have succesfully created an
                // implementation
                return m_trap_impl;
        }

        AllowAction pre_bump(Actor& actor_bumping) override;

        void bump(Actor& actor_bumping) override;

        char character() const override;

        TileId tile() const override;

        std::string name(const Article article) const override;

        void disarm();

        // Quietly destroys the trap, and either places rubble, or replaces it
        // with the mimic feature (depending on trap type)
        void destroy();

        void on_new_turn_hook() override;

        bool can_have_blood() const override
        {
                return m_is_hidden;
        }

        bool can_have_gore() const override
        {
                return m_is_hidden;
        }

        bool is_magical() const;

        void reveal(const Verbosity verbosity) override;

        bool is_hidden() const
        {
                return m_is_hidden;
        }

        Matl matl() const override;

        TrapId type() const;

        const TrapImpl* trap_impl() const
        {
                return m_trap_impl;
        }

        void player_try_spot_hidden();

private:
        TrapImpl* make_trap_impl_from_id(const TrapId trap_id);

        Color color_default() const override;
        Color color_bg_default() const override;

        void on_hit(const int dmg,
                    const DmgType dmg_type,
                    const DmgMethod dmg_method,
                    Actor* const actor) override;

        DidTriggerTrap trigger_trap(Actor* const actor) override;

        void trigger_start(const Actor* actor);

        Rigid* m_mimic_feature {nullptr};
        bool m_is_hidden {false};
        int m_nr_turns_until_trigger {-1};

        // TODO: Should be a unique pointer
        TrapImpl* m_trap_impl {nullptr};
};

class TrapImpl
{
protected:
        friend class Trap;
        TrapImpl(P p, TrapId type, Trap* const base_trap) :
                m_pos(p),
                m_type(type),
                m_base_trap(base_trap) {}

        virtual ~TrapImpl() {}

        // Called by the trap feature after picking a random trap
        // implementation. This allows the specific implementation initialize
        // and to modify the map. The implementation may report that the
        // placement is impossible (e.g. no suitable wall to fire a dart from),
        // in which case another implementation will be picked at random.
        virtual TrapPlacementValid on_place()
        {
                return TrapPlacementValid::yes;
        }

        // NOTE: The trigger may happen several turns after the trap activates,
        // so it's pointless to provide actor triggering as a parameter here.
        virtual void trigger() = 0;

        virtual Range nr_turns_range_to_trigger() const = 0;

        virtual std::string name(const Article article) const = 0;

        virtual Color color() const = 0;

        virtual TileId tile() const = 0;

        virtual char character() const
        {
                return '^';
        }

        virtual bool is_magical() const = 0;

        virtual bool is_disarmable() const
        {
                return true;
        }

        virtual std::string disarm_msg() const = 0;

        P m_pos;

        TrapId m_type;

        P m_dart_origin_pos;

        Trap* const m_base_trap;
};

class MechTrapImpl : public TrapImpl
{
protected:
        friend class Trap;

        MechTrapImpl(P pos, TrapId type, Trap* const base_trap) :
                TrapImpl(pos, type, base_trap) {}

        virtual ~MechTrapImpl() {}

        virtual TileId tile() const override
        {
                return TileId::trap_general;
        }

        bool is_magical() const override
        {
                return false;
        }

        std::string disarm_msg() const override
        {
                return "I disarm a trap.";
        }
};

class TrapDart: public MechTrapImpl
{
private:
        friend class Trap;

        TrapDart(P pos, Trap* const base_trap);

        virtual std::string name(const Article article) const override
        {
                std::string name =
                        (article == Article::a) ?
                        "a" :
                        "the";

                name += " dart trap";

                return name;
        }

        virtual Color color() const override
        {
                return colors::white();
        }

        void trigger() override;

        TrapPlacementValid on_place() override;

        Range nr_turns_range_to_trigger() const override
        {
                return {2, 3};
        }

        bool m_is_poisoned;

        P m_dart_origin;

        bool m_is_dart_origin_destroyed;
};

class TrapSpear: public MechTrapImpl
{
private:
        friend class Trap;

        TrapSpear(P pos, Trap* const base_trap);

        virtual std::string name(const Article article) const override
        {
                std::string name =
                        (article == Article::a) ?
                        "a" :
                        "the";

                name += " spear trap";

                return name;
        }

        virtual Color color() const override
        {
                return colors::light_white();
        }

        void trigger() override;

        TrapPlacementValid on_place() override;

        Range nr_turns_range_to_trigger() const override
        {
                return {2, 3};
        }

        bool m_is_poisoned;

        P m_spear_origin;

        bool m_is_spear_origin_destroyed;
};

class GasTrapImpl: public MechTrapImpl
{
protected:
        friend class Trap;

        GasTrapImpl(P pos, TrapId type, Trap* const base_trap) :
                MechTrapImpl(pos, type, base_trap) {}

        virtual std::string name(const Article article) const override
        {
                std::string name =
                        (article == Article::a) ?
                        "a" :
                        "the";

                name += " gas trap";

                return name;
        }

        virtual Color color() const override
        {
                return colors::magenta();
        }

        Range nr_turns_range_to_trigger() const override
        {
                return {1, 4};
        }
};

class TrapGasConfusion: public GasTrapImpl
{
private:
        friend class Trap;

        TrapGasConfusion(P pos, Trap* const base_trap) :
                GasTrapImpl(pos, TrapId::gas_confusion, base_trap) {}

        void trigger() override;
};

class TrapGasParalyzation: public GasTrapImpl
{
private:
        friend class Trap;

        TrapGasParalyzation(P pos, Trap* const base_trap) :
                GasTrapImpl(pos, TrapId::gas_paralyze, base_trap) {}

        void trigger() override;
};

class TrapGasFear: public GasTrapImpl
{
private:
        friend class Trap;

        TrapGasFear(P pos, Trap* const base_trap) :
                GasTrapImpl(pos, TrapId::gas_fear, base_trap) {}

        void trigger() override;
};

class TrapBlindingFlash: public MechTrapImpl
{
private:
        friend class Trap;

        TrapBlindingFlash(P pos, Trap* const base_trap) :
                MechTrapImpl(pos, TrapId::blinding, base_trap) {}

        virtual std::string name(const Article article) const override
        {
                std::string name =
                        (article == Article::a) ?
                        "a" :
                        "the";

                name += " blinding trap";

                return name;
        }

        virtual Color color() const override
        {
                return colors::yellow();
        }

        void trigger() override;

        Range nr_turns_range_to_trigger() const override
        {
                return {1, 3};
        }
};

class TrapDeafening: public MechTrapImpl
{
private:
        friend class Trap;

        TrapDeafening(P pos, Trap* const base_trap) :
                MechTrapImpl(pos, TrapId::deafening, base_trap) {}

        virtual std::string name(const Article article) const override
        {
                std::string name =
                        (article == Article::a) ?
                        "a" :
                        "the";

                name += " deafening trap";

                return name;
        }

        virtual Color color() const override
        {
                return colors::violet();
        }

        void trigger() override;

        Range nr_turns_range_to_trigger() const override
        {
                return {1, 3};
        }
};

class TrapSmoke: public MechTrapImpl
{
private:
        friend class Trap;

        TrapSmoke(P pos, Trap* const base_trap) :
                MechTrapImpl(pos, TrapId::smoke, base_trap) {}

        virtual std::string name(const Article article) const override
        {
                std::string name =
                        (article == Article::a) ?
                        "a" :
                        "the";

                name += " smoke trap";

                return name;
        }

        virtual Color color() const override
        {
                return colors::gray();
        }

        void trigger() override;

        Range nr_turns_range_to_trigger() const override
        {
                return {1, 3};
        }
};

class TrapFire: public MechTrapImpl
{
private:
        friend class Trap;

        TrapFire(P pos, Trap* const base_trap) :
                MechTrapImpl(pos, TrapId::smoke, base_trap) {}

        virtual std::string name(const Article article) const override
        {
                std::string name =
                        (article == Article::a) ?
                        "a" :
                        "the";

                name += " fire trap";

                return name;
        }

        virtual Color color() const override
        {
                return colors::light_red();
        }

        void trigger() override;

        Range nr_turns_range_to_trigger() const override
        {
                return {3, 4};
        }
};

class TrapAlarm: public MechTrapImpl
{
private:
        friend class Trap;

        TrapAlarm(P pos, Trap* const base_trap) :
                MechTrapImpl(pos, TrapId::alarm, base_trap) {}

        virtual std::string name(const Article article) const override
        {
                std::string name =
                        (article == Article::a) ?
                        "an" :
                        "the";

                name += " alarm trap";

                return name;
        }

        virtual Color color() const override
        {
                return colors::orange();
        }

        void trigger() override;

        Range nr_turns_range_to_trigger() const override
        {
                return {0, 2};
        }
};

class TrapWeb: public MechTrapImpl
{
private:
        friend class Trap;

        TrapWeb(P pos, Trap* const base_trap) :
                MechTrapImpl(pos, TrapId::web, base_trap) {}

        void trigger() override;

        Color color() const override
        {
                return colors::light_white();
        }

        std::string name(const Article article) const override
        {
                std::string name =
                        (article == Article::a)
                        ? "a"
                        : "the";

                name += " spider web";

                return name;
        }

        char character() const override
        {
                return '*';
        }

        Range nr_turns_range_to_trigger() const override
        {
                return {0, 0};
        }

        bool is_magical() const override
        {
                return false;
        }

        TileId tile() const override
        {
                return TileId::web;
        }

        std::string disarm_msg() const override
        {
                return "I tear down a spider web.";
        }
};

class MagicTrapImpl : public TrapImpl
{
protected:
        friend class Trap;

        MagicTrapImpl(P pos, TrapId type, Trap* const base_trap) :
                TrapImpl(pos, type, base_trap) {}

        virtual ~MagicTrapImpl() {}

        TrapPlacementValid on_place() override;

        virtual std::string name(const Article article) const override
        {
                std::string name =
                        (article == Article::a)
                        ? "a"
                        : "the";

                name += " strange shape";

                return name;
        }

        virtual Color color() const override
        {
                return colors::light_red();
        }

        virtual TileId tile() const override
        {
                return TileId::elder_sign;
        }

        bool is_magical() const override
        {
                return true;
        }

        std::string disarm_msg() const override
        {
                return "I dispel a magic trap.";
        }

        Range nr_turns_range_to_trigger() const override
        {
                return {0, 0};
        }
};

class TrapTeleport: public MagicTrapImpl
{
private:
        friend class Trap;

        TrapTeleport(P pos, Trap* const base_trap) :
                MagicTrapImpl(pos, TrapId::teleport, base_trap) {}

        void trigger() override;
};

class TrapSummonMon: public MagicTrapImpl
{
private:
        friend class Trap;

        TrapSummonMon(P pos, Trap* const base_trap) :
                MagicTrapImpl(pos, TrapId::summon, base_trap) {}

        void trigger() override;
};

class TrapSpiDrain: public MagicTrapImpl
{
private:
        friend class Trap;

        TrapSpiDrain(P pos, Trap* const base_trap) :
                MagicTrapImpl(pos, TrapId::summon, base_trap) {}

        void trigger() override;
};

class TrapSlow: public MagicTrapImpl
{
private:
        friend class Trap;

        TrapSlow(P pos, Trap* const base_trap) :
                MagicTrapImpl(pos, TrapId::slow, base_trap) {}

        void trigger() override;
};

class TrapCurse: public MagicTrapImpl
{
private:
        friend class Trap;

        TrapCurse(P pos, Trap* const base_trap) :
                MagicTrapImpl(pos, TrapId::curse, base_trap) {}

        void trigger() override;
};

#endif // FEATURE_TRAPS_HPP
