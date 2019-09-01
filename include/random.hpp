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


struct Range
{
        Range() = default;

        Range(const int min_val, const int max_val) :
                min(min_val),
                max(max_val) {}

        Range(const Range& other) = default;

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

        int len() const
        {
                return max - min + 1;
        }

        double avg() const
        {
                return (double)(min + max) / 2.0;
        }

        bool is_in_range(const int v) const
        {
                return
                        (v >= min) &&
                        (v <= max);
        }

        std::string str() const;

        std::string str_avg() const;

        int min {0};
        int max {0};
};

struct Fraction
{
        Fraction() = default;

        Fraction(const int numerator, const int denominator) :
                num(numerator),
                den(denominator) {}

        Fraction& operator=(const Fraction& other) = default;

        bool roll() const;

        int num {-1};
        int den {-1};
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
