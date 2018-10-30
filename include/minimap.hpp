#ifndef MINIMAP_HPP
#define MINIMAP_HPP

#include "colors.hpp"
#include "state.hpp"

// -----------------------------------------------------------------------------
// ViewMinimap
// -----------------------------------------------------------------------------
class ViewMinimap : public State
{
public:
        ViewMinimap() :
                State() {}

        StateId id() override
        {
                return StateId::view_minimap;
        }

        void draw() override;

        void update() override;
};

// -----------------------------------------------------------------------------
// minimap
// -----------------------------------------------------------------------------
namespace minimap
{

void clear();

void update();

}

#endif // MINIMAP_HPP
