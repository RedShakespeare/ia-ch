// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "io.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <ostream>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "SDL.h"
#include "SDL_blendmode.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_filesystem.h"
#include "SDL_hints.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_stdinc.h"
#include "SDL_surface.h"
#include "SDL_timer.h"
#include "SDL_video.h"
#include "audio.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "direction.hpp"
#include "io_internal.hpp"
#include "map_render_batch.hpp"
#include "paths.hpp"
#include "state.hpp"
#include "text_format.hpp"
#include "utf8.hpp"
#include "version.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static SDL_Surface* load_surface(const std::string& path)
{
    auto* const surface = IMG_Load(path.c_str());

    if (!surface) {
        TRACE_ERROR_RELEASE
            << "Failed to load surface from path '"
            << path
            << "': "
            << IMG_GetError()
            << "\n";

        PANIC;
    }

    return surface;
}

static SDL_Surface* duplicate_surface(SDL_Surface& surface)
{
    SDL_Surface* const result =
        SDL_ConvertSurface(&surface, surface.format, 0);

    if (!result) {
        TRACE_ERROR_RELEASE
            << "Failed to duplicate surface: "
            << SDL_GetError()
            << "\n";

        PANIC;
    }

    return result;
}

static std::unordered_map<uint32_t, int> s_font_glyph_indices;
static int s_font_glyph_columns = 95;

struct FontGlyph
{
    SDL_Rect source_rect {};
    int logical_w {};
    int logical_h {};
    int advance {};
    int render_offset_x {};
    int render_offset_y {};
};

static std::array<std::optional<FontGlyph>, 128> s_font_glyphs_ascii;
static std::unordered_map<uint32_t, FontGlyph> s_font_glyphs;

static std::string font_map_path_from_img_path(const std::string& img_path)
{
    const size_t suffix_pos = img_path.find_last_of('.');

    if (suffix_pos == std::string::npos) {
        return img_path + ".json";
    }

    return img_path.substr(0, suffix_pos) + ".json";
}

static std::string_view trim_json_line(std::string_view line)
{
    const auto first = line.find_first_not_of(" \t\r\n");

    if (first == std::string_view::npos) {
        return {};
    }

    const auto last = line.find_last_not_of(" \t\r\n");

    return line.substr(first, last - first + 1);
}

static int leading_space_count(const std::string& line)
{
    const auto first = line.find_first_not_of(' ');

    if (first == std::string::npos) {
        return static_cast<int>(line.size());
    }

    return static_cast<int>(first);
}

static bool consume_json_string(std::string_view& text, std::string& out)
{
    if (text.empty() || (text.front() != '"')) {
        return false;
    }

    out.clear();
    text.remove_prefix(1);

    while (!text.empty()) {
        const char c = text.front();
        text.remove_prefix(1);

        if (c == '"') {
            return true;
        }

        if (c != '\\') {
            out.push_back(c);

            continue;
        }

        if (text.empty()) {
            return false;
        }

        const char escaped = text.front();
        text.remove_prefix(1);

        switch (escaped) {
        case '"':
        case '\\':
        case '/':
            out.push_back(escaped);

            break;

        case 'b':
            out.push_back('\b');

            break;

        case 'f':
            out.push_back('\f');

            break;

        case 'n':
            out.push_back('\n');

            break;

        case 'r':
            out.push_back('\r');

            break;

        case 't':
            out.push_back('\t');

            break;

        case 'u':
            if (text.size() < 4) {
                return false;
            }

            // Generated font maps also store a codepoint field, so the key only
            // needs to mark that a glyph object started.
            out.push_back('?');
            text.remove_prefix(4);

            break;

        default:
            return false;
        }
    }

    return false;
}

static std::optional<int> parse_json_int(const std::string& line, const std::string_view key)
{
    std::string_view text = trim_json_line(line);
    std::string parsed_key;

    if (!consume_json_string(text, parsed_key) ||
        (std::string_view(parsed_key.data(), parsed_key.size()) != key)) {
        return std::nullopt;
    }

    text = trim_json_line(text);

    if (text.empty() || (text.front() != ':')) {
        return std::nullopt;
    }

    text.remove_prefix(1);
    text = trim_json_line(text);

    int value = 0;
    const auto* begin = text.data();
    const auto* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, value);

    if (result.ec != std::errc()) {
        return std::nullopt;
    }

    std::string_view rest(result.ptr, end - result.ptr);
    rest = trim_json_line(rest);

    if (!rest.empty() && (rest != ",")) {
        return std::nullopt;
    }

    return value;
}

static std::optional<std::string> parse_json_string_value(
    const std::string& line,
    const std::string_view key)
{
    std::string_view text = trim_json_line(line);
    std::string parsed_key;

    if (!consume_json_string(text, parsed_key) ||
        (std::string_view(parsed_key.data(), parsed_key.size()) != key)) {
        return std::nullopt;
    }

    text = trim_json_line(text);

    if (text.empty() || (text.front() != ':')) {
        return std::nullopt;
    }

    text.remove_prefix(1);
    text = trim_json_line(text);

    std::string value;

    if (!consume_json_string(text, value)) {
        return std::nullopt;
    }

    text = trim_json_line(text);

    if (!text.empty() && (text != ",")) {
        return std::nullopt;
    }

    return value;
}

static std::optional<int> parse_json_codepoint(const std::string& line)
{
    const auto codepoint_value = parse_json_string_value(line, "codepoint");

    if (!codepoint_value ||
        (codepoint_value->size() < 3) ||
        (codepoint_value->compare(0, 2, "U+") != 0)) {
        return std::nullopt;
    }

    int codepoint = 0;
    const std::string_view codepoint_str(*codepoint_value);
    const std::string_view hex_digits = codepoint_str.substr(2);
    const auto result = std::from_chars(
        hex_digits.data(),
        hex_digits.data() + hex_digits.size(),
        codepoint,
        16);

    if (result.ec != std::errc()) {
        return std::nullopt;
    }

    return codepoint;
}

static bool is_json_object_end(const std::string& line)
{
    const std::string_view text = trim_json_line(line);

    return (text == "}") || (text == "},");
}

static bool parse_json_named_object_begin(
    const std::string& line,
    std::string& key)
{
    std::string_view text = trim_json_line(line);

    if (!consume_json_string(text, key)) {
        return false;
    }

    text = trim_json_line(text);

    if (text.empty() || (text.front() != ':')) {
        return false;
    }

    text.remove_prefix(1);
    text = trim_json_line(text);

    return text == "{";
}

static bool parse_json_named_array_begin(
    const std::string& line,
    std::string& key)
{
    std::string_view text = trim_json_line(line);

    if (!consume_json_string(text, key)) {
        return false;
    }

    text = trim_json_line(text);

    if (text.empty() || (text.front() != ':')) {
        return false;
    }

    text.remove_prefix(1);
    text = trim_json_line(text);

    return text == "[";
}

static bool is_json_array_end(const std::string& line)
{
    const std::string_view text = trim_json_line(line);

    return text == "]";
}

static std::optional<std::vector<int>> parse_json_int_array(
    const std::string& line)
{
    std::string_view text = trim_json_line(line);

    if (text.empty() || (text.front() != '[')) {
        return std::nullopt;
    }

    text.remove_prefix(1);

    std::vector<int> values;

    while (true) {
        text = trim_json_line(text);

        if (text.empty()) {
            return std::nullopt;
        }

        if (text.front() == ']') {
            text.remove_prefix(1);
            text = trim_json_line(text);

            if (!text.empty() && (text != ",")) {
                return std::nullopt;
            }

            return values;
        }

        int value = 0;
        const auto* begin = text.data();
        const auto* end = text.data() + text.size();
        const auto result = std::from_chars(begin, end, value);

        if (result.ec != std::errc()) {
            return std::nullopt;
        }

        values.push_back(value);
        text = std::string_view(result.ptr, end - result.ptr);
        text = trim_json_line(text);

        if (!text.empty() && (text.front() == ',')) {
            text.remove_prefix(1);
        }
    }
}

static void load_font_map(const std::string& img_path)
{
    io::clear_text_width_cache();

    s_font_glyph_indices.clear();
    s_font_glyphs.clear();
    for (auto& glyph : s_font_glyphs_ascii) {
        glyph.reset();
    }
    s_font_glyph_columns = 95;

    const std::string map_path = font_map_path_from_img_path(img_path);

    std::ifstream file(map_path);

    if (!file) {
        TRACE << "No font map found at: " << map_path << "\n";

        return;
    }

    TRACE << "Loading font map: " << map_path << "\n";
    s_font_glyph_indices.reserve(1024);
    s_font_glyphs.reserve(1024);

    enum class FontMapSection
    {
        none,
        cell,
        atlas_cell,
        compact_glyphs
    };

    FontMapSection current_section = FontMapSection::none;
    std::string current_glyph;
    std::string line;
    std::optional<int> cell_w;
    std::optional<int> cell_h;
    std::optional<int> atlas_cell_w;
    std::optional<int> atlas_cell_h;
    std::optional<int> current_codepoint;
    std::optional<int> current_index;
    std::optional<int> current_x;
    std::optional<int> current_y;
    std::optional<int> current_x_px;
    std::optional<int> current_y_px;
    std::optional<int> current_w;
    std::optional<int> current_h;
    std::optional<int> current_logical_w;
    std::optional<int> current_logical_h;
    std::optional<int> current_advance;
    std::optional<int> current_render_offset_x;
    std::optional<int> current_render_offset_y;

    const auto add_glyph = [&](
        const int codepoint,
        const int index,
        const int source_x,
        const int source_y,
        const int source_w,
        const int source_h,
        const int logical_w,
        const int logical_h,
        const int advance,
        const int render_offset_x,
        const int render_offset_y) {
        const auto codepoint_u = static_cast<uint32_t>(codepoint);
        const FontGlyph glyph {
            {source_x, source_y, source_w, source_h},
            logical_w,
            logical_h,
            advance,
            render_offset_x,
            render_offset_y};

        if (codepoint_u < s_font_glyphs_ascii.size()) {
            s_font_glyphs_ascii[codepoint_u] = glyph;
        }
        else {
            s_font_glyphs[codepoint_u] = glyph;
            s_font_glyph_indices[codepoint_u] = index;
        }
    };

    const auto finish_glyph = [&]() {
        if (current_glyph.empty()) {
            return;
        }

        int source_x = 0;
        int source_y = 0;
        int source_w = 0;
        int source_h = 0;

        const bool has_source_x = current_x_px || (current_x && cell_w);
        const bool has_source_y = current_y_px || (current_y && cell_h);
        const bool has_source_w = current_w || current_logical_w || atlas_cell_w || cell_w;
        const bool has_source_h = current_h || current_logical_h || atlas_cell_h || cell_h;

        if (has_source_x) {
            source_x = current_x_px
                ? *current_x_px
                : *current_x * (*cell_w + 1);
        }

        if (has_source_y) {
            source_y = current_y_px
                ? *current_y_px
                : *current_y * *cell_h;
        }

        if (has_source_w) {
            source_w = current_w
                ? *current_w
                : current_logical_w
                      ? *current_logical_w
                      : atlas_cell_w
                            ? *atlas_cell_w
                            : *cell_w;
        }

        if (has_source_h) {
            source_h = current_h
                ? *current_h
                : current_logical_h
                      ? *current_logical_h
                      : atlas_cell_h
                            ? *atlas_cell_h
                            : *cell_h;
        }

        if (has_source_x &&
            has_source_y &&
            has_source_w &&
            has_source_h) {
            const int logical_w = current_logical_w.value_or(config::gui_cell_px_w());
            const int logical_h = current_logical_h.value_or(config::gui_cell_px_h());

            add_glyph(
                current_codepoint.value_or(
                    static_cast<int>(
                        utf8::codepoint_at(current_glyph, 0).value_or('?'))),
                current_index.value_or(0),
                source_x,
                source_y,
                source_w,
                source_h,
                logical_w,
                logical_h,
                current_advance.value_or(logical_w),
                current_render_offset_x.value_or(0),
                current_render_offset_y.value_or(0));
        }

        current_glyph.clear();
        current_codepoint.reset();
        current_index.reset();
        current_x.reset();
        current_y.reset();
        current_x_px.reset();
        current_y_px.reset();
        current_w.reset();
        current_h.reset();
        current_logical_w.reset();
        current_logical_h.reset();
        current_advance.reset();
        current_render_offset_x.reset();
        current_render_offset_y.reset();
    };

    while (std::getline(file, line)) {
        if (const auto columns_value = parse_json_int(line, "columns")) {
            s_font_glyph_columns = *columns_value;

            continue;
        }

        std::string object_key;

        if (parse_json_named_object_begin(line, object_key)) {
            const int indent = leading_space_count(line);

            if ((indent == 2) && (object_key == "cell")) {
                current_section = FontMapSection::cell;
            }
            else if ((indent == 2) && (object_key == "atlas_cell")) {
                current_section = FontMapSection::atlas_cell;
            }
            else if (indent == 4) {
                finish_glyph();
                current_glyph = object_key;
            }

            continue;
        }

        if (parse_json_named_array_begin(line, object_key)) {
            const int indent = leading_space_count(line);

            if ((indent == 2) && (object_key == "glyphs")) {
                finish_glyph();
                current_section = FontMapSection::compact_glyphs;
            }

            continue;
        }

        if (current_section != FontMapSection::none) {
            if (is_json_object_end(line)) {
                current_section = FontMapSection::none;
            }
            else if (
                current_section == FontMapSection::compact_glyphs &&
                is_json_array_end(line)) {
                current_section = FontMapSection::none;
            }
            else if (current_section == FontMapSection::compact_glyphs) {
                const auto values = parse_json_int_array(line);

                if (values && (values->size() >= 11)) {
                    add_glyph(
                        values->at(0),
                        values->at(1),
                        values->at(2),
                        values->at(3),
                        values->at(4),
                        values->at(5),
                        values->at(6),
                        values->at(7),
                        values->at(8),
                        values->at(9),
                        values->at(10));
                }
            }
            else if (const auto width_value = parse_json_int(line, "width")) {
                if (current_section == FontMapSection::cell) {
                    cell_w = *width_value;
                }
                else {
                    atlas_cell_w = *width_value;
                }
            }
            else if (const auto height_value = parse_json_int(line, "height")) {
                if (current_section == FontMapSection::cell) {
                    cell_h = *height_value;
                }
                else {
                    atlas_cell_h = *height_value;
                }
            }

            continue;
        }

        if (current_glyph.empty()) {
            continue;
        }

        if (const auto index_value = parse_json_int(line, "index")) {
            current_index = *index_value;

            continue;
        }

        if (const auto codepoint_value = parse_json_codepoint(line)) {
            current_codepoint = *codepoint_value;
        }
        else if (const auto x_value = parse_json_int(line, "x")) {
            current_x = *x_value;
        }
        else if (const auto y_value = parse_json_int(line, "y")) {
            current_y = *y_value;
        }
        else if (const auto x_px_value = parse_json_int(line, "x_px")) {
            current_x_px = *x_px_value;
        }
        else if (const auto y_px_value = parse_json_int(line, "y_px")) {
            current_y_px = *y_px_value;
        }
        else if (const auto width_value = parse_json_int(line, "width")) {
            current_w = *width_value;
        }
        else if (const auto height_value = parse_json_int(line, "height")) {
            current_h = *height_value;
        }
        else if (const auto logical_width_value = parse_json_int(line, "logical_width")) {
            current_logical_w = *logical_width_value;
        }
        else if (const auto logical_height_value = parse_json_int(line, "logical_height")) {
            current_logical_h = *logical_height_value;
        }
        else if (const auto advance_value = parse_json_int(line, "advance")) {
            current_advance = *advance_value;
        }
        else if (const auto render_offset_x_value = parse_json_int(line, "render_offset_x")) {
            current_render_offset_x = *render_offset_x_value;
        }
        else if (const auto render_offset_y_value = parse_json_int(line, "render_offset_y")) {
            current_render_offset_y = *render_offset_y_value;
        }
        else if (is_json_object_end(line)) {
            finish_glyph();
        }
    }

    finish_glyph();
}

static uint32_t glyph_codepoint(const std::string& glyph)
{
    return utf8::codepoint_at(glyph, 0).value_or('?');
}

static int glyph_index(const uint32_t codepoint)
{
    if ((codepoint >= ' ') && (codepoint <= '~')) {
        return static_cast<int>(codepoint) - ' ';
    }

    const auto it = s_font_glyph_indices.find(codepoint);

    if (it != std::end(s_font_glyph_indices)) {
        return it->second;
    }

    return '?' - ' ';
}

static const FontGlyph* glyph_metadata(const uint32_t codepoint)
{
    if (codepoint < s_font_glyphs_ascii.size()) {
        const auto& glyph = s_font_glyphs_ascii[codepoint];

        if (glyph) {
            return &(*glyph);
        }
    }

    const auto it = s_font_glyphs.find(codepoint);

    if (it != std::end(s_font_glyphs)) {
        return &it->second;
    }

    return nullptr;
}

namespace io
{
GlyphDrawData glyph_draw_data(const uint32_t codepoint)
{
    if (const auto* const metadata = glyph_metadata(codepoint)) {
        return {
            metadata->source_rect,
            metadata->logical_w,
            metadata->logical_h,
            metadata->advance,
            metadata->render_offset_x,
            metadata->render_offset_y};
    }

    const P gui_cell_px_dims(config::gui_cell_px_w(), config::gui_cell_px_h());
    const int glyph_idx = glyph_index(codepoint);

    P char_px_pos(
        glyph_idx % s_font_glyph_columns,
        glyph_idx / s_font_glyph_columns);

    char_px_pos.x *= (gui_cell_px_dims.x + 1);
    char_px_pos.y *= gui_cell_px_dims.y;

    return {
        {
            char_px_pos.x,
            char_px_pos.y,
            gui_cell_px_dims.x,
            gui_cell_px_dims.y},
        gui_cell_px_dims.x,
        gui_cell_px_dims.y,
        std::max(1, gui_cell_px_dims.x),
        0,
        0};
}
}  // namespace io

static void swap_surface_color(
    SDL_Surface& surface,
    const Color& color_before,
    const Color& color_after)
{
    for (int x = 0; x < surface.w; ++x) {
        for (int y = 0; y < surface.h; ++y) {
            const P p(x, y);

            const auto color = io::read_px_on_surface(surface, p);

            if (color == color_before) {
                io::put_px_on_surface(surface, p, color_after);
            }
        }
    }
}

static bool should_put_contour_at(
    const SDL_Surface& surface,
    const P& surface_px_pos,
    const R& surface_px_rect,
    const Color& bg_color,
    const Color& contour_color)
{
    // Only allow drawing a contour at pixels with the same color as the
    // background color parameter.
    const auto color = io::read_px_on_surface(surface, surface_px_pos);

    if (color != bg_color) {
#ifndef NDEBUG
        if ((color.r() != color.g()) ||
            (color.r() != color.b())) {
            TRACE
                << "Found color other than grayscale color: "
                << (int)color.r()
                << ","
                << (int)color.g()
                << ","
                << (int)color.b()
                << " - at position: "
                << surface_px_pos.x
                << "x"
                << surface_px_pos.y
                << "\n"
                << "(Background color is: "
                << (int)bg_color.r()
                << ","
                << (int)bg_color.g()
                << ","
                << (int)bg_color.b()
                << ")"
                << "\n";

            PANIC;
        }
#endif  // NDEBUG

        return false;
    }

    // Draw a contour here if it has a neighbour with different color than
    // the background or contour color (i.e. if it has a neighbour with a
    // color that will be drawn to the screen).
    auto pred = [&](const auto d) {
        const auto adj_p = surface_px_pos + d;

        if (!surface_px_rect.is_pos_inside(adj_p)) {
            return false;
        }

        const auto adj_color = io::read_px_on_surface(surface, adj_p);

        const bool is_bg_color = (adj_color == bg_color);
        const bool is_contour_color = (adj_color == contour_color);

        return !is_bg_color && !is_contour_color;
    };

    return (
        std::any_of(
            std::cbegin(dir_utils::g_dir_list),
            std::cend(dir_utils::g_dir_list),
            pred));
}

static void draw_black_contour_for_surface(
    SDL_Surface& surface,
    const Color& bg_color)
{
    const R surface_px_rect({0, 0}, {surface.w - 1, surface.h - 1});

    const auto contour_color = colors::black();

    for (const auto& surface_px_pos : surface_px_rect.positions()) {
        const bool should_put_contour =
            should_put_contour_at(
                surface,
                surface_px_pos,
                surface_px_rect,
                bg_color,
                contour_color);

        if (should_put_contour) {
            io::put_px_on_surface(
                surface,
                surface_px_pos,
                contour_color);
        }
    }
}

static void verify_tile_colors(
    const SDL_Surface& surface,
    const std::string& img_path)
{
#ifndef NDEBUG
    const Color full_black(0, 0, 0);
    const Color full_white(255, 255, 255);

    for (int x = 0; x < surface.w; ++x) {
        for (int y = 0; y < surface.h; ++y) {
            const Color color = io::read_px_on_surface(surface, {x, y});

            if ((color == full_black) || (color == full_white)) {
                continue;
            }

            TRACE
                << "Found illegal tile color in image '"
                << img_path
                << "': "
                << (int)color.r()
                << ","
                << (int)color.g()
                << ","
                << (int)color.b()
                << " - at position: "
                << x
                << "x"
                << y
                << "\n";
            PANIC;
        }
    }
#endif  // NDEBUG
}

static void verify_texture_size(
    SDL_Texture* texture,
    const P& expected_size,
    const std::string& img_path)
{
    P size;

    SDL_QueryTexture(texture, nullptr, nullptr, &size.x, &size.y);

    // Verify width and height of loaded image
    if (size != expected_size) {
        TRACE_ERROR_RELEASE
            << "Tile image at \""
            << img_path
            << "\" has wrong size: "
            << size.x
            << "x"
            << size.x
            << ", expected: "
            << expected_size.x
            << "x"
            << expected_size.y
            << "\n";

        PANIC;
    }
}

static SDL_Texture* create_texture_from_surface(SDL_Surface& surface)
{
    auto* const texture =
        SDL_CreateTextureFromSurface(
            io::g_sdl_renderer,
            &surface);

    if (!texture) {
        TRACE_ERROR_RELEASE
            << "Failed to create texture from surface: "
            << IMG_GetError()
            << "\n";

        PANIC;
    }

    return texture;
}

static void set_surface_color_key(SDL_Surface& surface, const Color& color)
{
    const auto v =
        SDL_MapRGB(
            surface.format,
            color.r(),
            color.g(),
            color.b());

    const bool enable_color_key = true;

    SDL_SetColorKey(&surface, enable_color_key, v);
}

static SDL_Texture* load_texture(const std::string& path)
{
    auto* const surface = load_surface(path);

    set_surface_color_key(*surface, colors::black());

    auto* const texture = create_texture_from_surface(*surface);

    SDL_FreeSurface(surface);

    return texture;
}

static SDL_Renderer* create_renderer()
{
    TRACE_FUNC_BEGIN;

    uint32_t flags = 0U;

    switch (config::renderer_type()) {
    case RendererType::auto_select:
        break;

    case RendererType::sw:
        flags = SDL_RENDERER_SOFTWARE;
        break;

    case RendererType::END:
        ASSERT(false);
        break;
    }

    SDL_Renderer* const renderer =
        SDL_CreateRenderer(io::g_sdl_window, -1, flags);

    if (!renderer) {
        TRACE_ERROR_RELEASE
            << "Failed to create SDL renderer: "
            << SDL_GetError()
            << "\n";

        PANIC;
    }

    SDL_RendererInfo info;

    if (SDL_GetRendererInfo(renderer, &info) != 0) {
        TRACE_ERROR_RELEASE
            << "Could not get RendererInfo: "
            << SDL_GetError()
            << "\n";

        PANIC;
    }

    TRACE
        << "Created SDL Renderer with name: '" << info.name << "'"
        << "\n";

    std::string flags_str;

    if (info.flags & SDL_RENDERER_SOFTWARE) {
        text_format::append_as_comma_list(
            flags_str, "SDL_RENDERER_SOFTWARE");
    }

    if (info.flags & SDL_RENDERER_ACCELERATED) {
        text_format::append_as_comma_list(
            flags_str, "SDL_RENDERER_ACCELERATED");
    }

    if (info.flags & SDL_RENDERER_PRESENTVSYNC) {
        text_format::append_as_comma_list(
            flags_str, "SDL_RENDERER_PRESENTVSYNC");
    }

    if (info.flags & SDL_RENDERER_TARGETTEXTURE) {
        text_format::append_as_comma_list(
            flags_str, "SDL_RENDERER_TARGETTEXTURE");
    }

    TRACE << "Flags: [" + flags_str + "]" << "\n";

    TRACE_FUNC_END;

    return renderer;
}

static void init_renderer()
{
    TRACE_FUNC_BEGIN;

    if (io::g_sdl_renderer) {
        io::clear_text_width_cache();
        io::clear_texture_color_mod_cache();
        SDL_DestroyRenderer(io::g_sdl_renderer);
    }

    io::g_sdl_renderer = create_renderer();

    TRACE_FUNC_END;
}

static void load_logo()
{
    TRACE_FUNC_BEGIN;

    const std::string img_path = paths::logo_img_path();

    io::g_logo_texture = load_texture(img_path);

    TRACE_FUNC_END;
}

static void free_font_surfaces()
{
    if (io::g_font_surface) {
        SDL_FreeSurface(io::g_font_surface);
        io::g_font_surface = nullptr;
    }

    if (io::g_font_surface_with_contours) {
        SDL_FreeSurface(io::g_font_surface_with_contours);
        io::g_font_surface_with_contours = nullptr;
    }
}

static void load_font()
{
    TRACE_FUNC_BEGIN;

    const std::string img_path = paths::fonts_dir() + config::font_name();

    TRACE << "Loading font image: " << img_path << "\n";

    free_font_surfaces();

    load_font_map(img_path);

    SDL_Surface* const surface = load_surface(img_path);

    swap_surface_color(*surface, colors::black(), colors::magenta());

    set_surface_color_key(*surface, colors::magenta());

    // Create the non-contour version
    SDL_Texture* texture = create_texture_from_surface(*surface);

    io::g_font_texture = texture;
    io::g_font_surface = duplicate_surface(*surface);

    draw_black_contour_for_surface(*surface, colors::magenta());

    // Create the version with contour
    texture = create_texture_from_surface(*surface);

    io::g_font_texture_with_contours = texture;
    io::g_font_surface_with_contours = duplicate_surface(*surface);

    SDL_FreeSurface(surface);

    TRACE_FUNC_END;
}

static void load_tile(const gfx::TileId id, const P& cell_px_dims)
{
    const std::string img_path = paths::tiles_dir() + gfx::tile_id_to_filename(id);

    TRACE << "Loading tile image: " << img_path << "\n";

    SDL_Surface* const surface = load_surface(img_path);

    verify_tile_colors(*surface, img_path);

    swap_surface_color(*surface, colors::black(), colors::magenta());

    set_surface_color_key(*surface, colors::magenta());

    // Create the non-contour version
    SDL_Texture* texture = create_texture_from_surface(*surface);

    io::g_tile_textures[(size_t)id] = texture;

    draw_black_contour_for_surface(*surface, colors::magenta());

    // Create the version with contour
    texture = create_texture_from_surface(*surface);

    io::g_tile_textures_with_contours[(size_t)id] = texture;

    verify_texture_size(texture, cell_px_dims, img_path);

    SDL_FreeSurface(surface);
}

static void load_tiles()
{
    TRACE_FUNC_BEGIN;

    const P cell_px_dims(
        config::map_cell_px_w(),
        config::map_cell_px_h());

    for (size_t i = 0; i < (size_t)gfx::TileId::END; ++i) {
        load_tile((gfx::TileId)i, cell_px_dims);
    }

    TRACE_FUNC_END;
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------
namespace io
{
SDL_Texture* g_font_texture_with_contours = nullptr;
SDL_Texture* g_font_texture = nullptr;
SDL_Surface* g_font_surface_with_contours = nullptr;
SDL_Surface* g_font_surface = nullptr;
SDL_Texture* g_tile_textures[(size_t)gfx::TileId::END] = {};
SDL_Texture* g_tile_textures_with_contours[(size_t)gfx::TileId::END] = {};
SDL_Texture* g_logo_texture = nullptr;

P g_rendering_px_offset = {};

namespace
{
SDL_Texture* s_current_color_mod_texture = nullptr;
Color s_current_color_mod_color = colors::black();
bool s_has_current_color_mod = false;
}  // namespace

void clear_texture_color_mod_cache()
{
    s_current_color_mod_texture = nullptr;
    s_current_color_mod_color = colors::black();
    s_has_current_color_mod = false;
}

void set_texture_color_mod_if_needed(
    SDL_Texture* const texture,
    const Color& color)
{
    if (s_has_current_color_mod &&
        (texture == s_current_color_mod_texture) &&
        (color == s_current_color_mod_color)) {
        return;
    }

    SDL_SetTextureColorMod(texture, color.r(), color.g(), color.b());

    s_current_color_mod_texture = texture;
    s_current_color_mod_color = color;
    s_has_current_color_mod = true;
}

void init_sdl()
{
    TRACE_FUNC_BEGIN;

    cleanup_sdl();

    SDL_SetHintWithPriority(
        SDL_HINT_RENDER_SCALE_QUALITY,
        "0",
        SDL_HINT_OVERRIDE);

    const uint32_t sdl_init_flags =
        SDL_INIT_VIDEO |
        SDL_INIT_AUDIO |
        SDL_INIT_EVENTS;

    if (SDL_Init(sdl_init_flags) == -1) {
        TRACE_ERROR_RELEASE
            << "Failed to init SDL"
            << "\n"
            << SDL_GetError()
            << "\n";

        PANIC;
    }

    SDL_ShowCursor(SDL_FALSE);

    const uint32_t sdl_img_flags = IMG_INIT_PNG;

    if (IMG_Init(sdl_img_flags) == -1) {
        TRACE_ERROR_RELEASE
            << "Failed to init SDL_image"
            << "\n"
            << SDL_GetError()
            << "\n";

        PANIC;
    }

    TRACE_FUNC_END;
}

void cleanup_sdl()
{
    if (!SDL_WasInit(SDL_INIT_EVERYTHING)) {
        return;
    }

    IMG_Quit();

    SDL_Quit();
}

void init_sdl_audio()
{
    cleanup_sdl_audio();

    const int audio_freq = 44100;
    const Uint16 audio_format = MIX_DEFAULT_FORMAT;
    const int audio_channels = MIX_DEFAULT_CHANNELS;
    const int audio_buffers = config::audio_buffer_size();

    const int result =
        Mix_OpenAudio(
            audio_freq,
            audio_format,
            audio_channels,
            audio_buffers);

    if (result == -1) {
        TRACE_ERROR_RELEASE
            << "Failed to init SDL_mixer"
            << "\n"
            << SDL_GetError()
            << "\n";

        ASSERT(false);
    }

    Mix_AllocateChannels(audio::g_allocated_channels);
}

void cleanup_sdl_audio()
{
    Mix_AllocateChannels(0);

    Mix_CloseAudio();
}

void init_other()
{
    TRACE_FUNC_BEGIN;

    cleanup_other();

    init_window();
    init_renderer();

    SDL_SetRenderDrawBlendMode(io::g_sdl_renderer, SDL_BLENDMODE_BLEND);

    load_font();

    if (config::is_tiles_mode()) {
        load_tiles();
        load_logo();
    }

    init_input();
    init_animation();

    TRACE_FUNC_END;
}

void cleanup_other()
{
    TRACE_FUNC_BEGIN;

    if (g_sdl_renderer) {
        clear_text_width_cache();
        free_font_surfaces();
        clear_texture_color_mod_cache();
        SDL_DestroyRenderer(g_sdl_renderer);
        g_sdl_renderer = nullptr;
    }

    free_font_surfaces();

    if (g_sdl_window) {
        SDL_DestroyWindow(g_sdl_window);
        g_sdl_window = nullptr;
    }

    TRACE_FUNC_END;
}

void on_user_toggle_fullscreen()
{
    TRACE_FUNC_BEGIN;

    init_other();

    states::draw();
    update_screen();

    TRACE_FUNC_END;
}

void on_user_toggle_scaling()
{
    TRACE_FUNC_BEGIN;

    init_other();

    states::draw();
    update_screen();

    TRACE_FUNC_END;
}

void set_clip_rect_to_panel(const Panel panel)
{
    auto px_area = gui_to_px_rect(panels::area(panel));

    px_area = px_area.scaled_up(config::video_scale_factor());

    const SDL_Rect clip_rect {
        px_area.p0.x,
        px_area.p0.y,
        px_area.w(),
        px_area.h()};

    SDL_RenderSetClipRect(g_sdl_renderer, &clip_rect);
}

void disable_clip_rect()
{
    SDL_RenderSetClipRect(g_sdl_renderer, nullptr);
}

static void draw_glyph_index_at_px(
    const int glyph_idx,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color)
{
    // TODO: Black foreground looks terrible with grayscale shaded font
    // image (all shades of white are colored black). The current solution
    // to this is to simply never use black foreground, since it's not
    // really necessary.
    ASSERT(color != colors::black());

    P gui_cell_px_dims(config::gui_cell_px_w(), config::gui_cell_px_h());

    if (draw_bg == io::DrawBg::yes) {
        // NOTE: No rendering offsets or scaling calculated yet, the
        // rectangle function performs its own offsets and scaling.
        io::draw_rectangle_filled(
            {px_pos, px_pos + gui_cell_px_dims - 1},
            bg_color);
    }

    // Set up the texture clip rectangle, before calculating scaling
    // NOTE: We expect one pixel separator between each glyph.
    P char_px_pos(
        glyph_idx % s_font_glyph_columns,
        glyph_idx / s_font_glyph_columns);

    char_px_pos.x *= (gui_cell_px_dims.x + 1);
    char_px_pos.y *= (gui_cell_px_dims.y);

    SDL_Rect clip_rect;

    clip_rect.x = char_px_pos.x;
    clip_rect.y = char_px_pos.y;
    clip_rect.w = gui_cell_px_dims.x;
    clip_rect.h = gui_cell_px_dims.y;

    // * Now apply offset and scaling *

    // Scaling
    const int scale_factor = config::video_scale_factor();

    px_pos = px_pos.scaled_up(scale_factor);
    gui_cell_px_dims = gui_cell_px_dims.scaled_up(scale_factor);

    // Apply rendering offsets (to center the graphics in the window)
    px_pos = px_pos.with_offsets(g_rendering_px_offset);

    SDL_Rect render_rect;

    render_rect.x = px_pos.x;
    render_rect.y = px_pos.y;
    render_rect.w = gui_cell_px_dims.x;
    render_rect.h = gui_cell_px_dims.y;

    SDL_Texture* texture = nullptr;

    // TODO: If black foreground will not be allowed, the contour version
    // can probably always be used
    if (/* (color == colors::black()) || */
        (bg_color == colors::black())) {
        texture = g_font_texture;
    }
    else {
        texture = g_font_texture_with_contours;
    }

    const Color color_adapted = color.with_brightness(config::brightness_pct());

    // Use batch rendering if active, otherwise render immediately
    if (map_render_batch::is_active()) {
        map_render_batch::add_character(texture, clip_rect, render_rect, color_adapted);
    }
    else {
        set_texture_color_mod_if_needed(texture, color_adapted);
        SDL_RenderCopy(g_sdl_renderer, texture, &clip_rect, &render_rect);
    }
}

static void draw_glyph_metadata_at_px(
    const FontGlyph& glyph,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color)
{
    ASSERT(color != colors::black());

    const P logical_px_dims(glyph.logical_w, glyph.logical_h);

    if (draw_bg == io::DrawBg::yes) {
        io::draw_rectangle_filled(
            {px_pos, px_pos + logical_px_dims - 1},
            bg_color);
    }

    const int scale_factor = config::video_scale_factor();

    SDL_Rect clip_rect = glyph.source_rect;

    px_pos.x += glyph.render_offset_x;
    px_pos.y += glyph.render_offset_y;
    px_pos = px_pos.scaled_up(scale_factor);
    px_pos = px_pos.with_offsets(g_rendering_px_offset);

    SDL_Rect render_rect;
    render_rect.x = px_pos.x;
    render_rect.y = px_pos.y;
    render_rect.w = glyph.source_rect.w * scale_factor;
    render_rect.h = glyph.source_rect.h * scale_factor;

    SDL_Texture* texture = nullptr;

    if (bg_color == colors::black()) {
        texture = g_font_texture;
    }
    else {
        texture = g_font_texture_with_contours;
    }

    const Color color_adapted = color.with_brightness(config::brightness_pct());

    if (map_render_batch::is_active()) {
        map_render_batch::add_character(texture, clip_rect, render_rect, color_adapted);
    }
    else {
        set_texture_color_mod_if_needed(texture, color_adapted);
        SDL_RenderCopy(g_sdl_renderer, texture, &clip_rect, &render_rect);
    }
}

void draw_character_at_px(
    const char character,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color)
{
    draw_glyph_at_px(
        static_cast<uint32_t>(static_cast<unsigned char>(character)),
        px_pos,
        color,
        draw_bg,
        bg_color);
}

void draw_glyph_at_px(
    const std::string& glyph,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color)
{
    draw_glyph_at_px(glyph_codepoint(glyph), px_pos, color, draw_bg, bg_color);
}

void draw_glyph_at_px(
    const uint32_t codepoint,
    P px_pos,
    const Color& color,
    const io::DrawBg draw_bg,
    const Color& bg_color)
{
    if (const auto* metadata = glyph_metadata(codepoint)) {
        draw_glyph_metadata_at_px(
            *metadata,
            px_pos,
            color,
            draw_bg,
            bg_color);

        return;
    }

    draw_glyph_index_at_px(
        glyph_index(codepoint),
        px_pos,
        color,
        draw_bg,
        bg_color);
}

int glyph_advance_px(const std::string& glyph)
{
    return glyph_advance_px(glyph_codepoint(glyph));
}

int glyph_advance_px(const uint32_t codepoint)
{
    if (const auto* metadata = glyph_metadata(codepoint)) {
        return metadata->advance;
    }

    return std::max(1, config::gui_cell_px_w());
}

void draw_character(const CharacterDrawObj& obj)
{
    const auto px_pos = gui_to_px_coords(obj.panel, obj.pos);

    const auto sdl_color = obj.color.sdl_color();
    const auto sdl_color_bg = obj.bg_color.sdl_color();

    draw_character_at_px(
        obj.character,
        px_pos,
        sdl_color,
        obj.draw_bg,
        sdl_color_bg);
}

void draw_tile(const TileDrawObj& obj)
{
    P px_pos = map_to_px_coords(obj.panel, obj.pos);

    P map_cell_px_dims(config::map_cell_px_w(), config::map_cell_px_h());

    if (obj.draw_bg == DrawBg::yes) {
        // NOTE: No rendering offsets or scaling calculated yet, the
        // rectangle function performs its own offsets and scaling
        draw_rectangle_filled(
            {px_pos, px_pos + map_cell_px_dims - 1},
            obj.bg_color);
    }

    // * Now apply offset and scaling *

    // Scaling
    const int scale_factor = config::video_scale_factor();

    px_pos = px_pos.scaled_up(scale_factor);
    map_cell_px_dims = map_cell_px_dims.scaled_up(scale_factor);

    // Apply rendering offsets (to center the graphics in the window)
    px_pos = px_pos.with_offsets(g_rendering_px_offset);

    SDL_Rect render_rect;

    render_rect.x = px_pos.x;
    render_rect.y = px_pos.y;
    render_rect.w = map_cell_px_dims.x;
    render_rect.h = map_cell_px_dims.y;

    SDL_Texture* texture = nullptr;

    if ((obj.color == colors::black()) ||
        (obj.bg_color == colors::black())) {
        // Foreground or background is black - no contours
        texture = g_tile_textures[(size_t)obj.tile];
    }
    else {
        // Both foreground and background are non-black - use contours
        texture = g_tile_textures_with_contours[(size_t)obj.tile];
    }

    const Color color = obj.color.with_brightness(config::brightness_pct());

    // Use batch rendering if active, otherwise render immediately
    if (map_render_batch::is_active()) {
        map_render_batch::add_tile(texture, nullptr, render_rect, color);
    }
    else {
        set_texture_color_mod_if_needed(texture, color);
        SDL_RenderCopy(g_sdl_renderer, texture, nullptr, &render_rect);
    }
}

void cover_panel(const Panel panel, const Color& color)
{
    const auto px_area = gui_to_px_rect(panels::area(panel));

    draw_rectangle_filled(px_area, color);
}

void cover_area(
    const Panel panel,
    const R& area,
    const Color& color)
{
    const auto panel_p0 = panels::p0(panel);

    const auto screen_area = area.with_offset(panel_p0);

    const auto px_area =
        gui_to_px_rect(screen_area)
            .with_offset(g_rendering_px_offset);

    draw_rectangle_filled(px_area, color);
}

void cover_area(
    const Panel panel,
    const P& offset,
    const P& dims,
    const Color& color)
{
    const auto area = R(offset, offset + dims - 1);

    cover_area(panel, area, color);
}

void cover_cell(const Panel panel, const P& offset)
{
    cover_area(panel, offset, {1, 1});
}

void draw_logo(Color color)
{
    // Set pixel position *before* applying rendering offset and scaling
    const int screen_px_w = panel_px_w(Panel::screen);

    P img_px_dims;

    SDL_QueryTexture(
        g_logo_texture,
        nullptr,
        nullptr,
        &img_px_dims.x,
        &img_px_dims.y);

    P px_pos((screen_px_w - img_px_dims.x) / 2, gui_to_px_coords_y(1));

    // * Now apply offset and scaling *

    // Scaling
    const int scale_factor = config::video_scale_factor();

    px_pos = px_pos.scaled_up(scale_factor);
    img_px_dims = img_px_dims.scaled_up(scale_factor);

    // Apply rendering offsets (to center the graphics in the window)
    px_pos = px_pos.with_offsets(g_rendering_px_offset);

    SDL_Rect render_rect;

    render_rect.x = px_pos.x;
    render_rect.y = px_pos.y;
    render_rect.w = img_px_dims.x;
    render_rect.h = img_px_dims.y;

    color = color.with_brightness(config::brightness_pct());

    set_texture_color_mod_if_needed(g_logo_texture, color);
    SDL_RenderCopy(g_sdl_renderer, g_logo_texture, nullptr, &render_rect);
}

void reload_logo()
{
    if (g_logo_texture) {
        clear_texture_color_mod_cache();
        SDL_DestroyTexture(g_logo_texture);
        g_logo_texture = nullptr;
    }

    load_logo();
}

std::string sdl_pref_dir()
{
    TRACE_FUNC_BEGIN;

    std::string subdir_str = version_info::g_version_str;

    std::replace(std::begin(subdir_str), std::end(subdir_str), '.', '_');
    std::replace(std::begin(subdir_str), std::end(subdir_str), '-', '_');

    const auto sha1_result = version_info::read_git_sha1_str_from_file();

    if (sha1_result) {
        subdir_str += "_" + sha1_result.value();
    }

    char* const path_ptr =
        // NOTE: This is somewhat of a hack, see the function arguments.
        SDL_GetPrefPath(
            "infra_arcana",       // "Organization"
            subdir_str.c_str());  // "Application"

    std::string path_str = path_ptr;

    SDL_free(path_ptr);

    TRACE << "SDL_GetPrefPath returned path '" << path_str << "'" << "\n";

    TRACE_FUNC_END;

    return path_str;
}

void sleep(const uint32_t duration)
{
    if ((duration == 0) || config::is_bot_playing()) {
        return;
    }
    else if (duration == 1) {
        SDL_Delay(duration);
    }
    else {
        // Duration longer than 1 ms
        const auto wait_until = SDL_GetTicks() + duration;

        while (SDL_GetTicks() < wait_until) {
            SDL_PumpEvents();
        }
    }
}

}  // namespace io
