---
name: export-i18n-source-strings
description: Export existing Infra Arcana i18n keys, English fallback strings, XML i18n_key entries, manual.txt chapter/paragraph entries, and data/messages/*.txt files as Paratranz-compatible CSV. Use when the user asks to scan all i18n-ed strings, extract English source text, split the Tome of Wisdom/manual into translation-platform entries, export XML-backed data strings, export message text files, build translation platform import files with index/source/translation columns, populate translation from an existing locale, or audit duplicate i18n keys.
---

# Export i18n source strings

Use this skill to extract the English source catalog from existing Infra Arcana
i18n calls in C++ source, XML elements with `i18n_key` attributes, the Tome of
Wisdom `manual.txt`, and `data/messages/*.txt`, then write
Paratranz-compatible CSV.

The bundled exporter handles both direct calls like
`i18n::get("key", "English fallback")` and the local insanity wrapper
`insanity_i18n::get("suffix", "English fallback")`. The wrapper form exports
as normal `insanity.*` keys because that is what `text.ini` stores. It also
scans `i18n::format("key", "English template with {placeholder}", args)`
calls; those templates are stored in `installed_files/data/locale/<locale>/grammar.ini`
rather than `text.ini`, so the exporter loads `grammar.ini` translations under
`section.key` keys (mirroring the C++ parser in `src/i18n.cpp`).

This is the inverse of raw-string extraction: it does not find untranslated
strings. Use `scan-i18n-raw-strings` for candidates still missing i18n and
`extract-i18n` to convert them.

The exporter also splits `installed_files/manual.txt` into stable manual keys:

- chapter titles: `manual.<chapter_slug>.title`
- blank-line-separated body blocks: `manual.<chapter_slug>.pNNN`

Manual body blocks preserve internal newlines for command tables, movement
diagrams, and bullet lists.

Message files under `installed_files/data/messages/*.txt` can be exported as
separate CSV files. The exporter follows the runtime loader: non-empty lines
whose first character is not a space and not `#` are message entries.

## Paratranz CSV

From the repository root:

```sh
python3 .claude/skills/export-i18n-source-strings/scripts/export_i18n_source_strings.py --output /tmp/ia-paratranz.csv
```

The CSV has no title/header row. Each row has three columns:

- `index`: the i18n key, e.g. `item_misc.horn.no_sound`.
- `source`: the English fallback string from the C++ `i18n::get` call.
- `translation`: blank by default, or filled from a locale file when requested.

For Paratranz compatibility, empty or whitespace-only source strings are emitted
as visible placeholders: `__EMPTY__` for an empty source and `__SPACE__` for a
whitespace-only source. The catalog key remains unchanged.

Useful variants:

```sh
# Fill the translation column from installed_files/data/locale/zh_CN/text.ini
# and installed_files/data/locale/zh_CN/manual.txt
python3 .claude/skills/export-i18n-source-strings/scripts/export_i18n_source_strings.py --locale zh_CN --output /tmp/ia-paratranz-zh_CN.csv

# Restrict to a file or subtree
python3 .claude/skills/export-i18n-source-strings/scripts/export_i18n_source_strings.py src/item_misc.cpp --output /tmp/item-misc.csv

# Export only manual entries
python3 .claude/skills/export-i18n-source-strings/scripts/export_i18n_source_strings.py --manual always installed_files/manual.txt --output /tmp/manual.csv

# Export an upload bundle: ia-paratranz-zh_CN.csv, manual.csv, and one CSV per message txt
python3 .claude/skills/export-i18n-source-strings/scripts/export_i18n_source_strings.py --locale zh_CN --output-dir /tmp/ia-paratranz

# Debug parsed entries as JSON
python3 .claude/skills/export-i18n-source-strings/scripts/export_i18n_source_strings.py --format json src/item_misc.cpp
```

`--output-dir` writes separate Paratranz CSV files:

- `ia-paratranz.csv`, or `ia-paratranz-<locale>.csv` when `--locale` is used:
  C++ `i18n::get`/wrapper entries and XML `i18n_key` entries intended for
  `text.ini`.
- `manual.csv`: `installed_files/manual.txt` chapter and paragraph entries.
- `<message-file-stem>.csv`: one CSV for each `installed_files/data/messages/*.txt`.

XML export follows the supplied path filters: the default full export scans
`installed_files/data/**/*.xml`; restricted exports scan XML only when a supplied
path is an XML file or a directory containing XML files.

Manual export defaults to `--manual auto`: include `manual.txt` in the default
full export, and include it for restricted exports only when a supplied path is
`installed_files/manual.txt` or a directory containing that file. Use
`--manual always` to force manual entries into a restricted export, or
`--manual never` to omit them.

## Review checks

After exporting, inspect the script diagnostics:

- duplicate key with same source: exported once, usually harmless.
- duplicate key with different source: fix before sending to translators.
- parse skipped calls: inspect manually; the script only exports calls whose
  first two arguments are string literals after any supported wrapper expansion.
- full `--output-dir --locale <locale>` exports fail if the main
  `ia-paratranz-<locale>.csv` catalog key set does not exactly match
  `installed_files/data/locale/<locale>/text.ini` plus `grammar.ini`.
  Stale `text.ini` keys that are no longer referenced in C++ (for example
  old fragment-based entries superseded by `i18n::format` templates) will
  appear as "missing from catalog"; remove them from `text.ini` to clear.
- missing locale translations when `--locale` is used: expected for newly added
  keys, but useful to review before upload.
- manual chapter/paragraph count mismatch when `--locale` is used: inspect the
  localized `manual.txt`; manual translations are paired to English keys by
  chapter order and paragraph order.
- message count mismatch when `--locale` is used: inspect the localized message
  file; message translations are paired by line order only when source and
  localized files have the same number of runtime-visible lines.

Keep `index` stable. Translators should edit only the `translation` column.

## Parser behavior

The bundled script is a small C++ token scanner, not a full compiler parser. It
handles multiline calls and escaped string literals, and it concatenates adjacent
C++ string literal tokens in the key or fallback argument. It also recognizes
the `insanity_i18n::get()` wrapper and exports those entries with the
`insanity.` prefix. It intentionally skips dynamic keys or dynamic fallbacks
because translation platforms need stable source text.

Manual parsing is delimiter-based: each chapter starts with an 80-character
dash line, followed by the title line and another 80-character dash line.
Paragraph entries are separated by blank lines until the next delimiter.
