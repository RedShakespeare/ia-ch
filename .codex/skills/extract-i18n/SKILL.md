---
name: extract-i18n
description: Find player-facing strings still hardcoded in Infra Arcana C++ source, usually by using scan-i18n-raw-strings first, extract them into locale text.ini via i18n::get lookups, build and run the test suite, then commit the extraction before finishing. Use when the user wants to continue the i18n string-extraction work on this repo — "extract raw text", "find untranslated strings", "i18n a file", "scan then extract", etc.
---

# Extract raw text into the i18n layer

This repo (Infra Arcana) is mid-migration from hardcoded English strings to a
keyed i18n layer. Player-facing strings must be fetched via `i18n::get` and
defined in the locale files — never hardcoded in source. This skill captures
the extraction workflow.

## 1. Find raw strings

Start with the `scan-i18n-raw-strings` skill. It provides the repo-specific
scanner that catches direct message calls and nearby UI constructor arguments
such as `Snd(...)` and `PickTraitState(...)`.

From the repository root, scan the current changed C++ files first:

```sh
python3 .codex/skills/scan-i18n-raw-strings/scripts/scan_raw_i18n_strings.py --changed
```

If the user names a file or module, scan that target directly:

```sh
python3 .codex/skills/scan-i18n-raw-strings/scripts/scan_raw_i18n_strings.py src/item_misc.cpp
```

For a broader audit after reviewing high-confidence hits, include low-confidence
alphabetic literals:

```sh
python3 .codex/skills/scan-i18n-raw-strings/scripts/scan_raw_i18n_strings.py --include-broad src/item_misc.cpp
```

Treat scanner output as candidates, not proof. Inspect each hit before editing.

Manual fallback searches are still useful when the scanner is unavailable or
when checking a specific pattern. Player-facing strings still hardcoded in
source commonly show up at these call sites:

```sh
# msg_log messages, popup titles/bodies, menu option vectors
grep -rn 'msg_log::add("' src/
grep -rn 'set_title("\|set_msg("' src/
# broad sweep: any literal at these sites without an i18n::get on the line
grep -rn 'set_title("\|set_msg("\|msg_log::add("' src/ | grep -v 'i18n::get'
```

What counts as player-facing raw text: anything the player reads in-game —
log messages, prompts, popup titles/bodies, menu entries, option labels and
descriptions, sound messages, game history entries, and inventory item info
suffixes. What does NOT: debug/assert/log-to-file strings, enum-name string
maps, save-file keys, IDs, and file paths.

Many strings are built by concatenating fragments with `+` (e.g.
`"I drop " + item_ref + "."`). Each literal fragment becomes its own key — see
the existing `reload.*` and `game_commands.*` keys for the prefix/suffix
convention (e.g. `*.period` for a trailing `"."`).

## 2. Choose a topic-sized workflow batch

Each workflow pass should extract a coherent topic containing several related
strings, not just the first 1-3 scanner hits found. Before editing, inspect the
surrounding source and choose a batch that a reviewer can understand as one
unit. Good batches usually include one of these:

- all `name()` fragments for a related set of terrain classes, item classes, or
  effect types
- one UI/menu/popup flow, including title, body text, options, and prompt
  suffixes
- one gameplay subsystem's related log messages, sound messages, and history
  entries
- one item/effect family, such as potion metadata, curse messages, or weapon
  proc text

Aim to extract roughly 8-30 related keys in a normal pass when the local topic
has that many strings. It is acceptable to extract fewer only when the selected
topic is genuinely small, at the end of a file/module, or when a behavior risk
requires a narrow commit. Do not stop after localizing one isolated string if
adjacent code contains related player-facing literals that can be safely handled
in the same topic.

Keep each workflow commit focused on one logical topic. If the scanner reveals
unrelated strings while working, leave them for a later workflow pass instead of
mixing domains in one commit.

Example terrain batches:

- floor/wall/pillar names and article fragments
- vegetation names: grass, shrubs, vines, trees, fungi, and burning/scorched
  modifiers
- container names: tomb/chest empty/open/material/name fragments
- fountain names and fountain effect descriptors

Example item batches:

- potion real names and identified descriptions
- unidentified potion appearance descriptors and potion name assembly
- curse trigger, warning, effect, and description text
- weapon proc messages for one item family

## 3. Extract each string

Replace the literal with an `i18n::get(key, english_fallback)` call:

```cpp
// before
msg_log::add("Nothing happens.");
// after
msg_log::add(i18n::get("<module>.nothing_happens", "Nothing happens."));
```

Rules:
- **Key naming**: `<module>.<snake_case_summary>`, where `<module>` matches the
  source file / namespace (e.g. `spells.`, `item_misc.`, `terrain_trap.`). Keep
  keys stable and descriptive.
- **Fallback**: the second arg is the exact original English string — preserve
  it verbatim (including trailing spaces/punctuation), since it renders when a
  locale lacks the key.
- **Add the key to every locale file** under
  `installed_files/data/locale/<locale>/text.ini`. Today that includes the
  `zh_CN` translation; add the key in `[text]` with the translated value. If you
  cannot translate confidently, add the key with the English value and flag it
  for the user rather than guessing.
- **Escapes**: `i18n::get` decodes `\n`, `\t`, and `\\` in locale values. Use
  `\n` in text.ini for multi-line strings; do not embed raw newlines.
- Add `#include "i18n.hpp"` to any source file that gains an `i18n::get` call
  and doesn't already include it (sorted with the other includes per
  `.clang-format`).
- Keep keys alphabetically/logically grouped as neighboring keys are; don't
  reorder unrelated lines.

## 4. Add or update tests

Mirror the existing coverage in `test/test_cases/src/test_i18n.cpp`: add a
`REQUIRE` asserting the new key resolves to its translation. For new text
wrapping/measurement behavior, cover it in `test_text_formatting.cpp` and
remember CJK glyphs are measured by pixel advance, not character count.

## 5. Build and run the tests

Run the project test suite and confirm it passes:

```sh
./run-tests.sh
```

This builds the `ia-test` target via `./build-tests.sh` (which runs
`cmake -B build` then builds with `-j$(nproc)`) and runs Catch2 with `-D 3
--abort`. To run a focused subset, pass a Catch2 name/tag filter:

```sh
./run-tests.sh "*I18n*"
```

### Cross-compile environments (mingw `build/`)

`./run-tests.sh` reuses the `build/` directory. If `build/` was first
configured with the mingw cross-compile toolchain
(`Toolchain-cross-mingw32.txt`), every `cmake -B build` keeps cross-compiling
and produces a Windows `ia-test.exe`, so the script's `./ia-test` invocation
fails with `not found`. Check with:

```sh
grep -i 'mingw\|CMAKE_TOOLCHAIN_FILE' build/CMakeCache.txt
```

When that happens, build and run the tests natively in a SEPARATE directory so
the mingw `build/` (used for Windows release artifacts) is left untouched:

```sh
cmake -B build-linux-tests
cmake --build build-linux-tests --target ia-test -- -j$(nproc)
cd build-linux-tests && ./ia-test -D 3 --abort "*I18n*"   # or no filter for all
```

The native binary is `ia-test` (no `.exe`). `build-linux-tests/` is generated
output — do not commit it.

If SDL/system dependencies are missing and block the build, report the exact
command attempted and the missing dependency rather than silently skipping the
run.

## 6. Commit before finishing

If this workflow changes source, locale, or test files, commit those changes
before giving the final response unless the user explicitly says not to commit.

Before committing:

```sh
git status --short
git diff --check
git diff --stat
```

Stage only the files that belong to the extraction. Do not stage generated build
output (`build/`, `build-linux-tests/`) or release artifacts.

Follow the repo convention: extraction commits are `[i18n]`; a wrapping/render
bug fix uncovered along the way is `[fix]`. Use a commit body that names the
source area and mentions locale/test coverage when applicable.

After committing, check `git status --short` again and report the commit hash in
the final response.
