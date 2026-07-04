#!/usr/bin/env python3
"""Export Infra Arcana i18n source strings as Paratranz-compatible CSV."""

from __future__ import annotations

import argparse
import csv
import json
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, TextIO


SOURCE_EXTS = {".cpp", ".hpp", ".h", ".cc", ".cxx"}
XML_EXTS = {".xml"}
MANUAL_REL_PATH = Path("installed_files") / "manual.txt"
MANUAL_LOCALE_REL_PATH = Path("installed_files") / "data" / "locale"
MANUAL_DELIM = "-" * 80
MESSAGES_REL_DIR = Path("installed_files") / "data" / "messages"
XML_REL_DIR = Path("installed_files") / "data"
BASE_CALL_SPECS = (
    ("i18n::get", ""),
    ("i18n::format", ""),
    ("insanity_i18n::get", "insanity."),
)
EMPTY_SOURCE_PLACEHOLDER = "__EMPTY__"
SPACE_SOURCE_PLACEHOLDER = "__SPACE__"


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


@dataclass(frozen=True)
class ManualBlock:
    chapter_slug: str
    title: str
    paragraphs: tuple[str, ...]
    title_line: int
    paragraph_lines: tuple[int, ...]


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


def xml_source_files(paths: Iterable[str]) -> list[Path]:
    files: list[Path] = []
    for raw_path in paths:
        path = Path(raw_path)
        if path.is_file() and path.suffix in XML_EXTS:
            files.append(path)
        elif path.is_dir():
            files.extend(
                child
                for child in path.rglob("*")
                if child.is_file() and child.suffix in XML_EXTS
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


def find_next_call(text: str, name: str, start: int) -> tuple[int, int]:
    pattern = re.compile(rf"(?<![A-Za-z0-9_]){re.escape(name)}\s*\(")
    match = pattern.search(text, start)
    if not match:
        return -1, -1
    open_pos = match.end() - 1
    return match.start(), open_pos


def call_specs_for_path(path: Path) -> list[tuple[str, str]]:
    specs = list(BASE_CALL_SPECS)

    if path.name == "item_data.cpp":
        specs.append(("tr", "item_data."))
    elif path.name == "map_mode_gui.cpp":
        specs.append(("tr", "map_mode_gui."))
    elif path.name == "i18n.cpp":
        specs.append(("get", ""))

    return specs


def find_line_containing(text: str, needle: str) -> int:
    pos = text.find(needle)
    return line_number(text, pos) if pos >= 0 else 1


def xml_i18n_key_lines(text: str) -> dict[str, list[int]]:
    result: dict[str, list[int]] = {}
    pattern = re.compile(r"\bi18n_key\s*=\s*([\"'])(.*?)\1")

    for match in pattern.finditer(text):
        result.setdefault(match.group(2), []).append(line_number(text, match.start()))

    return result


def scan_xml_file(path: Path) -> list[Entry]:
    raw_text = path.read_text(encoding="utf-8")
    line_by_key = xml_i18n_key_lines(raw_text)
    tree = ET.parse(path)
    entries: list[Entry] = []

    for element in tree.iter():
        key = element.attrib.get("i18n_key")
        if not key:
            continue

        source = "".join(element.itertext()).strip()
        if not source:
            continue

        key_lines = line_by_key.get(key)
        line = key_lines.pop(0) if key_lines else 1
        entries.append(Entry(key, source, str(path), line))

    return entries


def slugify_manual_chapter(title: str) -> str:
    slug = re.sub(r"[^A-Za-z0-9]+", "_", title.strip().lower()).strip("_")
    return slug or "chapter"


def split_manual_paragraphs(lines: list[str], start_line: int) -> tuple[tuple[str, ...], tuple[int, ...]]:
    paragraphs: list[str] = []
    paragraph_lines: list[int] = []
    current: list[str] = []
    current_line = start_line

    def flush() -> None:
        nonlocal current
        if not current:
            return
        paragraphs.append("\n".join(current).rstrip())
        paragraph_lines.append(current_line)
        current = []

    for offset, line in enumerate(lines):
        if not line:
            flush()
            continue
        if not current:
            current_line = start_line + offset
        current.append(line)

    flush()

    return tuple(paragraphs), tuple(paragraph_lines)


def parse_manual_blocks(path: Path) -> list[ManualBlock]:
    raw_lines = path.read_text(encoding="utf-8").splitlines()
    blocks: list[ManualBlock] = []
    idx = 0

    while idx < len(raw_lines):
        if raw_lines[idx] != MANUAL_DELIM:
            idx += 1
            continue

        title_idx = idx + 1
        delim_idx = idx + 2
        if delim_idx >= len(raw_lines) or raw_lines[delim_idx] != MANUAL_DELIM:
            raise ValueError(f"invalid manual chapter header at {path}:{idx + 1}")

        title = raw_lines[title_idx]
        content_start = delim_idx + 1
        content_end = content_start
        while content_end < len(raw_lines) and raw_lines[content_end] != MANUAL_DELIM:
            content_end += 1

        paragraphs, paragraph_lines = split_manual_paragraphs(
            raw_lines[content_start:content_end],
            content_start + 1,
        )
        blocks.append(
            ManualBlock(
                slugify_manual_chapter(title),
                title,
                paragraphs,
                title_idx + 1,
                paragraph_lines,
            )
        )

        idx = content_end

    return blocks


def manual_entries(repo: Path) -> list[Entry]:
    path = repo / MANUAL_REL_PATH
    if not path.exists():
        return []

    entries: list[Entry] = []
    slug_counts: dict[str, int] = {}

    for block in parse_manual_blocks(path):
        slug_counts[block.chapter_slug] = slug_counts.get(block.chapter_slug, 0) + 1
        slug = block.chapter_slug
        if slug_counts[block.chapter_slug] > 1:
            slug = f"{block.chapter_slug}_{slug_counts[block.chapter_slug]}"

        entries.append(
            Entry(
                f"manual.{slug}.title",
                block.title,
                str(path),
                block.title_line,
            )
        )

        for nr, paragraph in enumerate(block.paragraphs, start=1):
            entries.append(
                Entry(
                    f"manual.{slug}.p{nr:03d}",
                    paragraph,
                    str(path),
                    block.paragraph_lines[nr - 1],
                )
            )

    return entries


def manual_translation_entries(repo: Path, locale: str, quiet: bool) -> dict[str, str]:
    source_path = repo / MANUAL_REL_PATH
    locale_path = repo / MANUAL_LOCALE_REL_PATH / locale / "manual.txt"
    if not source_path.exists() or not locale_path.exists():
        return {}

    source_blocks = parse_manual_blocks(source_path)
    locale_blocks = parse_manual_blocks(locale_path)
    translations: dict[str, str] = {}
    slug_counts: dict[str, int] = {}

    if len(source_blocks) != len(locale_blocks) and not quiet:
        print(
            f"warning: manual chapter count differs for {locale}: "
            f"{len(source_blocks)} source, {len(locale_blocks)} localized",
            file=sys.stderr,
        )

    for block_idx, source_block in enumerate(source_blocks):
        if block_idx >= len(locale_blocks):
            break

        locale_block = locale_blocks[block_idx]
        slug_counts[source_block.chapter_slug] = slug_counts.get(source_block.chapter_slug, 0) + 1
        slug = source_block.chapter_slug
        if slug_counts[source_block.chapter_slug] > 1:
            slug = f"{source_block.chapter_slug}_{slug_counts[source_block.chapter_slug]}"

        translations[f"manual.{slug}.title"] = locale_block.title

        if len(source_block.paragraphs) != len(locale_block.paragraphs) and not quiet:
            print(
                f"warning: manual paragraph count differs for {locale} chapter "
                f"{source_block.title!r}: {len(source_block.paragraphs)} source, "
                f"{len(locale_block.paragraphs)} localized",
                file=sys.stderr,
            )

        for nr, paragraph in enumerate(locale_block.paragraphs[: len(source_block.paragraphs)], start=1):
            translations[f"manual.{slug}.p{nr:03d}"] = paragraph

    return translations


def read_runtime_message_lines(path: Path) -> list[tuple[str, int]]:
    result: list[tuple[str, int]] = []

    for line_nr, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if line and line[0] != " " and line[0] != "#":
            result.append((line, line_nr))

    return result


def message_file_entries(path: Path) -> list[Entry]:
    entries: list[Entry] = []
    stem = path.stem

    for nr, (line, line_nr) in enumerate(read_runtime_message_lines(path), start=1):
        entries.append(
            Entry(
                f"messages.{stem}.l{nr:03d}",
                line,
                str(path),
                line_nr,
            )
        )

    return entries


def message_source_files(repo: Path) -> list[Path]:
    messages_dir = repo / MESSAGES_REL_DIR
    if not messages_dir.exists():
        return []

    return sorted(messages_dir.glob("*.txt"))


def message_file_translations(repo: Path, locale: str, source_path: Path, quiet: bool) -> dict[str, str]:
    locale_path = repo / "installed_files" / "data" / "locale" / locale / "messages" / source_path.name
    if not locale_path.exists():
        return {}

    source_lines = read_runtime_message_lines(source_path)
    locale_lines = read_runtime_message_lines(locale_path)
    if len(source_lines) != len(locale_lines):
        if not quiet:
            print(
                f"warning: message count differs for {locale} {source_path.name}: "
                f"{len(source_lines)} source, {len(locale_lines)} localized; "
                "leaving translations blank for this file",
                file=sys.stderr,
            )
        return {}

    stem = source_path.stem
    translations: dict[str, str] = {}
    for nr, (line, _line_nr) in enumerate(locale_lines, start=1):
        translations[f"messages.{stem}.l{nr:03d}"] = line

    return translations


def scan_gods_file(raw_text: str, path: Path) -> list[Entry]:
    pattern = re.compile(
        r'\{\s*"[^"]+"\s*,\s*"([^"]+)"\s*,\s*("(?:(?:\\.)|[^"\\])*")\s*\}'
    )
    entries: list[Entry] = []

    for key, fallback_token in pattern.findall(raw_text):
        entries.append(
            Entry(
                key,
                decode_cpp_string(fallback_token),
                str(path),
                find_line_containing(raw_text, key),
            )
        )

    return entries


def scan_item_data_file(raw_text: str, path: Path) -> list[Entry]:
    string_token = r'"(?:(?:\\.)|[^"\\])*"'
    item_name_pattern = re.compile(
        rf'item_name\(\s*({string_token})\s*,\s*({string_token})\s*,\s*({string_token})\s*,\s*({string_token})\s*\)',
        re.DOTALL,
    )
    attack_msgs_pattern = re.compile(
        rf'attack_msgs\(\s*({string_token})\s*,\s*({string_token})\s*,\s*({string_token})\s*\)',
        re.DOTALL,
    )

    entries: list[Entry] = []

    for key_token, name_token, plural_token, a_token in item_name_pattern.findall(raw_text):
        key = decode_cpp_string(key_token)
        entries.extend(
            [
                Entry(
                    f"item_data.{key}.name",
                    decode_cpp_string(name_token),
                    str(path),
                    find_line_containing(raw_text, key_token),
                ),
                Entry(
                    f"item_data.{key}.name_plural",
                    decode_cpp_string(plural_token),
                    str(path),
                    find_line_containing(raw_text, key_token),
                ),
                Entry(
                    f"item_data.{key}.name_a",
                    decode_cpp_string(a_token),
                    str(path),
                    find_line_containing(raw_text, key_token),
                ),
            ]
        )

    for key_token, player_token, other_token in attack_msgs_pattern.findall(raw_text):
        key = decode_cpp_string(key_token)
        entries.extend(
            [
                Entry(
                    f"item_data.{key}.player",
                    decode_cpp_string(player_token),
                    str(path),
                    find_line_containing(raw_text, key_token),
                ),
                Entry(
                    f"item_data.{key}.other",
                    decode_cpp_string(other_token),
                    str(path),
                    find_line_containing(raw_text, key_token),
                ),
            ]
        )

    return entries


def scan_view_actor_descr_file(raw_text: str, path: Path) -> list[Entry]:
    pattern = re.compile(
        r'"(view_actor_descr\.[^"]+)"\s*,\s*("(?:(?:\\.)|[^"\\])*")'
    )
    entries: list[Entry] = []

    for key, fallback_token in pattern.findall(raw_text):
        entries.append(
            Entry(
                key,
                decode_cpp_string(fallback_token),
                str(path),
                find_line_containing(raw_text, key),
            )
        )

    return entries


def scan_version_file(raw_text: str, path: Path) -> list[Entry]:
    key_by_const = {
        "g_copyright_str": "version.copyright",
        "g_license_str": "version.license",
    }
    pattern = re.compile(
        r'const std::string (g_copyright_str|g_license_str)\s*=\s*("(?:(?:\\.)|[^"\\])*")\s*;'
    )
    entries: list[Entry] = []

    for const_name, fallback_token in pattern.findall(raw_text):
        key = key_by_const[const_name]
        entries.append(
            Entry(
                key,
                decode_cpp_string(fallback_token),
                str(path),
                find_line_containing(raw_text, const_name),
            )
        )

    return entries


def scan_config_file(raw_text: str, path: Path) -> list[Entry]:
    if "option.any_key_confirm_more.descr" not in raw_text:
        return []

    # The "[space]" (or whatever) more-prompt text is defined as the fallback of
    # the msg_log.more_prompt i18n::get call in src/msg_log.cpp.
    msg_log_path = path.parent / "msg_log.cpp"
    more_str = "[space]"
    if msg_log_path.exists():
        msg_log_text = msg_log_path.read_text(encoding="utf-8")
        msg_more_match = re.search(
            r'i18n::get\s*\(\s*"msg_log\.more_prompt"\s*,\s*("(?:(?:\\.)|[^"\\])*")',
            msg_log_text,
        )
        if msg_more_match:
            more_str = decode_cpp_string(msg_more_match.group(1))

    source = (
        f'Any key confirms "{more_str}" prompts in the message log '
        "(which can happen for example when a monster appears as a warning to the player), "
        "otherwise only space (and a few other keys) confirms these prompts. "
        "Keeping the option disabled is safer."
    )

    return [
        Entry(
            "option.any_key_confirm_more.descr",
            source,
            str(path),
            find_line_containing(raw_text, "option.any_key_confirm_more.descr"),
        )
    ]


def repo_specific_entries(raw_text: str, path: Path) -> list[Entry]:
    if path.name == "item_data.cpp":
        return scan_item_data_file(raw_text, path)
    if path.name == "gods.cpp":
        return scan_gods_file(raw_text, path)
    if path.name == "view_actor_descr.cpp":
        return scan_view_actor_descr_file(raw_text, path)
    if path.name == "version.cpp":
        return scan_version_file(raw_text, path)
    if path.name == "config.cpp":
        return scan_config_file(raw_text, path)
    return []


def scan_file(path: Path) -> tuple[list[Entry], list[Skipped]]:
    try:
        raw_text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        raw_text = path.read_text(encoding="latin-1")

    text = strip_comments(raw_text)
    entries = repo_specific_entries(raw_text, path)
    skipped: list[Skipped] = []
    search_pos = 0
    call_specs = call_specs_for_path(path)

    while True:
        call_pos = -1
        open_pos = -1
        key_prefix = ""
        for candidate, candidate_prefix in call_specs:
            pos, candidate_open_pos = find_next_call(text, candidate, search_pos)
            if pos >= 0 and (call_pos < 0 or pos < call_pos):
                call_pos = pos
                open_pos = candidate_open_pos
                key_prefix = candidate_prefix
        if call_pos < 0:
            break

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

        if key_prefix and not key.startswith(key_prefix):
            key = f"{key_prefix}{key}"

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


def load_grammar_locale(repo: Path, locale: str) -> dict[str, str]:
    """Load grammar.ini templates as section.key -> value.

    Mirrors the C++ parser in src/i18n.cpp: the INI section name becomes the
    key prefix, and the key within the section is the part after the first dot
    of the i18n::format() call's key argument.
    """
    path = repo / "installed_files" / "data" / "locale" / locale / "grammar.ini"
    if not path.exists():
        return {}

    translations: dict[str, str] = {}
    section = ""
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or line.startswith(";"):
            continue
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1].strip()
            continue
        if "=" not in raw_line or not section:
            continue

        key_name, value = raw_line.split("=", 1)
        translations[f"{section}.{key_name.strip()}"] = decode_text_ini_value(value.strip())

    return translations


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
                "source": paratranz_source_text(entry.source),
                "translation": translations.get(entry.index, ""),
            }
        )


def paratranz_source_text(source: str) -> str:
    if source == "":
        return EMPTY_SOURCE_PLACEHOLDER
    if source.strip() == "":
        return SPACE_SOURCE_PLACEHOLDER
    return source


def write_json(entries: list[Entry], translations: dict[str, str], out: TextIO) -> None:
    rows = []
    for entry in entries:
        row = asdict(entry)
        row["translation"] = translations.get(entry.index, "")
        rows.append(row)
    json.dump(rows, out, indent=2, ensure_ascii=False)
    out.write("\n")


def path_includes_manual(repo: Path, raw_path: str) -> bool:
    path = Path(raw_path)
    if not path.is_absolute():
        path = repo / path

    manual_path = repo / MANUAL_REL_PATH
    try:
        if path.is_file():
            return path.resolve() == manual_path.resolve()
        if path.is_dir():
            manual_path.resolve().relative_to(path.resolve())
            return True
    except OSError:
        return False
    except ValueError:
        return False

    return False


def should_include_manual(repo: Path, paths: list[str], mode: str) -> bool:
    if mode == "always":
        return True
    if mode == "never":
        return False
    if not paths:
        return True
    return any(path_includes_manual(repo, path) for path in paths)


def catalog_entries_for_args(
    repo: Path,
    paths: list[str],
    manual_mode: str,
) -> tuple[list[Entry], list[Skipped]]:
    entries: list[Entry] = []
    skipped: list[Skipped] = []
    scan_paths = paths or ["src", "include"]
    for path in source_files(scan_paths):
        file_entries, file_skipped = scan_file(path)
        entries.extend(file_entries)
        skipped.extend(file_skipped)

    xml_scan_paths = paths or [str(XML_REL_DIR)]
    for path in xml_source_files(xml_scan_paths):
        entries.extend(scan_xml_file(path))

    if should_include_manual(repo, paths, manual_mode):
        entries.extend(manual_entries(repo))

    return entries, skipped


def catalog_translations_for_args(
    repo: Path,
    locale: str | None,
    paths: list[str],
    manual_mode: str,
    quiet: bool,
) -> dict[str, str]:
    if not locale:
        return {}

    translations = load_locale(repo, locale)

    # grammar.ini templates: text.ini wins on key overlap (an i18n::get call
    # should keep its text.ini translation, not be shadowed by grammar.ini).
    for key, value in load_grammar_locale(repo, locale).items():
        translations.setdefault(key, value)

    if should_include_manual(repo, paths, manual_mode):
        translations.update(manual_translation_entries(repo, locale, quiet))

    return translations


def write_csv_file(path: Path, entries: list[Entry], translations: dict[str, str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as out_file:
        write_paratranz_csv(entries, translations, out_file)


def validate_catalog_keys_match_locale(
    entries: list[Entry],
    translations: dict[str, str],
    locale: str,
    quiet: bool,
) -> int:
    entry_keys = {entry.index for entry in entries}
    locale_keys = set(translations)
    missing_from_catalog = sorted(locale_keys - entry_keys)
    extra_in_catalog = sorted(entry_keys - locale_keys)
    mismatch_count = len(missing_from_catalog) + len(extra_in_catalog)

    if mismatch_count and not quiet:
        print(
            f"error: catalog key set does not match installed_files/data/locale/{locale}/text.ini or grammar.ini: "
            f"{len(missing_from_catalog)} missing from catalog, "
            f"{len(extra_in_catalog)} extra in catalog",
            file=sys.stderr,
        )
        for key in missing_from_catalog[:20]:
            print(f"  missing from catalog: {key}", file=sys.stderr)
        if len(missing_from_catalog) > 20:
            print(f"  ... {len(missing_from_catalog) - 20} more missing", file=sys.stderr)
        for key in extra_in_catalog[:20]:
            print(f"  extra in catalog: {key}", file=sys.stderr)
        if len(extra_in_catalog) > 20:
            print(f"  ... {len(extra_in_catalog) - 20} more extra", file=sys.stderr)

    return mismatch_count


def catalog_csv_name(locale: str | None) -> str:
    if locale:
        return f"ia-paratranz-{locale}.csv"
    return "ia-paratranz.csv"


def write_output_dir(
    repo: Path,
    out_dir: Path,
    paths: list[str],
    manual_mode: str,
    locale: str | None,
    quiet: bool,
) -> tuple[int, list[Skipped]]:
    entries, skipped = catalog_entries_for_args(repo, paths, "never")
    entries, duplicate_diff_count = unique_entries(entries, quiet)
    translations = catalog_translations_for_args(repo, locale, paths, "never", quiet)
    key_mismatch_count = (
        validate_catalog_keys_match_locale(entries, translations, locale, quiet)
        if locale and not paths
        else 0
    )

    if key_mismatch_count:
        return duplicate_diff_count + key_mismatch_count, skipped

    write_csv_file(out_dir / catalog_csv_name(locale), entries, translations)

    if should_include_manual(repo, paths, manual_mode):
        manual_file_entries = manual_entries(repo)
        manual_translations = (
            manual_translation_entries(repo, locale, quiet)
            if locale
            else {}
        )
        write_csv_file(out_dir / "manual.csv", manual_file_entries, manual_translations)

    for message_path in message_source_files(repo):
        message_entries = message_file_entries(message_path)
        message_translations = (
            message_file_translations(repo, locale, message_path, quiet)
            if locale
            else {}
        )
        write_csv_file(out_dir / f"{message_path.stem}.csv", message_entries, message_translations)

    return duplicate_diff_count, skipped


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Export Infra Arcana i18n source text as Paratranz-compatible CSV."
    )
    parser.add_argument(
        "paths",
        nargs="*",
        help="Files or directories to scan. Defaults to src/ and include/.",
    )
    parser.add_argument(
        "--locale",
        help="Populate translation from installed_files/data/locale/<locale>/text.ini and manual.txt.",
    )
    parser.add_argument(
        "--manual",
        choices=("auto", "always", "never"),
        default="auto",
        help=(
            "Include installed_files/manual.txt entries. "
            "auto includes them for the default full export or when a supplied path contains manual.txt."
        ),
    )
    parser.add_argument(
        "--format",
        choices=("csv", "json"),
        default="csv",
        help="Output format. CSV uses index,source,translation columns.",
    )
    parser.add_argument("--output", help="Write output to this file instead of stdout.")
    parser.add_argument(
        "--output-dir",
        help=(
            "Write a Paratranz CSV bundle to this directory: ia-paratranz*.csv, "
            "manual.csv, plus one CSV per installed_files/data/messages/*.txt file."
        ),
    )
    parser.add_argument("--quiet", action="store_true", help="Suppress diagnostics.")
    args = parser.parse_args()

    repo = Path.cwd()

    if args.output and args.output_dir:
        raise SystemExit("--output and --output-dir cannot be used together")

    if args.output_dir:
        duplicate_diff_count, skipped = write_output_dir(
            repo,
            Path(args.output_dir),
            args.paths,
            args.manual,
            args.locale,
            args.quiet,
        )
        entries_for_missing_check: list[Entry] = []
        translations_for_missing_check: dict[str, str] = {}
    else:
        entries, skipped = catalog_entries_for_args(repo, args.paths, args.manual)
        entries, duplicate_diff_count = unique_entries(entries, args.quiet)
        translations = catalog_translations_for_args(
            repo,
            args.locale,
            args.paths,
            args.manual,
            args.quiet,
        )
        entries_for_missing_check = entries
        translations_for_missing_check = translations

    if args.locale and not args.quiet and entries_for_missing_check:
        missing = sum(
            1 for entry in entries_for_missing_check if entry.index not in translations_for_missing_check
        )
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

    if not args.output_dir:
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
