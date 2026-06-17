#!/usr/bin/env python3
"""
Generate a bitmap font atlas from the text used by the game.

The first 95 glyphs keep the current game atlas layout: ASCII 32..126 at
positions codepoint - 32. Extra UTF-8 characters are appended after that and
written to a JSON map beside the PNG. The script can use different fonts and
different logical/atlas cell sizes for ASCII and CJK glyphs while keeping them
in the same atlas image.

Example mixed-font command:
    ./tools/generate-bitmap-font.py \
        --font /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf \
        --ascii-font /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf \
        --cjk-font /usr/share/fonts/truetype/adobe/dengkuanheiti.ttf \
        --font-size 18 \
        --cjk-font-size 16 \
        --cell 12x24 \
        --atlas-cell 12x32 \
        --ascii-cell 12x24 \
        --ascii-atlas-cell 12x32 \
        --cjk-cell 16x24 \
        --cjk-atlas-cell 16x24 \
        --output installed_files/gfx/fonts/12x24_cjk.png

Requires Pillow:
    python3 -m pip install Pillow
"""

from __future__ import annotations

import argparse
import ast
import json
import re
import subprocess
import sys
import warnings
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ASCII_PRINTABLE = [chr(codepoint) for codepoint in range(32, 127)]
TEXT_EXTENSIONS = {
    ".cpp",
    ".hpp",
    ".h",
    ".ini",
    ".txt",
    ".xml",
}
CJK_RE = re.compile(r"[\u3000-\u303f\uff00-\uffef\u3400-\u4dbf\u4e00-\u9fff\uf900-\ufaff]+")
CPP_STRING_RE = re.compile(r'"(?:\\.|[^"\\])*"')


@dataclass(frozen=True)
class FontSpec:
    path: Path
    size: int


@dataclass(frozen=True)
class LoadedFont:
    spec: FontSpec
    font: object
    missing_glyph_signatures: frozenset[tuple[tuple[int, int, int, int] | None, bytes]]


@dataclass(frozen=True)
class GlyphLayout:
    logical_w: int
    logical_h: int
    atlas_w: int
    atlas_h: int


@dataclass(frozen=True)
class GlyphMetadata:
    codepoint: str
    index: int
    x: int
    y: int
    x_px: int
    y_px: int
    width: int
    height: int
    logical_width: int
    logical_height: int
    advance: int
    render_width: int
    render_height: int
    render_offset_x: int
    render_offset_y: int
    font: str
    font_size: int


@dataclass(frozen=True)
class PackedGlyphPosition:
    col: int
    row: int
    x_px: int
    y_px: int


def parse_cell(value: str) -> tuple[int, int]:
    match = re.fullmatch(r"(\d+)x(\d+)", value)
    if not match:
        raise argparse.ArgumentTypeError("cell size must use WIDTHxHEIGHT, for example 12x24")

    return int(match.group(1)), int(match.group(2))


def iter_text_files(paths: Iterable[Path]) -> Iterable[Path]:
    for path in paths:
        if path.is_file():
            if path.suffix.lower() in TEXT_EXTENSIONS:
                yield path
            continue

        if path.is_dir():
            for child in sorted(path.rglob("*")):
                if child.is_file() and child.suffix.lower() in TEXT_EXTENSIONS:
                    yield child


def decode_cpp_string_literal(token: str) -> str:
    try:
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", SyntaxWarning)
            return ast.literal_eval(token)
    except (SyntaxError, ValueError):
        return ""


def extract_file_text(path: Path) -> str:
    try:
        raw = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        raw = path.read_text(encoding="latin-1")

    if path.suffix.lower() in {".cpp", ".hpp", ".h"}:
        return "".join(decode_cpp_string_literal(match.group(0)) for match in CPP_STRING_RE.finditer(raw))

    return raw


def collect_corpus(paths: Iterable[Path]) -> str:
    chunks: list[str] = []

    for path in iter_text_files(paths):
        chunks.append(extract_file_text(path))

    return "\n".join(chunks)


def collect_chars(corpus: str, include_ascii: bool) -> list[str]:
    chars: list[str] = []
    seen: set[str] = set()

    if include_ascii:
        for char in ASCII_PRINTABLE:
            chars.append(char)
            seen.add(char)

    for char in corpus:
        if char.isspace():
            continue

        if char not in seen:
            chars.append(char)
            seen.add(char)

    return chars


def collect_cjk_words(corpus: str) -> list[str]:
    return sorted(set(CJK_RE.findall(corpus)))


def is_ascii_printable(char: str) -> bool:
    return len(char) == 1 and " " <= char <= "~"


def is_cjk(char: str) -> bool:
    return bool(CJK_RE.fullmatch(char))


def resolve_font(font: str) -> Path:
    path = Path(font)

    if path.exists():
        return path

    try:
        resolved = subprocess.check_output(
            ["fc-match", "-f", "%{file}", font],
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
    except (FileNotFoundError, subprocess.CalledProcessError):
        resolved = ""

    if resolved:
        resolved_path = Path(resolved)
        if resolved_path.exists():
            return resolved_path

    raise SystemExit(f"Could not resolve font: {font}")


def load_fonts(fonts: list[str], font_size: int):
    try:
        from PIL import ImageFont
    except ModuleNotFoundError as exc:
        raise SystemExit(
            "Pillow is required to render PNG fonts. Install it with: python3 -m pip install Pillow"
        ) from exc

    result: list[LoadedFont] = []

    for font in fonts:
        spec = FontSpec(resolve_font(font), font_size)
        loaded_font = ImageFont.truetype(str(spec.path), spec.size)
        missing_glyph_signatures = frozenset(
            glyph_signature(loaded_font, char)
            for char in ["\uffff", "\ufffd"]
        )

        result.append(
            LoadedFont(
                spec=spec,
                font=loaded_font,
                missing_glyph_signatures=missing_glyph_signatures,
            )
        )

    return result


def load_font_group(fonts: list[str], font_size: int, fallback_fonts: list[str]) -> list[LoadedFont]:
    return load_fonts(fonts or fallback_fonts, font_size)


def glyph_signature(font, char: str) -> tuple[tuple[int, int, int, int] | None, bytes]:
    mask = font.getmask(char)

    return mask.getbbox(), bytes(mask)


def glyph_has_real_pixels(loaded_font: LoadedFont, char: str) -> bool:
    if char == " ":
        return True

    signature = glyph_signature(loaded_font.font, char)
    bbox, _ = signature

    return (bbox is not None) and (signature not in loaded_font.missing_glyph_signatures)


def choose_font(loaded_fonts: list[LoadedFont], char: str) -> LoadedFont | None:
    for loaded_font in loaded_fonts:
        if glyph_has_real_pixels(loaded_font, char):
            return loaded_font

    return None


def choose_font_for_char(
    ascii_fonts: list[LoadedFont],
    cjk_fonts: list[LoadedFont],
    fallback_fonts: list[LoadedFont],
    char: str,
) -> LoadedFont | None:
    if is_ascii_printable(char):
        font = choose_font(ascii_fonts, char)
        if font is not None:
            return font

    if is_cjk(char):
        font = choose_font(cjk_fonts, char)
        if font is not None:
            return font

    return choose_font(fallback_fonts, char)


def layout_for_char(
    char: str,
    default_layout: GlyphLayout,
    ascii_layout: GlyphLayout | None,
    cjk_layout: GlyphLayout | None,
) -> GlyphLayout:
    if is_ascii_printable(char) and ascii_layout is not None:
        return ascii_layout

    if is_cjk(char) and cjk_layout is not None:
        return cjk_layout

    return default_layout


def packed_positions(
    layouts: list[GlyphLayout],
    columns: int,
    single_row: bool,
) -> tuple[list[PackedGlyphPosition], int, int, int]:
    if not layouts:
        return [], 1, 1, 0

    if single_row:
        x = 0
        positions: list[PackedGlyphPosition] = []
        height = 0

        for index, layout in enumerate(layouts):
            positions.append(PackedGlyphPosition(index, 0, x, 0))
            x += layout.atlas_w + 1
            height = max(height, layout.atlas_h)

        return positions, max(1, x - 1), height, len(layouts)

    if columns <= 0:
        approx_columns = max(1, int(len(layouts) ** 0.5))
        widest_slot = max(layout.atlas_w for layout in layouts)
        target_width = max(1, approx_columns * (widest_slot + 1) - 1)
        wrap_on_count = 0
    else:
        target_width = 0
        wrap_on_count = columns

    positions = []
    row_widths: list[int] = []
    row_heights: list[int] = []
    x = 0
    y = 0
    row = 0
    col = 0
    row_height = 0

    for index, layout in enumerate(layouts):
        glyph_width = layout.atlas_w
        projected_width = glyph_width if col == 0 else x + 1 + glyph_width

        if (col > 0) and (
            ((wrap_on_count > 0) and (col >= wrap_on_count)) or
            ((wrap_on_count == 0) and (projected_width > target_width))
        ):
            row_widths.append(x)
            row_heights.append(row_height)
            y += row_height
            x = 0
            row += 1
            col = 0
            row_height = 0

        if col > 0:
            x += 1

        positions.append(PackedGlyphPosition(col, row, x, y))
        x += glyph_width
        row_height = max(row_height, layout.atlas_h)
        col += 1

    row_widths.append(x)
    row_heights.append(row_height)

    width = max(1, max(row_widths))
    height = max(1, sum(row_heights))
    columns_actual = max(col + 1 for col in (position.col for position in positions))

    return positions, width, height, columns_actual


def render_font_png(
    chars: list[str],
    fonts: list[str],
    font_size: int,
    default_layout: GlyphLayout,
    ascii_fonts: list[str],
    ascii_font_size: int,
    ascii_layout: GlyphLayout | None,
    cjk_fonts: list[str],
    cjk_font_size: int,
    cjk_layout: GlyphLayout | None,
    columns: int,
    single_row: bool,
    output: Path,
    ink: int,
    allow_missing: bool,
    fit_glyphs: bool,
) -> tuple[list[LoadedFont], list[GlyphMetadata]]:
    try:
        from PIL import Image, ImageDraw
    except ModuleNotFoundError as exc:
        raise SystemExit(
            "Pillow is required to render PNG fonts. Install it with: python3 -m pip install Pillow"
        ) from exc

    fallback_fonts = load_fonts(fonts, font_size)
    loaded_ascii_fonts = load_font_group(ascii_fonts, ascii_font_size, fonts)
    loaded_cjk_fonts = load_font_group(cjk_fonts, cjk_font_size, fonts)
    loaded_fonts = fallback_fonts + loaded_ascii_fonts + loaded_cjk_fonts
    unsupported_chars = [
        char
        for char in chars
        if choose_font_for_char(
            loaded_ascii_fonts,
            loaded_cjk_fonts,
            fallback_fonts,
            char) is None
    ]

    if unsupported_chars and not allow_missing:
        unsupported_str = "".join(unsupported_chars)
        raise SystemExit(
            "No supplied font can render these glyphs without using a missing-glyph box: "
            f"{unsupported_str}"
        )

    layouts = [
        layout_for_char(char, default_layout, ascii_layout, cjk_layout)
        for char in chars
    ]
    positions, width, height, columns_actual = packed_positions(
        layouts=layouts,
        columns=columns,
        single_row=single_row,
    )

    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    metadata: list[GlyphMetadata] = []

    for index, char in enumerate(chars):
        layout = layouts[index]
        position = positions[index]
        col = position.col
        row = position.row
        x0 = position.x_px
        y0 = position.y_px
        loaded_font = choose_font_for_char(
            loaded_ascii_fonts,
            loaded_cjk_fonts,
            fallback_fonts,
            char) or fallback_fonts[0]
        font = loaded_font.font
        ascent, descent = font.getmetrics()
        line_h = ascent + descent
        baseline = y0 + ((layout.atlas_h - line_h) // 2) + ascent
        bbox = draw.textbbox((x0, baseline), char, font=font, anchor="ls")
        glyph_w = bbox[2] - bbox[0]
        glyph_h = bbox[3] - bbox[1]
        x = x0 + ((layout.atlas_w - glyph_w) // 2) - (bbox[0] - x0)

        if fit_glyphs and (glyph_w > 0) and (glyph_h > 0) and ((glyph_w > layout.atlas_w) or (glyph_h > layout.atlas_h)):
            glyph_image = Image.new("RGBA", (glyph_w, glyph_h), (0, 0, 0, 0))
            glyph_draw = ImageDraw.Draw(glyph_image)
            glyph_draw.text((-bbox[0], -bbox[1]), char, font=font, fill=(ink, ink, ink, 255))

            scale = min(layout.atlas_w / glyph_w, layout.atlas_h / glyph_h)
            resized_size = (
                max(1, int(glyph_w * scale)),
                max(1, int(glyph_h * scale)),
            )
            glyph_image = glyph_image.resize(resized_size, Image.Resampling.LANCZOS)
            draw_x = x0 + ((layout.atlas_w - resized_size[0]) // 2)
            draw_y = y0 + ((layout.atlas_h - resized_size[1]) // 2)
            image.alpha_composite(glyph_image, (draw_x, draw_y))
        else:
            if (glyph_w > layout.atlas_w) or (glyph_h > layout.atlas_h):
                raise SystemExit(
                    f"Glyph {char!r} rendered as {glyph_w}x{glyph_h}, "
                    f"larger than atlas cell {layout.atlas_w}x{layout.atlas_h}; "
                    "increase the atlas cell or pass --fit-glyphs"
                )

            draw.text((x, baseline), char, font=font, anchor="ls", fill=(ink, ink, ink, 255))

        metadata.append(
            GlyphMetadata(
                codepoint=f"U+{ord(char):04X}",
                index=index,
                x=col,
                y=row,
                x_px=x0,
                y_px=y0,
                width=layout.atlas_w,
                height=layout.atlas_h,
                logical_width=layout.logical_w,
                logical_height=layout.logical_h,
                advance=layout.logical_w,
                render_width=layout.atlas_w,
                render_height=layout.atlas_h,
                render_offset_x=0,
                render_offset_y=(layout.logical_h - layout.atlas_h) // 2,
                font=str(loaded_font.spec.path),
                font_size=loaded_font.spec.size,
            )
        )

    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output)

    return loaded_fonts, metadata, columns_actual


def write_map(
    chars: list[str],
    cjk_words: list[str],
    columns: int,
    default_layout: GlyphLayout,
    glyphs: list[GlyphMetadata],
    output: Path,
    fonts_used,
) -> None:
    atlas_w = max((glyph.width for glyph in glyphs), default=default_layout.atlas_w)
    atlas_h = max((glyph.height for glyph in glyphs), default=default_layout.atlas_h)
    data = {
        "cell": {"width": default_layout.logical_w, "height": default_layout.logical_h},
        "atlas_cell": {"width": atlas_w, "height": atlas_h},
        "columns": columns,
        "glyphs": {
            char: glyph.__dict__
            for char, glyph in zip(chars, glyphs)
        },
        "cjk_words": cjk_words,
        "fonts": [
            {"path": str(loaded_font.spec.path), "size": loaded_font.spec.size}
            for loaded_font in fonts_used
        ],
    }

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(data, ensure_ascii=True, indent=2) + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate a bitmap PNG font atlas and JSON glyph map from game text. "
            "Supports mixed ASCII/CJK fonts and mixed logical or atlas cell sizes."
        ),
    )
    parser.add_argument(
        "--font",
        action="append",
        required=True,
        help=(
            "Fallback font path or fontconfig family. May be repeated. "
            "Used for glyphs not covered by the ASCII or CJK font groups."
        ),
    )
    parser.add_argument(
        "--font-size",
        type=int,
        default=22,
        help="Fallback font rasterization size.",
    )
    parser.add_argument(
        "--cell",
        type=parse_cell,
        required=True,
        help="Default logical character size as WIDTHxHEIGHT, for example 12x24.",
    )
    parser.add_argument(
        "--atlas-cell",
        type=parse_cell,
        help="Default source atlas slot size. Defaults to --cell.",
    )
    parser.add_argument(
        "--ascii-font",
        action="append",
        default=[],
        help=(
            "Font path or fontconfig family preferred for printable ASCII glyphs. "
            "May be repeated for fallbacks within the ASCII group."
        ),
    )
    parser.add_argument(
        "--ascii-font-size",
        type=int,
        help="ASCII font rasterization size. Defaults to --font-size.",
    )
    parser.add_argument(
        "--ascii-cell",
        type=parse_cell,
        help="ASCII logical character size. Defaults to --cell.",
    )
    parser.add_argument(
        "--ascii-atlas-cell",
        type=parse_cell,
        help="ASCII source atlas slot size. Defaults to --atlas-cell.",
    )
    parser.add_argument(
        "--cjk-font",
        action="append",
        default=[],
        help=(
            "Font path or fontconfig family preferred for CJK glyphs. "
            "May be repeated for fallbacks within the CJK group."
        ),
    )
    parser.add_argument(
        "--cjk-font-size",
        type=int,
        help="CJK font rasterization size. Defaults to --font-size.",
    )
    parser.add_argument(
        "--cjk-cell",
        type=parse_cell,
        help="CJK logical character size. Defaults to --cell.",
    )
    parser.add_argument(
        "--cjk-atlas-cell",
        type=parse_cell,
        help="CJK source atlas slot size. Defaults to --atlas-cell.",
    )
    parser.add_argument(
        "--columns",
        type=int,
        default=0,
        help=(
            "Preferred maximum number of glyph slots per row. Default 0 picks an "
            "automatic multi-line packing width."
        ),
    )
    parser.add_argument(
        "--single-row",
        action="store_true",
        help="Force the legacy single-row atlas layout.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Output PNG atlas path, usually installed_files/gfx/fonts/WxH_name.png.",
    )
    parser.add_argument(
        "--map-output",
        type=Path,
        help="Output JSON glyph map. Defaults to OUTPUT with .json suffix.",
    )
    parser.add_argument(
        "--source",
        action="append",
        type=Path,
        default=[],
        help=(
            "File or directory to scan for text. May be repeated. "
            "Defaults to installed_files, src, and include."
        ),
    )
    parser.add_argument(
        "--extra-text",
        default="",
        help="Additional UTF-8 text to include in the atlas.",
    )
    parser.add_argument(
        "--no-ascii",
        action="store_true",
        help="Do not force ASCII 32..126 into the atlas.",
    )
    parser.add_argument(
        "--ink",
        type=int,
        default=255,
        help="Glyph grayscale value from 0 to 255.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print collected character and CJK word counts without writing files.",
    )
    parser.add_argument(
        "--allow-missing",
        action="store_true",
        help="Allow missing-glyph boxes for unsupported characters instead of failing.",
    )
    parser.add_argument(
        "--fit-glyphs",
        action="store_true",
        help="Scale individual glyphs down when they are larger than their atlas slot.",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    roots = args.source or [Path("installed_files"), Path("src"), Path("include")]
    corpus = collect_corpus(roots) + args.extra_text
    chars = collect_chars(corpus, include_ascii=not args.no_ascii)
    cjk_words = collect_cjk_words(corpus)
    cell_w, cell_h = args.cell
    atlas_cell_w, atlas_cell_h = args.atlas_cell or args.cell
    default_layout = GlyphLayout(cell_w, cell_h, atlas_cell_w, atlas_cell_h)
    ascii_cell_w, ascii_cell_h = args.ascii_cell or args.cell
    ascii_atlas_w, ascii_atlas_h = args.ascii_atlas_cell or args.atlas_cell or args.ascii_cell or args.cell
    ascii_layout = GlyphLayout(ascii_cell_w, ascii_cell_h, ascii_atlas_w, ascii_atlas_h)
    cjk_cell_w, cjk_cell_h = args.cjk_cell or args.cell
    cjk_atlas_w, cjk_atlas_h = args.cjk_atlas_cell or args.atlas_cell or args.cjk_cell or args.cell
    cjk_layout = GlyphLayout(cjk_cell_w, cjk_cell_h, cjk_atlas_w, cjk_atlas_h)
    map_output = args.map_output or args.output.with_suffix(".json")

    print(f"Collected {len(chars)} unique glyphs from {len(list(iter_text_files(roots)))} files")
    print(f"Collected {len(cjk_words)} unique CJK text runs")

    if args.dry_run:
        if cjk_words:
            print("CJK text runs:")
            for word in cjk_words:
                print(word)

        return 0

    fonts_used, glyphs, columns = render_font_png(
        chars=chars,
        fonts=args.font,
        font_size=args.font_size,
        default_layout=default_layout,
        ascii_fonts=args.ascii_font,
        ascii_font_size=args.ascii_font_size or args.font_size,
        ascii_layout=ascii_layout,
        cjk_fonts=args.cjk_font,
        cjk_font_size=args.cjk_font_size or args.font_size,
        cjk_layout=cjk_layout,
        columns=args.columns,
        single_row=args.single_row,
        output=args.output,
        ink=args.ink,
        allow_missing=args.allow_missing,
        fit_glyphs=args.fit_glyphs,
    )
    write_map(
        chars=chars,
        cjk_words=cjk_words,
        columns=columns,
        default_layout=default_layout,
        glyphs=glyphs,
        output=map_output,
        fonts_used=fonts_used,
    )

    print(f"Wrote {args.output}")
    print(f"Wrote {map_output}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
