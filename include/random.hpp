// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <random>
#include <algorithm>
#include <string>
#include <vector>

#include "debug.hpp"

struct Dice
{
        Dice() {}

        Dice(const int rolls, const int sides, const int plus = 0) :
                rolls(rolls),
                sides(sides),
                plus(plus) {}

        Dice(const Dice& other) :
                rolls(other.rolls),
                sides(other.sides),
                plus(other.plus) {}

        Dice& operator=(const Dice& other)
        {
                rolls = other.rolls;
                sides = other.sides;
                plus  = other.plus;
                return *this;
        }

        bool operator==(const Dice& other) const
        {
                return
                        (rolls == other.rolls) &&
                        (sides == other.sides) &&
                        (plus == other.plus);
        }

        bool operator!=(const Dice& other) const
        {
                return !(*this == other);
        }

        int max() const
        {
                return (rolls * sides) + plus;
        }

        int min() const
        {
                return (rolls + plus);
        }

        double avg() const
        {
                const double roll_avg = ((double)sides + 1.0) / 2.0;

                const double roll_avg_tot = roll_avg * (double)rolls;

                return roll_avg_tot + (double)plus;
        }

        int roll() const;

        std::string str() const;

        std::string str_plus() const;

        std::string str_avg() const;

        int rolls {0};
        int sides {0};
        int plus {0};
};

struct Range
{
        Range() :
                min(-1),
                max(-1) {}

        Range(const int min, const int max) :
                min(min),
                max(max) {}

        Range(const Range& other) :
                Range(other.min, other.max) {}

        int len() const
        {
                return max - min + 1;
        }

        bool is_in_range(const int v) const
        {
                return
                        (v >= min) &&
                        (v <= max);
        }

        void set(const int min_val, const int max_val)
        {
                min = min_val;
                max = max_val;
        }

        Range& operator/=(const int v)
        {
                min /= v;
                max /= v;
                return *this;
        }

        int roll() const;

        std::string str() const;

        int min, max;
};

struct Fraction
{
        Fraction() :
                num(-1),
                den(-1) {}

        Fraction(const int num, const int den) :
                num(num),
                den(den) {}

        void set(const int num, const int den)
        {
                this->num = num;
                this->den = den;
        }

        Fraction& operator=(const Fraction& other)
        {
                num = other.num;
                den = other.den;

                return *this;
        }

        bool roll() const;

        int num, den;
};

template <typename T>
struct WeightedItems
{
        std::vector<T> items = {};
        std::vector<int> weights = {};
};

//------------------------------------------------------------------------------
// Random number generation
//------------------------------------------------------------------------------
namespace rnd
{

extern std::mt19937 g_rng;

void seed();

void seed(uint32_t seed);

// NOTE: If not called with a positive non-zero number of sides, this will
// always return zero.
int dice(const int rolls, const int sides);

bool coin_toss();

bool fraction(const int num, const int den);

bool one_in(const int N);

// Can be called with any range (positive or negative), V2 does *not* have to be
// bigger than V1.
int range(const int v1, const int v2);

// NOTE: "p" shall be within [0.0, 1.0]
int range_binom(const int v1, const int v2, const double p);

bool percent(const int pct_chance);

int weighted_choice(const std::vector<int> weights);

template <typename T>
T weighted_choice(const WeightedItems<T>& weighted_items)
{
        ASSERT(weighted_items.items.size() == weighted_items.weights.size());

        ASSERT(weighted_items.items.size() > 0);

        const size_t idx = weighted_choice(weighted_items.weights);

        return weighted_items.items[idx];
}

template <typename T>
T element(const std::vector<T>& v)
{
        const size_t idx = range(0, v.size() - 1);

        return v[idx];
}

template <typename T>
size_t idx(const std::vector<T>& v)
{
        return range(0, v.size() - 1);
}

template <typename T>
void shuffle(std::vector<T>& v)
{
        std::shuffle(std::begin(v), std::end(v), g_rng);
}

} // rnd

#endif // RANDOM_HPP
