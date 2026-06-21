---
name: scan-i18n-raw-strings
description: Scan Infra Arcana C++ source and player-facing monster XML text for hardcoded English strings that likely still need i18n extraction. Use when the user asks to find untranslated/raw strings, audit modified files for i18n misses, broaden beyond simple msg_log::add searches, check monsters.xml data text, or prepare a candidate list before using extract-i18n.
---

# Scan Infra Arcana raw i18n strings

Use this skill to find candidate hardcoded English strings in Infra Arcana. It is
a scanning/audit skill, not the extraction workflow itself. For actually
replacing strings with `i18n::get` or XML `i18n_key` attributes and adding
locale/test coverage, use `extract-i18n` after the scan.

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

# Search player-facing monster data text
python3 .codex/skills/scan-i18n-raw-strings/scripts/scan_raw_i18n_strings.py installed_files/data/monsters.xml

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
- inventory item info strings such as `"(4 uses)"`, `", Lit"`, and action
  prompts built with concatenation
- monster XML text under `installed_files/data/monsters.xml` in player-facing
  tags such as names, descriptions, awareness messages, spell messages, death
  messages, and smell messages

Usually not player-facing:
- `#include` paths, save keys, data IDs, enum-name maps, file paths, debug text
- test names and assertions unless the task explicitly localizes tests
- English fallbacks already inside `i18n::get`
- empty strings used to suppress messages

## Workflow

1. Run the focused scan first, usually with `--changed` when continuing existing
   work. When a target string may live in monster data, scan
   `installed_files/data/monsters.xml` directly.
2. Inspect high-confidence hits first: `direct-ui-call` and
   `ui-constructor-arg`; inspect `monster-xml-text` hits as data-backed
   player-facing text.
3. If the file is known to contain menus or dynamic messages, rerun with
   `--include-broad` and inspect low-confidence hits.
4. For each real player-facing string, switch to `extract-i18n`:
   replace literals with `i18n::get`, add `zh_CN` keys, add or extend
   `test_i18n.cpp`, and run the relevant tests.
5. Re-run this scanner on the touched files after extraction. A clean direct
   scan is expected; broad mode may still show false positives.

## Scanner behavior

The bundled script intentionally avoids full C++ parsing. It scans `.cpp`,
`.hpp`, `.h`, `.cc`, and `.cxx` files, plus
`installed_files/data/monsters.xml` when that file is passed explicitly or
changed. It reports literals with categories:

- `direct-ui-call`: literal appears on a line with a known UI/log call.
- `ui-constructor-arg`: literal appears inside a short multiline UI constructor
  or setup context, such as `Snd(...)` or `PickTraitState(...)`.
- `broad-literal`: alphabetic literal not already inside `i18n::get`; only
  emitted with `--include-broad`.
- `monster-xml-text`: text inside a known player-facing monster XML tag that
  does not already have an `i18n_key` attribute.

Do not mass-edit all hits. The point is to avoid missing strings outside the
simple `msg_log::add("...")` pattern while still requiring human judgment.
