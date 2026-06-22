---
name: paratranz-csv-to-text-ini
description: Convert Paratranz-compatible CSV files with index/source/translation columns into Infra Arcana locale text.ini files. Use when Codex needs to import translator-edited Paratranz CSV back into installed_files/data/locale locale text.ini files, preserve existing text.ini ordering/comments, validate matching key sets, or regenerate a locale text.ini without hand-editing.
---

# Paratranz CSV to text.ini

## Workflow

Use the bundled deterministic script:

```sh
python3 .codex/skills/paratranz-csv-to-text-ini/scripts/paratranz_csv_to_text_ini.py \
  tools/i18n/ia-paratranz-text-zh_CN.csv \
  --template installed_files/data/locale/zh_CN/text.ini \
  --output installed_files/data/locale/zh_CN/text.ini
```

Default behavior:

- Read Paratranz CSV rows as `index,source,translation`; a header row is optional.
- Use the `translation` column as the `text.ini` value.
- Fail on duplicate CSV indexes.
- Fail on blank translations unless `--blank-translation source` or `--blank-translation empty` is provided.
- With `--template`, preserve existing non-entry lines, key order, comments, and section layout.
- With `--template`, fail if CSV and template key sets differ unless the explicit allow flags are used.
- With `--template`, preserve existing leading/trailing spaces for fragment values when the CSV cell has been trimmed. Use `--no-preserve-template-edge-whitespace` to disable that.

## Checks

After conversion, run:

```sh
python3 .codex/skills/paratranz-csv-to-text-ini/scripts/paratranz_csv_to_text_ini.py \
  tools/i18n/ia-paratranz-text-zh_CN.csv \
  --template installed_files/data/locale/zh_CN/text.ini \
  --output /tmp/text.ini.check
```

Then compare or replace as needed. For committed locale changes in this repo, run the relevant i18n/test hook before staging if the git hook requests it.
