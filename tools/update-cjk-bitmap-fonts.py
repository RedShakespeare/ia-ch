#!/usr/bin/env python3
"""Regenerate only CJK-enabled Infra Arcana bitmap font atlases."""

from __future__ import annotations

import argparse
import fnmatch
import json
import re
import subprocess
import sys
import tempfile
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


CJK_RANGES = (
    (0x3000, 0x303F),
    (0xFF00, 0xFFEF),
    (0x3400, 0x4DBF),
    (0x4E00, 0x9FFF),
    (0xF900, 0xFAFF),
)
FONT_NAME_RE = re.compile(r"^(\d+)x(\d+)")
DEFAULT_EXTRA_TEXT = "朝华打字机朝華打字機爱愛"


@dataclass(frozen=True)
class FontSpec:
    path: str
    size: int
    index: int

    def as_ref(self) -> str:
        path = Path(self.path)
        if path.suffix.lower() in {".ttc", ".otc"}:
            return f"{self.path}:{self.index}"

        return self.path


@dataclass(frozen=True)
class Layout:
    width: int
    height: int
    logical_width: int
    logical_height: int
    advance: int
    render_offset_x: int
    render_offset_y: int

    def logical_cell(self) -> str:
        return f"{self.logical_width}x{self.logical_height}"

    def atlas_cell(self) -> str:
        return f"{self.width}x{self.height}"

    def validate_for_generator(self, role: str, json_path: Path) -> None:
        expected_offset_y = (self.logical_height - self.height) // 2

        if self.advance != self.logical_width:
            raise SystemExit(
                f"{json_path}: {role} advance {self.advance} differs from "
                f"logical width {self.logical_width}; this script cannot "
                "preserve that custom advance."
            )

        if self.render_offset_x != 0:
            raise SystemExit(
                f"{json_path}: {role} render_offset_x {self.render_offset_x} "
                "is custom; this script only supports centered glyphs."
            )

        if self.render_offset_y != expected_offset_y:
            raise SystemExit(
                f"{json_path}: {role} render_offset_y {self.render_offset_y} "
                f"does not match centered atlas offset {expected_offset_y}."
            )


@dataclass(frozen=True)
class Target:
    png_path: Path
    json_path: Path
    data: dict
    ascii_layout: Layout
    default_layout: Layout
    cjk_layout: Layout
    fallback_font: FontSpec
    ascii_font: FontSpec
    cjk_font: FontSpec


def is_cjk_codepoint(codepoint: int) -> bool:
    return any(start <= codepoint <= end for start, end in CJK_RANGES)


def parse_name_cell(path: Path) -> tuple[int, int] | None:
    match = FONT_NAME_RE.match(path.name)
    if not match:
        return None

    return int(match.group(1)), int(match.group(2))


def should_include(path: Path, include_patterns: list[str], exclude_patterns: list[str]) -> bool:
    name = path.name

    if include_patterns and not any(fnmatch.fnmatch(name, pattern) for pattern in include_patterns):
        return False

    if any(fnmatch.fnmatch(name, pattern) for pattern in exclude_patterns):
        return False

    return True


def glyph_fields(data: dict, json_path: Path) -> dict[str, int]:
    fields = data.get("glyph_fields")
    if not isinstance(fields, list):
        raise SystemExit(f"{json_path}: missing glyph_fields")

    required = [
        "codepoint",
        "width",
        "height",
        "logical_width",
        "logical_height",
        "advance",
        "render_offset_x",
        "render_offset_y",
    ]
    missing = [field for field in required if field not in fields]
    if missing:
        raise SystemExit(f"{json_path}: missing glyph fields: {', '.join(missing)}")

    return {field: fields.index(field) for field in fields}


def layout_from_row(row: list[int], field_indices: dict[str, int]) -> Layout:
    return Layout(
        width=row[field_indices["width"]],
        height=row[field_indices["height"]],
        logical_width=row[field_indices["logical_width"]],
        logical_height=row[field_indices["logical_height"]],
        advance=row[field_indices["advance"]],
        render_offset_x=row[field_indices["render_offset_x"]],
        render_offset_y=row[field_indices["render_offset_y"]],
    )


def common_layout(
    data: dict,
    field_indices: dict[str, int],
    predicate,
    role: str,
    json_path: Path,
) -> Layout:
    layouts = [
        layout_from_row(row, field_indices)
        for row in data.get("glyphs", [])
        if predicate(row[field_indices["codepoint"]])
    ]

    if not layouts:
        raise SystemExit(f"{json_path}: could not find {role} glyph layout")

    [(layout, count)] = Counter(layouts).most_common(1)
    if count != len(layouts):
        raise SystemExit(f"{json_path}: {role} glyphs use mixed layouts")

    layout.validate_for_generator(role, json_path)

    return layout


def has_cjk_glyphs(data: dict, field_indices: dict[str, int]) -> bool:
    return any(
        is_cjk_codepoint(row[field_indices["codepoint"]])
        for row in data.get("glyphs", [])
    )


def font_spec(data: dict, index: int, json_path: Path) -> FontSpec:
    fonts = data.get("fonts")
    if not isinstance(fonts, list) or len(fonts) <= index:
        raise SystemExit(f"{json_path}: expected at least {index + 1} font entries")

    entry = fonts[index]
    try:
        spec = FontSpec(
            path=str(entry["path"]),
            size=int(entry["size"]),
            index=int(entry.get("index", 0)),
        )
    except (KeyError, TypeError, ValueError) as exc:
        raise SystemExit(f"{json_path}: invalid font entry {index}: {entry!r}") from exc

    if not Path(spec.path).exists():
        raise SystemExit(
            f"{json_path}: font entry {index} does not exist: {spec.path}. "
            "Install the original font or restore the recorded path; do not "
            "silently substitute another CJK font."
        )

    return spec


def collect_targets(
    font_dir: Path,
    include_patterns: list[str],
    exclude_patterns: list[str],
) -> list[Target]:
    targets: list[Target] = []

    for png_path in sorted(font_dir.glob("*.png")):
        if parse_name_cell(png_path) is None:
            continue

        if not should_include(png_path, include_patterns, exclude_patterns):
            continue

        json_path = png_path.with_suffix(".json")
        if not json_path.exists():
            continue

        data = json.loads(json_path.read_text(encoding="utf-8"))
        field_indices = glyph_fields(data, json_path)

        if not has_cjk_glyphs(data, field_indices):
            continue

        name_cell = parse_name_cell(png_path)
        map_cell = data.get("cell", {})
        if name_cell != (map_cell.get("width"), map_cell.get("height")):
            raise SystemExit(
                f"{json_path}: filename cell {name_cell[0]}x{name_cell[1]} "
                f"does not match JSON cell {map_cell!r}"
            )

        ascii_layout = common_layout(
            data,
            field_indices,
            lambda codepoint: 32 <= codepoint <= 126,
            "ASCII",
            json_path,
        )
        cjk_layout = common_layout(
            data,
            field_indices,
            is_cjk_codepoint,
            "CJK",
            json_path,
        )
        non_cjk_extra_layouts = [
            layout_from_row(row, field_indices)
            for row in data.get("glyphs", [])
            if row[field_indices["codepoint"]] > 126 and
            not is_cjk_codepoint(row[field_indices["codepoint"]])
        ]
        if non_cjk_extra_layouts:
            [(default_layout, count)] = Counter(non_cjk_extra_layouts).most_common(1)
            if count != len(non_cjk_extra_layouts):
                raise SystemExit(f"{json_path}: non-CJK extra glyphs use mixed layouts")
            default_layout.validate_for_generator("default", json_path)
        else:
            default_layout = ascii_layout

        targets.append(
            Target(
                png_path=png_path,
                json_path=json_path,
                data=data,
                ascii_layout=ascii_layout,
                default_layout=default_layout,
                cjk_layout=cjk_layout,
                fallback_font=font_spec(data, 0, json_path),
                ascii_font=font_spec(data, 1, json_path),
                cjk_font=font_spec(data, 2, json_path),
            )
        )

    return targets


def generator_cmd(
    repo_root: Path,
    target: Target,
    output: Path,
    extra_text: str,
) -> list[str]:
    script = repo_root / "tools" / "generate-bitmap-font.py"

    return [
        str(script),
        "--font",
        target.fallback_font.as_ref(),
        "--ascii-font",
        target.ascii_font.as_ref(),
        "--cjk-font",
        target.cjk_font.as_ref(),
        "--font-size",
        str(target.fallback_font.size),
        "--ascii-font-size",
        str(target.ascii_font.size),
        "--cjk-font-size",
        str(target.cjk_font.size),
        "--cell",
        target.default_layout.logical_cell(),
        "--atlas-cell",
        target.default_layout.atlas_cell(),
        "--ascii-cell",
        target.ascii_layout.logical_cell(),
        "--ascii-atlas-cell",
        target.ascii_layout.atlas_cell(),
        "--cjk-cell",
        target.cjk_layout.logical_cell(),
        "--cjk-atlas-cell",
        target.cjk_layout.atlas_cell(),
        "--columns",
        str(int(target.data.get("columns", 0))),
        "--fit-glyphs",
        "--extra-text",
        extra_text,
        "--output",
        str(output),
    ]


def run_generator(repo_root: Path, target: Target, output: Path, extra_text: str) -> None:
    subprocess.run(
        generator_cmd(repo_root, target, output, extra_text),
        cwd=repo_root,
        check=True,
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Update only bitmap fonts whose paired JSON maps already contain "
            "Chinese/CJK glyphs. ASCII-only fonts are left untouched."
        )
    )
    parser.add_argument(
        "--font-dir",
        type=Path,
        default=Path("installed_files/gfx/fonts"),
        help="Font directory relative to the repo root or absolute.",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path.cwd(),
        help="Repository root. Defaults to the current working directory.",
    )
    parser.add_argument(
        "--include-font",
        action="append",
        default=[],
        help="Only update PNG names matching this glob. May be repeated.",
    )
    parser.add_argument(
        "--exclude-font",
        action="append",
        default=[],
        help="Skip PNG names matching this glob. May be repeated.",
    )
    parser.add_argument(
        "--extra-text",
        default=DEFAULT_EXTRA_TEXT,
        help="Additional characters to include in every updated CJK atlas.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Render to a temporary directory and report planned changes.",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = args.repo_root.resolve()
    font_dir = args.font_dir
    if not font_dir.is_absolute():
        font_dir = repo_root / font_dir

    targets = collect_targets(
        font_dir=font_dir,
        include_patterns=args.include_font,
        exclude_patterns=args.exclude_font,
    )

    if not targets:
        print("No CJK-enabled font JSON maps matched; nothing to update.")
        return 0

    print("CJK-enabled font targets:")
    for target in targets:
        print(
            "  "
            f"{target.png_path.relative_to(repo_root)} "
            f"ASCII={target.ascii_layout.logical_cell()}/{target.ascii_font.size} "
            f"CJK={target.cjk_layout.logical_cell()}/{target.cjk_font.size} "
            f"CJK font={target.cjk_font.path}"
        )
    sys.stdout.flush()

    if args.dry_run:
        with tempfile.TemporaryDirectory(prefix="ia-update-font-") as temp_dir:
            temp_root = Path(temp_dir)
            for target in targets:
                run_generator(
                    repo_root=repo_root,
                    target=target,
                    output=temp_root / target.png_path.name,
                    extra_text=args.extra_text,
                )
        print("Dry run completed; no repository files were written.")
        return 0

    for target in targets:
        run_generator(
            repo_root=repo_root,
            target=target,
            output=target.png_path,
            extra_text=args.extra_text,
        )

    print(f"Updated {len(targets)} CJK-enabled font atlas(es).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
