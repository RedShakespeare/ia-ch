---
name: export-i18n-source-strings
description: Export existing Infra Arcana i18n keys and English fallback strings from C++ source as Paratranz-compatible CSV. Use when the user asks to scan all i18n-ed strings, extract English source text, build translation platform import files with index/source/translation columns, populate translation from an existing locale, or audit duplicate i18n keys.
---

# Export i18n source strings

Use this skill to extract the English source catalog from existing Infra Arcana
i18n calls in C++ source and write a Paratranz-compatible CSV.

The bundled exporter handles both direct calls like
`i18n::get("key", "English fallback")` and the local insanity wrapper
`insanity_i18n::get("suffix", "English fallback")`. The wrapper form exports
as normal `insanity.*` keys because that is what `text.ini` stores.

This is the inverse of raw-string extraction: it does not find untranslated
strings. Use `scan-i18n-raw-strings` for candidates still missing i18n and
`extract-i18n` to convert them.

## Paratranz CSV

From the repository root:

```sh
python3 .codex/skills/export-i18n-source-strings/scripts/export_i18n_source_strings.py --output /tmp/ia-paratranz.csv
```

The CSV has no title/header row. Each row has three columns:

- `index`: the i18n key, e.g. `item_misc.horn.no_sound`.
- `source`: the English fallback string from the C++ `i18n::get` call.
- `translation`: blank by default, or filled from a locale file when requested.

Useful variants:

```sh
# Fill the translation column from installed_files/data/locale/zh_CN/text.ini
python3 .codex/skills/export-i18n-source-strings/scripts/export_i18n_source_strings.py --locale zh_CN --output /tmp/ia-paratranz-zh_CN.csv

# Restrict to a file or subtree
python3 .codex/skills/export-i18n-source-strings/scripts/export_i18n_source_strings.py src/item_misc.cpp --output /tmp/item-misc.csv

# Debug parsed entries as JSON
python3 .codex/skills/export-i18n-source-strings/scripts/export_i18n_source_strings.py --format json src/item_misc.cpp
```

## Review checks

After exporting, inspect the script diagnostics:

- duplicate key with same source: exported once, usually harmless.
- duplicate key with different source: fix before sending to translators.
- parse skipped calls: inspect manually; the script only exports calls whose
  first two arguments are string literals after any supported wrapper expansion.
- missing locale translations when `--locale` is used: expected for newly added
  keys, but useful to review before upload.

Keep `index` stable. Translators should edit only the `translation` column.

## Parser behavior

The bundled script is a small C++ token scanner, not a full compiler parser. It
handles multiline calls and escaped string literals, and it concatenates adjacent
C++ string literal tokens in the key or fallback argument. It also recognizes
the `insanity_i18n::get()` wrapper and exports those entries with the
`insanity.` prefix. It intentionally skips dynamic keys or dynamic fallbacks
because translation platforms need stable source text.
