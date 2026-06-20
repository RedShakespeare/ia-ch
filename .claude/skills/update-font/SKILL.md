---
name: update-font
description: Regenerate Infra Arcana bitmap font atlases that have Chinese/CJK glyphs, preserving original cell size and CJK font settings.
---

# Update Font

Regenerate CJK-enabled Infra Arcana bitmap font PNG/JSON atlases. Do not touch ASCII-only fonts.

## Workflow

1. Check the tree first:
   `git status --short`
   Keep unrelated user changes intact.
2. Dry-run the update:
   `python3 tools/update-cjk-bitmap-fonts.py --dry-run`
3. If the dry-run reports the expected CJK-enabled targets and no missing fonts or glyphs, regenerate:
   `python3 tools/update-cjk-bitmap-fonts.py`
4. Validate:
   `python3 -m json.tool installed_files/gfx/fonts/<changed>.json`
   `python3 tools/test_generate_bitmap_font.py`
   Prefer `./run-tests.sh` after broad asset updates when dependencies allow it.
5. Inspect:
   `git status --short`
   `git diff --stat`
6. Commit only after requested checks pass. Use an `[assets]` commit prefix, for example:
   `git commit -m "[assets] Update CJK bitmap font atlases"`

## Hard Rules

- Update a PNG only when its paired `.json` already contains CJK codepoints.
- Leave ASCII-only fonts and ASCII-only JSON maps untouched.
- Preserve the original logical cell size from the filename and JSON.
- Derive the ASCII layout, CJK layout, font paths, font sizes, and font indexes from the paired JSON. Do not silently substitute a different CJK font.
- Use fitted rendering for CJK glyphs so oversized glyphs are scaled into the original CJK atlas slot recorded in the JSON.

## Script

Run `python3 tools/update-cjk-bitmap-fonts.py --help` for options. Useful flags:

- `--include-font GLOB`: limit to specific PNG names, repeatable.
- `--exclude-font GLOB`: skip specific PNG names, repeatable.
- `--font-dir PATH`: update a non-default font directory.
- `--extra-text TEXT`: include additional characters in the generated atlas.
- `--dry-run`: render to a temporary directory and report what would change.
