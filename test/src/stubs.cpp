#include "io.hpp"
#include "panel.hpp"
#include "sdl_base.hpp"

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace io
{

void init()
{
        panels::init({100, 100});
}

void cleanup() {}

void update_screen() {}

void clear_screen() {}

void on_fullscreen_toggled() {}

P min_screen_gui_dims()
{
        return {};
}

int gui_to_px_coords_x(const int)
{
        return 0;
}

int gui_to_px_coords_y(const int)
{
        return 0;
}


int map_to_px_coords_x(const int)
{
        return 0;
}

int map_to_px_coords_y(const int)
{
        return 0;
}

P gui_to_px_coords(const P)
{
        return {};
}

P gui_to_px_coords(const int, const int)
{
        return {};
}

P map_to_px_coords(const P)
{
        return {};
}

P map_to_px_coords(const int, const int)
{
        return {};
}

P px_to_gui_coords(const P)
{
        return {};
}

P px_to_map_coords(const P)
{
        return {};
}

P gui_to_map_coords(const P)
{
        return {};
}

P gui_to_px_coords(const Panel, const P)
{
        return {};
}

P map_to_px_coords(const Panel, const P)
{
        return {};
}

void draw_symbol(
        const TileId,
        const char,
        const Panel,
        const P,
        const Color&,
        const bool,
        const Color&) {}

void draw_tile(
        const TileId,
        const Panel,
        const P,
        const Color&,
        const bool,
        const Color&) {}

void draw_character(
        const char,
        const Panel,
        const P,
        const Color&,
        const bool,
        const Color&) {}

void draw_text(
        const std::string&,
        const Panel,
        const P,
        const Color&,
        const bool,
        const Color&) {}

void draw_text_center(
        const std::string&,
        const Panel,
        const P,
        const Color&,
        const bool,
        const Color&,
        const bool) {}

void draw_text_right(
        const std::string&,
        const Panel,
        const P,
        const Color&,
        const bool,
        const Color&) {}

void cover_cell(const Panel, const P) {}

void cover_panel(
        const Panel,
        const Color&) {}

void cover_area(
        const Panel,
        const R,
        const Color&) {}

void cover_area(
        const Panel,
        const P,
        const P,
        const Color&) {}

void draw_rectangle(
        const R&,
        const Color&) {}

void draw_rectangle_filled(
        const R&,
        const Color&) {}

void draw_blast_at_cells(
        const std::vector<P>&,
        const Color&) {}

void draw_blast_at_seen_cells(
        const std::vector<P>&,
        const Color&) {}

void draw_blast_at_seen_actors(
        const std::vector<Actor*>&,
        const Color&) {}

void draw_main_menu_logo() {}

void draw_skull(const P) {}

void draw_box(
        const R&,
        const Color&) {}

void draw_descr_box(const std::vector<ColoredString>&) {}

void flush_input() {}

void clear_events() {}

InputData get()
{
        InputData d = {};

        d.key = SDLK_SPACE;

        return d;
}

} // io

// -----------------------------------------------------------------------------
// sdl_base
// -----------------------------------------------------------------------------
namespace sdl_base
{

void init() {}

void cleanup() {}

void sleep(const Uint32) {}

} // sdl_base

// -----------------------------------------------------------------------------
// audio
// -----------------------------------------------------------------------------
namespace audio
{

void init() {}

void cleanup() {}

void play(const SfxId,
          const int,
          const int) {}

void play(const SfxId,
          const Dir,
          const int) {}

void try_play_amb(const int) {}

void play_music(const MusId) {}

void fade_out_music() {}

} // audio

// #include "SDL_image.h"
// #include "SDL_mixer.h"

// #include "rl_utils.hpp"

// SDL_Surface* SDL_ConvertSurface(
//         SDL_Surface* src,
//         const SDL_PixelFormat* fmt,
//         Uint32 flags)
// {
//         (void)src;
//         (void)fmt;
//         (void)flags;

//         ASSERT(false);

//         return nullptr;
// }

// void SDL_FreeSurface(SDL_Surface * surface)
// {
//         (void)surface;

//         ASSERT(false);
// }

// extern  const char* SDL_GetError()
// {
//         ASSERT(false);

//         return nullptr;
// }

// SDL_Surface* IMG_Load(const char* file)
// {
//         (void)file;

//         ASSERT(false);

//         return nullptr;
// }

// Uint32 SDL_MapRGB(
//         const SDL_PixelFormat* format,
//         Uint8 r,
//         Uint8 g,
//         Uint8 b)
// {
//         (void)format;
//         (void)r;
//         (void)g;
//         (void)b;

//         ASSERT(false);

//         return 0;
// }

// void IMG_Quit()
// {
//         ASSERT(false);
// }

// int Mix_Playing(int channel)
// {
//         (void)channel;

//         ASSERT(false);

//         return 0;
// }
