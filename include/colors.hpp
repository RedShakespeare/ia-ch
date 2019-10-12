// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef COLORS_HPP
#define COLORS_HPP

#include <string>
#include <utility>

#include "SDL_video.h"

//-----------------------------------------------------------------------------
// Color
//-----------------------------------------------------------------------------
class Color
{
public:
        Color() = default;

        Color(const Color& other) = default;

        Color(uint8_t r, uint8_t g, uint8_t b) :
                m_sdl_color({r, g, b, 0}),
                m_is_defined(true) {}

        explicit Color(const SDL_Color& sdl_color) :
                m_sdl_color(sdl_color),
                m_is_defined(true) {}

        ~Color() = default;

        Color& operator=(const Color& other) = default;

        bool operator==(const Color& other) const
        {
                return
                        m_sdl_color.r == other.m_sdl_color.r &&
                        m_sdl_color.g == other.m_sdl_color.g &&
                        m_sdl_color.b == other.m_sdl_color.b;
        }

        bool operator!=(const Color& other) const
        {
                return
                        m_sdl_color.r != other.m_sdl_color.r ||
                        m_sdl_color.g != other.m_sdl_color.g ||
                        m_sdl_color.b != other.m_sdl_color.b;
        }

        Color fraction(const double div);

        bool is_defined() const;

        void clear();

        SDL_Color sdl_color() const;

        uint8_t r() const;
        uint8_t g() const;
        uint8_t b() const;

        void set_rgb(const uint8_t r, const uint8_t g, const uint8_t b);

        void randomize_rgb(const int range);

private:
        SDL_Color m_sdl_color {0, 0, 0, 0};

        bool m_is_defined {false};
};

//-----------------------------------------------------------------------------
// colors
//-----------------------------------------------------------------------------
namespace colors
{

void init();

Color name_to_color(const std::string& name);

std::string color_to_name(const Color& color);

// Available colors
Color black();
Color extra_dark_gray();
Color dark_gray();
Color gray();
Color white();
Color light_white();
Color red();
Color light_red();
Color dark_green();
Color green();
Color light_green();
Color dark_yellow();
Color yellow();
Color blue();
Color light_blue();
Color magenta();
Color light_magenta();
Color cyan();
Color light_cyan();
Color brown();
Color dark_brown();
Color gray_brown();
Color dark_gray_brown();
Color violet();
Color dark_violet();
Color orange();
Color sepia();
Color light_sepia();
Color dark_sepia();
Color teal();
Color light_teal();
Color dark_teal();

// GUI colors (using the colors above)
Color text();
Color menu_highlight();
Color menu_dark();
Color title();
Color msg_good();
Color msg_bad();
Color msg_note();
Color mon_unaware_bg();
Color mon_allied_bg();
Color mon_temp_property_bg();

}  // namespace colors

//-----------------------------------------------------------------------------
// Colored string
//-----------------------------------------------------------------------------
struct ColoredString
{
        ColoredString() = default;

        ColoredString(std::string  the_str, const Color& the_color) :
                str(std::move(the_str)),
                color(the_color) {}

        ColoredString& operator=(const ColoredString& other) = default;

        std::string str {};
        Color color {colors::white()};
};

#endif // COLORS_HPP
