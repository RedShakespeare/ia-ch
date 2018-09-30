#ifndef FOV_HPP
#define FOV_HPP

#include "rl_utils.hpp"
#include "global.hpp"

struct FovMap
{
        // NOTE: These fields are NOT optional, even though they are pointers
        const Array2<bool>* hard_blocked {nullptr};
        const Array2<bool>* light {nullptr};
        const Array2<bool>* dark {nullptr};
};

struct LosResult
{
        LosResult() :
                is_blocked_hard(false),
                is_blocked_by_drk(false) {}

        bool is_blocked_hard;
        bool is_blocked_by_drk;
};

namespace fov
{

R fov_rect(const P& p, const P& map_dims);

bool is_in_fov_range(const P& p0, const P& p1);

LosResult check_cell(
        const P& p0,
        const P& p1,
        const FovMap& map);

Array2<LosResult> run(const P& p0, const FovMap& map);

} // fov

#endif // FOV_HPP
