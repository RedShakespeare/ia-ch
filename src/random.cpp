// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "random.hpp"

#include <iomanip>
#include <sstream>

#include "debug.hpp"

int Dice::roll() const
{
        return rnd::dice(rolls, sides) + plus;
}

std::string Dice::str() const
{
        const std::string rolls_str = std::to_string(rolls);

        const std::string sides_str = std::to_string(sides);

        const std::string plus_str = str_plus();

        return rolls_str + "d" + sides_str + plus_str;
}

std::string Dice::str_plus() const
{
        if (plus == 0)
        {
                return "";
        }
        else if (plus > 0)
        {
                return "+" + std::to_string(plus);
        }
        else
        {
                return "-" + std::to_string(plus);
        }
}

std::string Dice::str_avg() const
{
        const double val = avg();

        double rounded = roundf(val * 100.0) / 100.0;

        std::ostringstream ss;

        ss << std::fixed << std::setprecision(1) << rounded;

        return ss.str();
}

std::string Range::str() const
{
        const int min_actual = std::min(min, max);
        const int max_actual = std::max(min, max);

        return
                std::to_string(min_actual) +
                "-" +
                std::to_string(max_actual);
}

int Range::roll() const
{
        return rnd::range(min, max);
}

bool Fraction::roll() const
{
        return rnd::fraction(num, den);
}

namespace rnd
{

std::mt19937 rng;

void seed()
{
        uint32_t t = static_cast<uint32_t>(time(nullptr));

        std::hash<uint32_t> hasher;

        size_t hashed = hasher(t);

        seed(static_cast<uint32_t>(hashed));
}

void seed(uint32_t seed)
{
        rng.seed(static_cast<std::mt19937::result_type>(seed));
}

int range(const int v1, const int v2)
{
        const int min = std::min(v1, v2);
        const int max = std::max(v1, v2);

        std::uniform_int_distribution<int> dist(min, max);

        return dist(rng);
}

int range_binom(const int v1, const int v2, const double p)
{
        const int min = std::min(v1, v2);
        const int max = std::max(v1, v2);

        const int upper_random_value = max - min;

        std::binomial_distribution<std::mt19937::result_type>
                dist(upper_random_value, p);

        const int random_value = dist(rng);

        return min + random_value;
}

int dice(const int rolls, const int sides)
{
        if (sides <= 0)
        {
                return 0;
        }

        if (sides == 1)
        {
                return rolls * sides;
        }

        int result = 0;

        const Range roll_range(1, sides);

        for (int i = 0; i < rolls; ++i)
        {
                result += roll_range.roll();
        }

        return result;
}

bool coin_toss()
{
        return range(1, 2) == 2;
}

bool fraction(const int num, const int den)
{
        // Debug mode checks

        // This function should never be called with a denominator less than
        // one, since it's unclear what e.g. "N times in -1" would mean.
        ASSERT(den >= 1);

        // If the numerator is bigger than the denominator, it's likely a bug
        // (should something occur e.g. 5 times in 3 ???) - don't allow this...
        ASSERT(num <= den);

        // A negative numerator is of course nonsense
        ASSERT(num >= 0);

        // If any of the rules above are broken on a release build, we try to
        // perform the action that was probably intended.

        // NOTE: A numerator of 0 is allowed (it simply means "no chance")

        if ((num <= 0) || (den <= 0))
        {
                return false;
        }

        if ((num >= den) || (den == 1))
        {
                return true;
        }

        return range(1, den) <= num;
}

bool one_in(const int N)
{
        return fraction(1, N);
}

bool percent(const int pct_chance)
{
        return pct_chance >= range(1, 100);
}

int weighted_choice(const std::vector<int> weights)
{
        ASSERT(!weights.empty());

#ifndef NDEBUG
        for (const int weight : weights)
        {
                ASSERT(weight > 0);
        }
#endif // NDEBUG

        const int sum = std::accumulate(begin(weights), end(weights), 0);

        int rnd = rnd::range(0, sum - 1);

        for (size_t i = 0; i < weights.size(); ++i)
        {
                const int weight = weights[i];

                if (rnd < weight)
                {
                        return i;
                }

                rnd -= weight;
        }

        // This point should never be reached
        ASSERT(false);

        return 0;
}

} // rnd
