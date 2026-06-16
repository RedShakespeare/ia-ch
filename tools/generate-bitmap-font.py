#!/usr/bin/env python3
"""
Generate a bitmap font atlas from the text used by the game.

The first 95 glyphs keep the current game atlas layout: ASCII 32..126 at
positions codepoint - 32. Extra UTF-8 characters are appended after that and
written to a JSON map beside the PNG.

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
CJK_RE = re.compile(r"[\u3400-\u4dbf\u4e00-\u9fff\uf900-\ufaff]+")
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


def render_font_png(
    chars: list[str],
    fonts: list[str],
    font_size: int,
    cell_w: int,
    cell_h: int,
    columns: int,
    output: Path,
    ink: int,
    allow_missing: bool,
    fit_glyphs: bool,
):
    try:
        from PIL import Image, ImageDraw
    except ModuleNotFoundError as exc:
        raise SystemExit(
            "Pillow is required to render PNG fonts. Install it with: python3 -m pip install Pillow"
        ) from exc

    loaded_fonts = load_fonts(fonts, font_size)
    unsupported_chars = [
        char
        for char in chars
        if choose_font(loaded_fonts, char) is None
    ]

    if unsupported_chars and not allow_missing:
        unsupported_str = "".join(unsupported_chars)
        raise SystemExit(
            "No supplied font can render these glyphs without using a missing-glyph box: "
            f"{unsupported_str}"
        )

    if columns <= 0:
        columns = len(chars)

    rows = (len(chars) + columns - 1) // columns
    width = (columns * (cell_w + 1)) - 1
    height = rows * cell_h
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    for index, char in enumerate(chars):
        col = index % columns
        row = index // columns
        x0 = col * (cell_w + 1)
        y0 = row * cell_h
        loaded_font = choose_font(loaded_fonts, char) or loaded_fonts[0]
        font = loaded_font.font
        bbox = draw.textbbox((0, 0), char, font=font)
        glyph_w = bbox[2] - bbox[0]
        glyph_h = bbox[3] - bbox[1]

        if fit_glyphs and (glyph_w > 0) and (glyph_h > 0) and ((glyph_w > cell_w) or (glyph_h > cell_h)):
            glyph_image = Image.new("RGBA", (glyph_w, glyph_h), (0, 0, 0, 0))
            glyph_draw = ImageDraw.Draw(glyph_image)
            glyph_draw.text((-bbox[0], -bbox[1]), char, font=font, fill=(ink, ink, ink, 255))

            scale = min(cell_w / glyph_w, cell_h / glyph_h)
            resized_size = (
                max(1, int(glyph_w * scale)),
                max(1, int(glyph_h * scale)),
            )
            glyph_image = glyph_image.resize(resized_size, Image.Resampling.LANCZOS)
            x = x0 + ((cell_w - resized_size[0]) // 2)
            y = y0 + ((cell_h - resized_size[1]) // 2)
            image.alpha_composite(glyph_image, (x, y))
        else:
            x = x0 + ((cell_w - glyph_w) // 2) - bbox[0]
            y = y0 + ((cell_h - glyph_h) // 2) - bbox[1]
            draw.text((x, y), char, font=font, fill=(ink, ink, ink, 255))

    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output)

    return loaded_fonts


def write_map(
    chars: list[str],
    cjk_words: list[str],
    columns: int,
    cell_w: int,
    cell_h: int,
    output: Path,
    fonts_used,
) -> None:
    data = {
        "cell": {"width": cell_w, "height": cell_h},
        "columns": columns,
        "glyphs": {
            char: {
                "codepoint": f"U+{ord(char):04X}",
                "index": index,
                "x": index % columns,
                "y": index // columns,
            }
            for index, char in enumerate(chars)
        },
        "cjk_words": cjk_words,
        "fonts": [
            {"path": str(loaded_font.spec.path), "size": loaded_font.spec.size}
            for loaded_font in fonts_used
        ],
    }

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a bitmap PNG font atlas from game text characters.",
    )
    parser.add_argument(
        "--font",
        action="append",
        required=True,
        help="Font path or fontconfig family. May be repeated for fallback fonts.",
    )
    parser.add_argument(
        "--font-size",
        type=int,
        default=22,
        help="TrueType/OpenType font size used for rasterization.",
    )
    parser.add_argument(
        "--cell",
        type=parse_cell,
        required=True,
        help="Output cell size as WIDTHxHEIGHT, for example 12x24.",
    )
    parser.add_argument(
        "--columns",
        type=int,
        default=0,
        help="Number of glyph cells per row. Default 0 writes all glyphs in one row.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Output PNG path, usually installed_files/gfx/fonts/WxH_name.png.",
    )
    parser.add_argument(
        "--map-output",
        type=Path,
        help="Output JSON character map. Defaults to OUTPUT with .json suffix.",
    )
    parser.add_argument(
        "--source",
        action="append",
        type=Path,
        default=[],
        help="File or directory to scan. May be repeated. Defaults to installed_files, src, and include.",
    )
    parser.add_argument(
        "--extra-text",
        default="",
        help="Additional UTF-8 text to include in the atlas.",
    )
    parser.add_argument(
        "--no-ascii",
        action="store_true",
        help="Do not force ASCII 32..126 into the first row.",
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
        help="Print collected character and word counts without writing image files.",
    )
    parser.add_argument(
        "--allow-missing",
        action="store_true",
        help="Draw a missing-glyph box for unsupported characters instead of failing.",
    )
    parser.add_argument(
        "--no-fit-glyphs",
        action="store_true",
        help="Do not scale glyphs down when they are larger than the output cell.",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    roots = args.source or [Path("installed_files"), Path("src"), Path("include")]
    corpus = collect_corpus(roots) + args.extra_text
    chars = collect_chars(corpus, include_ascii=not args.no_ascii)
    cjk_words = collect_cjk_words(corpus)
    cell_w, cell_h = args.cell
    map_output = args.map_output or args.output.with_suffix(".json")

    print(f"Collected {len(chars)} unique glyphs from {len(list(iter_text_files(roots)))} files")
    print(f"Collected {len(cjk_words)} unique CJK text runs")

    if args.dry_run:
        if cjk_words:
            print("CJK text runs:")
            for word in cjk_words:
                print(word)

        return 0

    fonts_used = render_font_png(
        chars=chars,
        fonts=args.font,
        font_size=args.font_size,
        cell_w=cell_w,
        cell_h=cell_h,
        columns=args.columns,
        output=args.output,
        ink=args.ink,
        allow_missing=args.allow_missing,
        fit_glyphs=not args.no_fit_glyphs,
    )
    columns = args.columns if args.columns > 0 else len(chars)
    write_map(
        chars=chars,
        cjk_words=cjk_words,
        columns=columns,
        cell_w=cell_w,
        cell_h=cell_h,
        output=map_output,
        fonts_used=fonts_used,
    )

    print(f"Wrote {args.output}")
    print(f"Wrote {map_output}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
