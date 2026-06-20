---
name: scan-i18n-raw-strings
description: Scan C++ source for hardcoded player-facing English strings that likely still need i18n extraction.
---

# Scan Infra Arcana raw i18n strings

Use this skill to find candidate hardcoded English strings in Infra Arcana. It is a scanning/audit skill, not the extraction workflow itself. For actually replacing strings with `i18n::get` and adding locale/test coverage, use `extract-i18n` after the scan.

## Quick scan

From the repository root:

```sh
python3 .codex/skills/scan-i18n-raw-strings/scripts/scan_raw_i18n_strings.py
```

Useful variants:

```sh
# Only files changed relative to HEAD
python3 .codex/skills/scan-i18n-raw-strings/scripts/scan_raw_i18n_strings.py --changed

# Search one file or subtree
python3 .codex/skills/scan-i18n-raw-strings/scripts/scan_raw_i18n_strings.py src/item_misc.cpp

# Include broad low-confidence string literals after reviewing direct hits
python3 .codex/skills/scan-i18n-raw-strings/scripts/scan_raw_i18n_strings.py --include-broad src/
```

## Review rules

Treat output as candidates. Inspect each hit before editing.

Player-facing:
- message log text and interrupt prompts
- popup titles, bodies, menu choices, and state titles
- game history entries shown in summaries
- sound messages (`Snd`) that can be displayed to the player
- inventory item info strings such as `"(4 uses)"`, `", Lit"`, and action prompts built with concatenation

Usually not player-facing:
- `#include` paths, save keys, data IDs, enum-name maps, file paths, debug text
- test names and assertions unless the task explicitly localizes tests
- English fallbacks already inside `i18n::get`
- empty strings used to suppress messages

## Workflow

1. Run the focused scan first, usually with `--changed` when continuing existing work.
2. Inspect high-confidence hits first: `direct-ui-call` and `ui-constructor-arg`.
3. If the file is known to contain menus or dynamic messages, rerun with `--include-broad` and inspect low-confidence hits.
4. For each real player-facing string, switch to `extract-i18n`: replace literals with `i18n::get`, add `zh_CN` keys, add or extend `test_i18n.cpp`, and run the relevant tests.
5. Re-run this scanner on the touched files after extraction. A clean direct scan is expected; broad mode may still show false positives.

## Scanner behavior

The bundled script intentionally avoids full C++ parsing. It scans `.cpp`, `.hpp`, `.h`, `.cc`, and `.cxx` files and reports literals with categories:

- `direct-ui-call`: literal appears on a line with a known UI/log call.
- `ui-constructor-arg`: literal appears inside a short multiline UI constructor or setup context, such as `Snd(...)` or `PickTraitState(...)`.
- `broad-literal`: alphabetic literal not already inside `i18n::get`; only emitted with `--include-broad`.

Do not mass-edit all hits. The point is to avoid missing strings outside the simple `msg_log::add("...")` pattern while still requiring human judgment.
