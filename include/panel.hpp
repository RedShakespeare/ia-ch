#ifndef PANEL_HPP
#define PANEL_HPP

struct PxPos;
struct R;
struct P;

// TODO: Add *_border for other panels than log as well
enum class Panel
{
        screen,
        map,
        player_stats,
        log,
        log_border,
        create_char_menu,
        create_char_descr,
        item_menu,
        item_descr,
        END
};

namespace panels
{

void init(P max_gui_dims);

bool is_valid();

R area(const Panel panel);

P dims(const Panel panel);

P p0(const Panel panel);

P p1(const Panel panel);

int x0(const Panel panel);

int y0(const Panel panel);

int x1(const Panel panel);

int y1(const Panel panel);

int w(const Panel panel);

int h(const Panel panel);

int center_x(const Panel panel);

int center_y(const Panel panel);

P center(const Panel panel);

} // panels

#endif // PANEL_HPP
