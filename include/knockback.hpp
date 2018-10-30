#ifndef KNOCKBACK_HPP
#define KNOCKBACK_HPP

#include "global.hpp"

struct P;
class Actor;

namespace knockback
{

void run(Actor& defender,
         const P& attacked_from_pos,
         const bool is_spike_gun,
         const Verbosity verbosity = Verbosity::verbose,
         const int paralyze_extra_turns = 0);

} // knockback

#endif
