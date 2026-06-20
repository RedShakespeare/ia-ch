---
name: extract-i18n
description: Find player-facing strings still hardcoded in Infra Arcana C++ source, usually by using scan-i18n-raw-strings first, extract a complete coherent class of related text into locale text.ini via i18n::get lookups, rely on the pre-commit test hook that builds and runs ia-test in build-linux-tests, then commit the extraction before finishing. Use when the user wants to continue the i18n string-extraction work on this repo — "extract raw text", "find untranslated strings", "i18n a file", "scan then extract", etc.
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

## 2. Choose a class-sized workflow batch

Each workflow pass should extract a complete coherent class of related text,
not just one enum case, one menu entry, or the first few scanner hits. Before
editing, inspect the surrounding source and choose the largest natural unit that
a reviewer can still understand as one change. Good batches usually include one
of these:

- all `name()` fragments for a related set of terrain classes, item classes, or
  effect types
- all descriptions/titles for one UI category or enum family, such as every
  player background description in `bg_descr(Bg)`
- one UI/menu/popup flow, including title, body text, options, and prompt
  suffixes
- one gameplay subsystem's related log messages, sound messages, and history
  entries
- one item/effect family, such as potion metadata, curse messages, or weapon
  proc text

Prefer complete sibling sets over per-sibling commits. For example, extract all
player background descriptions (Exorcist, Flagellant, Ghoul, Occultist, Rogue,
and War Veteran) in one pass rather than committing one background at a time.
Similarly, extract all potion metadata, all terrain container names, or all
closely related trait descriptions together when they live in one local data
block.

Aim to extract roughly 20-80 related keys in a normal pass when the local class
has that many strings. It is acceptable to extract fewer only when the complete
class is genuinely small, at the end of a file/module, or when a behavior risk
requires a narrow commit. Do not stop after localizing one isolated string if
adjacent code contains sibling player-facing literals that can be safely handled
in the same class.

Keep each workflow commit focused on one logical class. If the scanner reveals
unrelated strings while working, leave them for a later workflow pass instead of
mixing domains in one commit. Split a large class only when the diff becomes too
risky to review, the source requires separate behavior changes, or the tests
would be hard to diagnose as one change.

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

## 5. Let the test hook run

Do not use `./run-tests.sh` as the default validation path for this workflow.
This repo's normal `build/` directory may be configured for mingw release
artifacts, which produces a Windows `ia-test.exe` that cannot run in the Linux
agent shell.

The repo has a Codex pre-commit hook in `.codex/hooks.json`. Before `git commit`,
the hook runs `.codex/hooks/run-extract-i18n-tests.sh`, which:

- enters the `ia` conda environment with `conda run -n ia` if needed
- configures a native Linux CMake build in `build-linux-tests/`
- builds the `ia-test` target
- runs `./ia-test -D 3 --abort`

Treat this hook as the required test gate before an extraction commit. If the
hook fails, fix the failure and commit again. If the hook does not fire before a
commit command, run `.codex/hooks/run-extract-i18n-tests.sh` once and report that
the hook did not run automatically.

`build-linux-tests/` is generated output; do not commit it. If conda, the `ia`
environment, or SDL/system dependencies are missing and block the hook, report
the exact failed command and dependency error rather than silently skipping
validation.

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
