#!/usr/bin/env python3
"""Convert Paratranz CSV rows into an Infra Arcana locale text.ini."""

from __future__ import annotations

import argparse
import csv
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class CsvEntry:
    key: str
    source: str
    translation: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert Paratranz index/source/translation CSV into text.ini."
    )
    parser.add_argument("csv_path", type=Path, help="Paratranz CSV to import.")
    parser.add_argument(
        "--template",
        type=Path,
        help="Existing text.ini whose order, comments, and non-text sections should be preserved.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Destination text.ini path. May be the same as --template.",
    )
    parser.add_argument(
        "--value-column",
        choices=("translation", "source"),
        default="translation",
        help="CSV column to write as the text.ini value.",
    )
    parser.add_argument(
        "--blank-translation",
        choices=("error", "source", "empty"),
        default="error",
        help="How to handle blank translation cells when --value-column=translation.",
    )
    parser.add_argument(
        "--allow-extra-csv-keys",
        action="store_true",
        help="With --template, append CSV keys that are absent from the template [text] section.",
    )
    parser.add_argument(
        "--allow-missing-csv-keys",
        action="store_true",
        help="With --template, keep template values for keys that are absent from the CSV.",
    )
    parser.add_argument(
        "--no-preserve-template-edge-whitespace",
        action="store_true",
        help=(
            "With --template, do not restore leading/trailing spaces from existing values "
            "when the CSV cell has been trimmed."
        ),
    )
    return parser.parse_args()


def read_csv_entries(path: Path) -> dict[str, CsvEntry]:
    if not path.exists():
        raise SystemExit(f"CSV not found: {path}")

    entries: dict[str, CsvEntry] = {}
    with path.open(newline="", encoding="utf-8-sig") as csv_file:
        reader = csv.reader(csv_file)
        for row_nr, row in enumerate(reader, start=1):
            if not row or all(cell == "" for cell in row):
                continue
            if row_nr == 1 and [cell.strip().lower() for cell in row[:3]] == [
                "index",
                "source",
                "translation",
            ]:
                continue
            if len(row) < 3:
                raise SystemExit(f"{path}:{row_nr}: expected at least 3 CSV columns")

            key = row[0].strip()
            if not key:
                raise SystemExit(f"{path}:{row_nr}: blank index")
            if key in entries:
                raise SystemExit(f"{path}:{row_nr}: duplicate index: {key}")
            entries[key] = CsvEntry(key=key, source=row[1], translation=row[2])

    if not entries:
        raise SystemExit(f"CSV has no entries: {path}")
    return entries


def value_for_entry(entry: CsvEntry, args: argparse.Namespace) -> str:
    if args.value_column == "source":
        return entry.source

    if entry.translation != "":
        return entry.translation

    if args.blank_translation == "source":
        return entry.source
    if args.blank_translation == "empty":
        return ""
    raise SystemExit(f"blank translation for key: {entry.key}")


def encode_text_ini_value(value: str) -> str:
    out: list[str] = []
    for ch in value:
        if ch == "\\":
            out.append("\\\\")
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        else:
            out.append(ch)
    return "".join(out)


def template_edge_whitespace(raw_value: str) -> tuple[str, str]:
    prefix_len = len(raw_value) - len(raw_value.lstrip(" \t"))
    suffix_len = len(raw_value) - len(raw_value.rstrip(" \t"))
    prefix = raw_value[:prefix_len]
    suffix = raw_value[len(raw_value) - suffix_len :] if suffix_len else ""
    return prefix, suffix


def apply_template_edge_whitespace(
    value: str,
    raw_template_value: str,
    args: argparse.Namespace,
) -> str:
    if args.no_preserve_template_edge_whitespace:
        return value

    prefix, suffix = template_edge_whitespace(raw_template_value)
    if prefix and not value.startswith((" ", "\t")):
        value = prefix + value
    if suffix and not value.endswith((" ", "\t")):
        value = value + suffix
    return value


def parse_text_key(raw_line: str, in_text_section: bool) -> str | None:
    stripped = raw_line.strip()
    if (
        not in_text_section
        or not stripped
        or stripped.startswith("#")
        or stripped.startswith(";")
        or "=" not in raw_line
    ):
        return None
    return raw_line.split("=", 1)[0].strip()


def render_without_template(entries: dict[str, CsvEntry], args: argparse.Namespace) -> str:
    lines = ["[text]"]
    for key in sorted(entries):
        value = encode_text_ini_value(value_for_entry(entries[key], args))
        lines.append(f"{key}={value}")
    return "\n".join(lines) + "\n"


def render_with_template(entries: dict[str, CsvEntry], args: argparse.Namespace) -> str:
    template_path: Path = args.template
    if not template_path.exists():
        raise SystemExit(f"template not found: {template_path}")

    lines = template_path.read_text(encoding="utf-8").splitlines()
    out: list[str] = []
    seen_template_keys: set[str] = set()
    used_csv_keys: set[str] = set()
    in_text_section = False
    text_section_seen = False
    pending_extra_inserted = False

    def append_extra_csv_keys() -> None:
        nonlocal pending_extra_inserted
        if pending_extra_inserted:
            return
        extra_keys = sorted(set(entries) - seen_template_keys)
        if extra_keys:
            if not args.allow_extra_csv_keys:
                preview = ", ".join(extra_keys[:10])
                raise SystemExit(f"CSV has {len(extra_keys)} keys absent from template: {preview}")
            for key in extra_keys:
                value = encode_text_ini_value(value_for_entry(entries[key], args))
                out.append(f"{key}={value}")
                used_csv_keys.add(key)
        pending_extra_inserted = True

    for raw_line in lines:
        stripped = raw_line.strip()
        is_section = stripped.startswith("[") and stripped.endswith("]")
        if is_section:
            if in_text_section:
                append_extra_csv_keys()
            in_text_section = stripped == "[text]"
            text_section_seen = text_section_seen or in_text_section
            out.append(raw_line)
            continue

        key = parse_text_key(raw_line, in_text_section)
        if key is None:
            out.append(raw_line)
            continue

        if key in seen_template_keys:
            raise SystemExit(f"duplicate key in template: {key}")
        seen_template_keys.add(key)

        if key not in entries:
            if not args.allow_missing_csv_keys:
                raise SystemExit(f"template key missing from CSV: {key}")
            out.append(raw_line)
            continue

        raw_template_value = raw_line.split("=", 1)[1]
        value = apply_template_edge_whitespace(
            value_for_entry(entries[key], args),
            raw_template_value,
            args,
        )
        value = encode_text_ini_value(value)
        out.append(f"{key}={value}")
        used_csv_keys.add(key)

    if not text_section_seen:
        raise SystemExit(f"template has no [text] section: {template_path}")
    if in_text_section:
        append_extra_csv_keys()

    missing_used = set(entries) - used_csv_keys
    if missing_used and not args.allow_extra_csv_keys:
        preview = ", ".join(sorted(missing_used)[:10])
        raise SystemExit(f"CSV has {len(missing_used)} keys absent from template: {preview}")

    return "\n".join(out) + "\n"


def main() -> int:
    args = parse_args()
    entries = read_csv_entries(args.csv_path)
    text = (
        render_with_template(entries, args)
        if args.template
        else render_without_template(entries, args)
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text, encoding="utf-8")
    print(f"wrote {args.output} ({len(entries)} entries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
