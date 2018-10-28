#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

struct P;
struct R;

namespace viewport
{

R get_map_view_area();

void focus_on(const P map_pos);

bool is_in_view(const P map_pos);

P to_view_pos(const P map_pos);

P to_map_pos(const P view_pos);

} // viewport

#endif // VIEWPORT_HPP
