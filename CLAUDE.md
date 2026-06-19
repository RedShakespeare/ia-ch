# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Start here

Two agent-facing docs already exist and are the source of truth — read them before this file:

- **`AGENTS.md`** — build/test commands, coding style, git hygiene.
- **`AGENT_CODEBASE_GUIDE.md`** — the codebase map: engine loop, state stack, map/actor/turn model, the i18n layer, fonts, and testing layout.

This file only summarizes; when guidance changes, update those two so they don't drift.

## Project

Infra Arcana — an open-source Lovecraftian roguelike (turn-based, grid-based, single-player). C++17, built with CMake, links SDL2 and SDL2_image. No scripting layer; all game logic is compiled into a single native binary. GPLv3.

## Common commands

The repo drives everything through scripts at the root (they create and use `build/`, treated as generated output):

```
./build-debug.sh      # configure + build the debug game target
./build-tests.sh      # configure + build tests only
./run-tests.sh        # build and run the full test suite
./run-debug.sh        # run the debug build
./clang-format.sh     # format project C++ files
./clang-tidy.sh       # clang-tidy wrapper
./cppcheck.sh         # cppcheck wrapper
```

Tests use bundled Catch2 under `test/`; cases live in `test/test_cases/src/`. To run a narrow set, build tests and pass a Catch2 name/tag filter to the test binary the build produces, rather than running the whole suite. If a full build is blocked by missing SDL/system deps, report the exact command and missing dependency — tests can build with a narrower dependency set than the game.

## Architecture (big picture)

See `AGENT_CODEBASE_GUIDE.md` for detail. The essentials:

- **State stack** — states derive from `State` (`include/state.hpp`), managed as a stack in `src/state.cpp`; the top state gets `update()`/`draw()`. `src/init.cpp` owns startup/teardown, `src/main_menu.cpp` is the entry state, input is polled via `src/io.cpp`.
- **Map / actors / turns** — fixed grid of `Cell` (`src/map.cpp`); actors (`src/actor*.cpp`) run on a turn/energy system in `src/game_time.cpp`; player actions dispatch from `src/game_commands.cpp`; terrain in `src/terrain*.cpp`.
- **i18n layer (active work area on this branch)** — keyed strings fetched via `i18n::get` (`src/i18n.cpp`), loaded from `installed_files/data/locale/<locale>/text.ini`. `src/text_format.cpp` handles wrapping/alignment/width and must treat CJK glyphs as double-width; `src/io_text.cpp` bridges strings to bitmap-font atlas rendering; player messages flow through `src/msg_log.cpp`.

## Conventions specific to this repo

- Player-facing strings must be added to the locale `text.ini` and fetched via `i18n::get` — never hardcoded in source.
- When changing text wrapping/measurement, account for double-width CJK glyphs and verify with the text-formatting tests.
- Font-map files under `installed_files/data/fonts/` are large and machine-generated — regenerate via the scripts in `tools/`, don't hand-edit. The root `.deb` files are upstream font sources for atlas generation, not linked into the game.
- Follow `.clang-format` (4-space indent, no tabs, sorted includes, left pointer alignment). Keep changes scoped; don't reformat unrelated files. If adding/removing sources, update `CMakeLists.txt`.
- Commit messages start with a `[type]` prefix (`fix`, `assets`, `i18n`, `docs`, `test`, …).
