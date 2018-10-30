#ifndef FLOOD_HPP
#define FLOOD_HPP

#include "array2.hpp"

Array2<int> floodfill(
        const P& p0,
        const Array2<bool>& blocked,
        int travel_lmt = -1,
        const P& p1 = P(-1, -1),
        const bool allow_diagonal = true);

#endif // FLOOD_HPP
