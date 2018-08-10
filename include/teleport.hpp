#ifndef TELEPORT_HPP
#define TELEPORT_HPP

#include "actor.hpp"
#include "global.hpp"
#include "rl_utils.hpp"

void teleport(
        Actor& actor,
        const ShouldCtrlTele ctrl_tele = ShouldCtrlTele::if_tele_ctrl_prop);

void teleport(
        Actor& actor,
        P p,
        const Array2<bool>& blocked);

#endif // TELEPORT_HPP
