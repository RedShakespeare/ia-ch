# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Start here

Two agent-facing docs already exist and are the source of truth — read them before this file:

- **`AGENTS.md`** — build/test commands, coding style, git hygiene.
- **`AGENT_CODEBASE_GUIDE.md`** — the codebase map: engine loop, state stack, map/actor/turn model, the i18n layer, fonts, and testing layout.

This file only summarizes; when guidance changes, update those two so they don't drift.

## Project

Infra Arcana — an open-source Lovecraftian roguelike (turn-based, grid-based, single-player). C++17, built with CMake, links SDL2, SDL2_image, and SDL2_mixer. No scripting layer; all game logic is compiled into a single native binary. GPLv3.

GitLab repository: https://gitlab.com/martin-tornqvist/ia

## Common commands

The repo drives everything through scripts at the root (they create and use `build/`, treated as generated output):

```bash
./build-debug.sh      # configure + build the debug game target
./build-tests.sh      # configure + build tests only
./run-tests.sh        # build and run the full test suite
./run-debug.sh        # run the debug build
./clang-format.sh     # format project C++ files
./clang-tidy.sh       # clang-tidy wrapper
./cppcheck.sh         # cppcheck wrapper
```

Additional scripts for specialized builds and testing:

```bash
./build-sanitize.sh   # build with address/undefined-behavior sanitizers
./run-sanitize.sh     # run debug build with sanitizers
./run-bot.sh          # run automated bot play (--bot flag)
./run-stress-test.sh  # run stress test (--stress-test flag)
./run-gdb.sh          # run debug build under gdb
./valgrind.sh         # run under valgrind with suppressions
```

**Manual builds (for Linux artifacts):** Use `build-linux-tests/` as the build directory instead of `build/` when manually running cmake commands:

```bash
cmake -B build-linux-tests
cmake --build build-linux-tests --target ia-test -- -j$(nproc)
cd build-linux-tests && ./ia-test   # run tests
```

Tests use bundled Catch2 v2 under `test/`; cases live in `test/test_cases/src/`. To run a narrow set, build tests first and then pass Catch2 name/tag filters directly to `build/ia-test`:

```bash
./build-tests.sh
cd build
./ia-test "test_i18n"           # run specific test
./ia-test "[text]"              # run tests with tag
./ia-test -D 3 --abort "foo"    # verbose, abort on first failure
```

If a full build is blocked by missing SDL/system deps, report the exact command and missing dependency — tests can build with a narrower dependency set than the game.

## Architecture (big picture)

See `AGENT_CODEBASE_GUIDE.md` for detail. The essentials:

- **State stack** — states derive from `State` (`include/state.hpp`), managed as a stack in `src/state.cpp`; the top state gets `update()`/`draw()`. `src/init.cpp` owns startup/teardown, `src/main_menu.cpp` is the entry state, input is polled via `src/io.cpp`. States have lifecycle hooks: `on_pushed()`, `on_start()`, `on_popped()`, `draw()`, and `update()`.
- **Map / actors / turns** — fixed grid of `Cell` (`src/map.cpp`); actors (`src/actor*.cpp`) run on a turn/energy system in `src/game_time.cpp`; player actions dispatch from `src/game_commands.cpp`; terrain in `src/terrain*.cpp`.
- **i18n layer (active work area on this branch)** — keyed strings fetched via `i18n::get(key, fallback)` (`src/i18n.cpp`), loaded from `installed_files/data/locale/<locale>/text.ini`. Format: `[text]` section with `key=value` pairs. `src/text_format.cpp` handles wrapping/alignment/width and must treat CJK glyphs as double-width; `src/io_text.cpp` bridges strings to bitmap-font atlas rendering; player messages flow through `src/msg_log.cpp`.
- **Audio** — SDL2_mixer used in `src/audio.cpp` for music (OGG) and sound effects; assets in `installed_files/audio/`.
- **Data-driven elements** — `installed_files/data/monsters.xml` for monster definitions, `installed_files/data/map/rooms.txt` for room templates, `installed_files/data/messages/*.txt` for inscription pools.

## Conventions specific to this repo

- **i18n strings** — Player-facing strings must be added to `installed_files/data/locale/zh_CN/text.ini` (Chinese) or the en_US equivalent, and fetched via `i18n::get(key, fallback)` — never hardcoded in source. The fallback string serves as English text when translation is missing. Use the `/scan-i18n-raw-strings` and `/extract-i18n` skills to find and extract hardcoded strings.
- **Text formatting** — When changing text wrapping/measurement in `src/text_format.cpp`, account for double-width CJK glyphs (Chinese characters take 2 cell widths). Verify changes with the text-formatting tests (`test/test_cases/src/test_text*.cpp`).
- **Bitmap fonts** — Font atlas files under `installed_files/data/fonts/` (PNG + metadata) are large and machine-generated. Regenerate CJK-enabled fonts via `tools/update-cjk-bitmap-fonts.py` or the `/update-font` skill — don't hand-edit atlas files. The root `.deb` files (`fonts-hack_*.deb`, `fonts-noto-cjk_*.deb`) are upstream font sources for atlas generation, not linked into the game.
- **Code style** — Follow `.clang-format` (4-space indent, no tabs, sorted includes, left pointer alignment, AlwaysBreak after open bracket). Keep changes scoped; don't reformat unrelated files. Run `./clang-format.sh` only when appropriate. New files need the existing copyright/SPDX header (AGPL-3.0-or-later).
- **CMake source lists** — If adding/removing C++ source files, update the explicit `COMMON_SRC` or `SRC` lists in `CMakeLists.txt`. Test files under `test/test_cases/src/*.cpp` are globbed automatically.
- **Commit messages** — Start with a `[type]` prefix (`fix`, `assets`, `i18n`, `docs`, `test`, …). See memory file for details.
