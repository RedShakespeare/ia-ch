#!/usr/bin/env python3
"""Scan Infra Arcana C++ files for likely raw player-facing strings."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


SOURCE_EXTS = {".cpp", ".hpp", ".h", ".cc", ".cxx"}
STRING_RE = re.compile(r'"(?:\\.|[^"\\])*"')

DIRECT_UI_PATTERNS = (
    "msg_log::add(",
    ".set_title(",
    ".set_msg(",
    "game::add_history_event(",
)

MULTILINE_UI_PATTERNS = (
    "Snd(",
    "Snd ",
    "std::make_unique<PickTraitState>",
    "std::make_unique<RemoveTraitState>",
    ".setup_menu_mode(",
)

SKIP_LINE_PARTS = (
    "#include",
    "i18n::get(",
    "static_assert(",
)


@dataclass(frozen=True)
class Finding:
    path: str
    line: int
    category: str
    literal: str
    source: str


def strip_line_comment(line: str) -> str:
    in_string = False
    escaped = False
    for idx, ch in enumerate(line):
        if escaped:
            escaped = False
            continue
        if ch == "\\" and in_string:
            escaped = True
            continue
        if ch == '"':
            in_string = not in_string
            continue
        if not in_string and line[idx : idx + 2] == "//":
            return line[:idx]
    return line


def paren_delta_outside_strings(line: str) -> int:
    delta = 0
    in_string = False
    escaped = False
    for ch in line:
        if escaped:
            escaped = False
            continue
        if ch == "\\" and in_string:
            escaped = True
            continue
        if ch == '"':
            in_string = not in_string
            continue
        if not in_string and ch == "(":
            delta += 1
        elif not in_string and ch == ")":
            delta -= 1
    return delta


def unquote(literal: str) -> str:
    return literal[1:-1]


def has_alpha(text: str) -> bool:
    return any(("A" <= ch <= "Z") or ("a" <= ch <= "z") for ch in text)


def looks_path_like(text: str) -> bool:
    return "/" in text or "\\" in text or re.search(r"\.(cpp|hpp|xml|txt|png|ogg|wav)$", text)


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


def changed_files() -> list[str]:
    cmd = [
        "git",
        "diff",
        "--name-only",
        "--diff-filter=ACMRTUXB",
        "HEAD",
        "--",
        "src",
        "include",
    ]
    result = subprocess.run(cmd, check=False, text=True, capture_output=True)
    if result.returncode != 0:
        raise SystemExit(result.stderr.strip() or "git diff failed")
    return [line for line in result.stdout.splitlines() if line]


def scan_file(path: Path, include_broad: bool, context_lines: int) -> list[Finding]:
    findings: list[Finding] = []
    context = ""
    context_remaining = 0
    i18n_depth = 0

    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except UnicodeDecodeError:
        lines = path.read_text(encoding="latin-1").splitlines()

    for idx, original_line in enumerate(lines, start=1):
        line = strip_line_comment(original_line)
        stripped = line.strip()

        if i18n_depth > 0:
            i18n_depth += paren_delta_outside_strings(line)
            if i18n_depth <= 0:
                i18n_depth = 0
            continue

        if "i18n::get(" in line:
            i18n_depth = max(0, paren_delta_outside_strings(line))
            continue

        if any(part in line for part in SKIP_LINE_PARTS):
            continue

        if any(pattern in line for pattern in MULTILINE_UI_PATTERNS):
            context = "ui-constructor-arg"
            context_remaining = context_lines

        is_direct = any(pattern in line for pattern in DIRECT_UI_PATTERNS)

        for match in STRING_RE.finditer(line):
            literal = unquote(match.group(0))

            if not literal or not has_alpha(literal):
                continue

            if looks_path_like(literal):
                continue

            if is_direct:
                category = "direct-ui-call"
            elif context_remaining > 0:
                category = context
            elif include_broad:
                category = "broad-literal"
            else:
                continue

            findings.append(
                Finding(
                    path=str(path),
                    line=idx,
                    category=category,
                    literal=literal,
                    source=stripped,
                )
            )

        if context_remaining > 0:
            context_remaining -= 1
            if context_remaining == 0:
                context = ""

    return findings


def print_text(findings: list[Finding]) -> None:
    for finding in findings:
        print(
            f"{finding.path}:{finding.line}: "
            f"{finding.category}: {json.dumps(finding.literal)}"
        )
        print(f"    {finding.source}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Scan Infra Arcana C++ files for likely raw i18n strings."
    )
    parser.add_argument(
        "paths",
        nargs="*",
        default=["src"],
        help="Files or directories to scan. Defaults to src/.",
    )
    parser.add_argument(
        "--changed",
        action="store_true",
        help="Scan C++ files changed relative to HEAD under src/ and include/.",
    )
    parser.add_argument(
        "--include-broad",
        action="store_true",
        help="Also report lower-confidence alphabetic string literals.",
    )
    parser.add_argument(
        "--context-lines",
        type=int,
        default=8,
        help="Lines to inspect after a multiline UI constructor trigger.",
    )
    parser.add_argument(
        "--format",
        choices=("text", "json"),
        default="text",
        help="Output format.",
    )
    args = parser.parse_args()

    paths = changed_files() if args.changed else args.paths
    findings: list[Finding] = []

    for path in source_files(paths):
        findings.extend(scan_file(path, args.include_broad, args.context_lines))

    if args.format == "json":
        print(json.dumps([finding.__dict__ for finding in findings], indent=2))
    else:
        print_text(findings)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
