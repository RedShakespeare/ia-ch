#!/usr/bin/env python3
"""Export Infra Arcana i18n source strings as Paratranz-compatible CSV."""

from __future__ import annotations

import argparse
import csv
import json
import re
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, TextIO


SOURCE_EXTS = {".cpp", ".hpp", ".h", ".cc", ".cxx"}
I18N_CALLS = ("i18n::get", "insanity_i18n::get")


@dataclass(frozen=True)
class Entry:
    index: str
    source: str
    file: str
    line: int


@dataclass(frozen=True)
class Skipped:
    file: str
    line: int
    reason: str
    snippet: str


def source_files(paths: Iterable[str]) -> list[Path]:
    files: list[Path] = []
    for raw_path in paths:
        path = Path(raw_path)
        if path.is_file() and path.suffix in SOURCE_EXTS:
            files.append(path)
        elif path.is_dir():
            files.extend(
                child
                for child in path.rglob("*")
                if child.is_file() and child.suffix in SOURCE_EXTS
            )
    return sorted(set(files))


def strip_comments(text: str) -> str:
    out: list[str] = []
    idx = 0
    in_string = False
    in_char = False
    escaped = False

    while idx < len(text):
        ch = text[idx]
        nxt = text[idx + 1] if idx + 1 < len(text) else ""

        if escaped:
            out.append(ch)
            escaped = False
            idx += 1
            continue

        if ch == "\\" and (in_string or in_char):
            out.append(ch)
            escaped = True
            idx += 1
            continue

        if ch == '"' and not in_char:
            in_string = not in_string
            out.append(ch)
            idx += 1
            continue

        if ch == "'" and not in_string:
            in_char = not in_char
            out.append(ch)
            idx += 1
            continue

        if not in_string and not in_char and ch == "/" and nxt == "/":
            while idx < len(text) and text[idx] != "\n":
                out.append(" ")
                idx += 1
            continue

        if not in_string and not in_char and ch == "/" and nxt == "*":
            out.extend("  ")
            idx += 2
            while idx < len(text):
                if text[idx] == "\n":
                    out.append("\n")
                    idx += 1
                elif text[idx] == "*" and idx + 1 < len(text) and text[idx + 1] == "/":
                    out.extend("  ")
                    idx += 2
                    break
                else:
                    out.append(" ")
                    idx += 1
            continue

        out.append(ch)
        idx += 1

    return "".join(out)


def line_number(text: str, pos: int) -> int:
    return text.count("\n", 0, pos) + 1


def find_matching_paren(text: str, open_pos: int) -> int | None:
    depth = 0
    in_string = False
    in_char = False
    escaped = False

    for idx in range(open_pos, len(text)):
        ch = text[idx]

        if escaped:
            escaped = False
            continue

        if ch == "\\" and (in_string or in_char):
            escaped = True
            continue

        if ch == '"' and not in_char:
            in_string = not in_string
            continue

        if ch == "'" and not in_string:
            in_char = not in_char
            continue

        if in_string or in_char:
            continue

        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return idx

    return None


def split_top_level_args(text: str) -> list[str]:
    args: list[str] = []
    start = 0
    depth = 0
    in_string = False
    in_char = False
    escaped = False

    for idx, ch in enumerate(text):
        if escaped:
            escaped = False
            continue

        if ch == "\\" and (in_string or in_char):
            escaped = True
            continue

        if ch == '"' and not in_char:
            in_string = not in_string
            continue

        if ch == "'" and not in_string:
            in_char = not in_char
            continue

        if in_string or in_char:
            continue

        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        elif ch == "," and depth == 0:
            args.append(text[start:idx].strip())
            start = idx + 1

    args.append(text[start:].strip())
    return args


def decode_cpp_string(token: str) -> str:
    return bytes(token[1:-1], "utf-8").decode("unicode_escape")


def literal_value(arg: str) -> str | None:
    pos = 0
    values: list[str] = []
    string_re = re.compile(r'"(?:\\.|[^"\\])*"')

    while pos < len(arg):
        match = string_re.match(arg, pos)
        if match:
            values.append(decode_cpp_string(match.group(0)))
            pos = match.end()
            continue

        if arg[pos].isspace():
            pos += 1
            continue

        return None

    return "".join(values) if values else None


def scan_file(path: Path) -> tuple[list[Entry], list[Skipped]]:
    try:
        raw_text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        raw_text = path.read_text(encoding="latin-1")

    text = strip_comments(raw_text)
    entries: list[Entry] = []
    skipped: list[Skipped] = []
    search_pos = 0

    while True:
        call_pos = -1
        call_name = ""
        for candidate in I18N_CALLS:
            pos = text.find(candidate, search_pos)
            if pos >= 0 and (call_pos < 0 or pos < call_pos):
                call_pos = pos
                call_name = candidate
        if call_pos < 0:
            break

        open_pos = text.find("(", call_pos + len(call_name))
        if open_pos < 0:
            break

        line = line_number(text, call_pos)
        close_pos = find_matching_paren(text, open_pos)
        if close_pos is None:
            skipped.append(Skipped(str(path), line, "unclosed call", text[call_pos : call_pos + 120].strip()))
            search_pos = open_pos + 1
            continue

        args = split_top_level_args(text[open_pos + 1 : close_pos])
        if len(args) < 2:
            skipped.append(Skipped(str(path), line, "fewer than two args", text[call_pos : close_pos + 1].strip()))
            search_pos = close_pos + 1
            continue

        key = literal_value(args[0])
        source = literal_value(args[1])
        if key is None or source is None:
            skipped.append(Skipped(str(path), line, "nonliteral key or source", text[call_pos : close_pos + 1].strip()))
            search_pos = close_pos + 1
            continue

        entries.append(Entry(key, source, str(path), line))
        search_pos = close_pos + 1

    return entries, skipped


def decode_text_ini_value(value: str) -> str:
    out: list[str] = []
    idx = 0
    while idx < len(value):
        ch = value[idx]
        if ch != "\\" or idx + 1 >= len(value):
            out.append(ch)
            idx += 1
            continue

        nxt = value[idx + 1]
        if nxt == "n":
            out.append("\n")
        elif nxt == "t":
            out.append("\t")
        elif nxt == "r":
            out.append("\r")
        elif nxt == "\\":
            out.append("\\")
        else:
            out.append(nxt)
        idx += 2

    return "".join(out)


def load_locale(repo: Path, locale: str) -> dict[str, str]:
    path = repo / "installed_files" / "data" / "locale" / locale / "text.ini"
    if not path.exists():
        raise SystemExit(f"locale file not found: {path}")

    translations: dict[str, str] = {}
    in_text_section = False
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or line.startswith(";"):
            continue
        if line.startswith("[") and line.endswith("]"):
            in_text_section = line == "[text]"
            continue
        if not in_text_section or "=" not in raw_line:
            continue

        key, value = raw_line.split("=", 1)
        translations[key.strip()] = decode_text_ini_value(value.strip())

    return translations


def unique_entries(entries: list[Entry], quiet: bool) -> tuple[list[Entry], int]:
    by_key: dict[str, list[Entry]] = {}
    for entry in entries:
        by_key.setdefault(entry.index, []).append(entry)

    unique: list[Entry] = []
    duplicate_diff_count = 0
    duplicate_same_count = 0

    for key in sorted(by_key):
        key_entries = by_key[key]
        sources = {entry.source for entry in key_entries}
        if len(sources) > 1:
            duplicate_diff_count += 1
            if not quiet:
                print(f"warning: duplicate index with different source: {key}", file=sys.stderr)
                for entry in key_entries:
                    print(f"  {entry.file}:{entry.line}: {entry.source}", file=sys.stderr)
        elif len(key_entries) > 1:
            duplicate_same_count += 1

        unique.append(sorted(key_entries, key=lambda entry: (entry.file, entry.line))[0])

    if duplicate_same_count and not quiet:
        print(f"note: duplicate indexes with same source: {duplicate_same_count}", file=sys.stderr)

    return unique, duplicate_diff_count


def write_paratranz_csv(entries: list[Entry], translations: dict[str, str], out: TextIO) -> None:
    writer = csv.DictWriter(out, fieldnames=["index", "source", "translation"])
    for entry in entries:
        writer.writerow(
            {
                "index": entry.index,
                "source": entry.source,
                "translation": translations.get(entry.index, ""),
            }
        )


def write_json(entries: list[Entry], translations: dict[str, str], out: TextIO) -> None:
    rows = []
    for entry in entries:
        row = asdict(entry)
        row["translation"] = translations.get(entry.index, "")
        rows.append(row)
    json.dump(rows, out, indent=2, ensure_ascii=False)
    out.write("\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Export Infra Arcana i18n source text as Paratranz-compatible CSV."
    )
    parser.add_argument(
        "paths",
        nargs="*",
        default=["src", "include"],
        help="Files or directories to scan. Defaults to src/ and include/.",
    )
    parser.add_argument(
        "--locale",
        help="Populate translation from installed_files/data/locale/<locale>/text.ini.",
    )
    parser.add_argument(
        "--format",
        choices=("csv", "json"),
        default="csv",
        help="Output format. CSV uses index,source,translation columns.",
    )
    parser.add_argument("--output", help="Write output to this file instead of stdout.")
    parser.add_argument("--quiet", action="store_true", help="Suppress diagnostics.")
    args = parser.parse_args()

    repo = Path.cwd()
    entries: list[Entry] = []
    skipped: list[Skipped] = []
    for path in source_files(args.paths):
        file_entries, file_skipped = scan_file(path)
        entries.extend(file_entries)
        skipped.extend(file_skipped)

    entries, duplicate_diff_count = unique_entries(entries, args.quiet)
    translations = load_locale(repo, args.locale) if args.locale else {}

    if args.locale and not args.quiet:
        missing = sum(1 for entry in entries if entry.index not in translations)
        if missing:
            print(f"warning: missing {args.locale} translations: {missing}", file=sys.stderr)

    if skipped and not args.quiet:
        print(f"warning: skipped i18n calls: {len(skipped)}", file=sys.stderr)
        for skipped_call in skipped[:20]:
            print(
                f"  {skipped_call.file}:{skipped_call.line}: {skipped_call.reason}: {skipped_call.snippet}",
                file=sys.stderr,
            )
        if len(skipped) > 20:
            print(f"  ... {len(skipped) - 20} more", file=sys.stderr)

    out_path = Path(args.output) if args.output else None
    out_file = out_path.open("w", encoding="utf-8", newline="") if out_path else sys.stdout
    try:
        if args.format == "json":
            write_json(entries, translations, out_file)
        else:
            write_paratranz_csv(entries, translations, out_file)
    finally:
        if out_path:
            out_file.close()

    return 1 if duplicate_diff_count else 0


if __name__ == "__main__":
    raise SystemExit(main())
