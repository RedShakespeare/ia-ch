#ifndef FEATURE_MOB_HPP
#define FEATURE_MOB_HPP

#include "feature.hpp"

class Mob: public Feature
{
public:
        Mob(const P& feature_pos) :
                Feature(feature_pos) {}

        virtual ~Mob() {}

        virtual FeatureId id() const override = 0;

        virtual std::string name(const Article article) const override = 0;

        Color color() const override = 0;

        Color color_bg() const override final
        {
                return colors::black();
        }
};

class Smoke: public Mob
{
public:
        Smoke(const P& feature_pos, const int nr_turns) :
                Mob(feature_pos),
                nr_turns_left_(nr_turns) {}

        Smoke(const P& feature_pos) :
                Mob(feature_pos),
                nr_turns_left_(-1) {}

        ~Smoke() {}

        FeatureId id() const override
        {
                return FeatureId::smoke;
        }

        std::string name(const Article article)  const override;

        Color color() const override;

        void on_new_turn() override;

protected:
        int nr_turns_left_;
};

class ForceField: public Mob
{
public:
        ForceField(const P& feature_pos, const int nr_turns) :
                Mob(feature_pos),
                nr_turns_left_(nr_turns) {}

        ForceField(const P& feature_pos) :
                Mob(feature_pos),
                nr_turns_left_(-1) {}

        ~ForceField() {}

        FeatureId id() const override
        {
                return FeatureId::force_field;
        }

        void bump(Actor& actor_bumping) override;

        void on_new_turn() override;

        std::string name(const Article article)  const override;

        Color color() const override;

protected:
        int nr_turns_left_;
};

class LitDynamite: public Mob
{
public:
        LitDynamite(const P& feature_pos, const int nr_turns) :
                Mob(feature_pos),
                nr_turns_left_(nr_turns) {}

        LitDynamite(const P& feature_pos) :
                Mob(feature_pos),
                nr_turns_left_(-1) {}

        ~LitDynamite() {}

        FeatureId id() const override
        {
                return FeatureId::lit_dynamite;
        }

        std::string name(const Article article) const override;

        Color color() const override;

        // TODO: Lit dynamite should add light on their own cell (just one cell)
        // void add_light(Array2<bool>& light) const;

        void on_new_turn() override;

private:
        int nr_turns_left_;
};

class LitFlare: public Mob
{
public:
        LitFlare(const P& feature_pos, const int nr_turns) :
                Mob(feature_pos),
                nr_turns_left_(nr_turns) {}

        LitFlare(const P& feature_pos) :
                Mob(feature_pos),
                nr_turns_left_(-1) {}

        ~LitFlare() {}

        FeatureId id() const override
        {
                return FeatureId::lit_flare;
        }

        std::string name(const Article article) const override;

        Color color() const override;

        void on_new_turn() override;

        void add_light(Array2<bool>& light) const override;

private:
        int nr_turns_left_;
};

#endif // FEATURE_MOB_HPP
