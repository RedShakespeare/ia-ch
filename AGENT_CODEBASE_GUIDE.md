# Agent Codebase Guide

This file is a codebase orientation guide for future agents. Repository rules and
required commands stay in `AGENTS.md`; this file explains how the project is put
together and where changes usually belong.

## Project Shape

Infra Arcana is a C++ roguelike built with CMake and SDL2. Public headers are in
`include/`, implementations are in `src/`, tests are in `test/`, and runtime
assets live in `installed_files/`. The main build targets are:

- `ia`: release game executable.
- `ia-debug`: debug game executable used by `./run-debug.sh`.
- `ia-test`: Catch2 test executable used by `./run-tests.sh`.

`CMakeLists.txt` keeps the game source list explicit in `COMMON_SRC`/`SRC`.
Tests are globbed from `test/src/*.cpp` and `test/test_cases/src/*.cpp`. When
adding or removing game files, update `CMakeLists.txt`; when adding test files
under the existing test directories, CMake will pick them up after reconfigure.

## Runtime Flow

`src/main.cpp` is the executable entry point. It seeds RNG, initializes IO,
parses flags, initializes game-wide data, pushes `MainMenuState`, and enters
`states::run()`.

The state stack lives behind `include/state.hpp` and `src/state.cpp`. Screens,
menus, popups, gameplay, inventory, and modal views are `State` subclasses.
State methods are the main UI lifecycle hooks:

- `on_pushed()`, `on_start()`, `on_popped()` for lifecycle.
- `draw()` and `cycle_graphics()` for rendering.
- `update()` for input and game logic.
- `draw_overlayed()` when a state should render on top of lower states.

Initialization is split in `src/init.cpp`:

- `init::init_io()` sets up SDL/audio, paths, config, i18n, colors, panels, and
  audio.
- `init::init_game()` initializes save/messages/line/map-template systems.
- `init::init_session()` initializes per-run mutable systems such as actors,
  terrain, item data, map, player bonuses, spells, hints, smell, and bot state.
- `init::cleanup_session()` cleans per-run state.

## Major Systems

The code uses many namespace-style modules with module-level state rather than a
central application object. Look for `namespace <module>` in matching
`include/<module>.hpp` and `src/<module>.cpp` files.

- Actors: `include/actor.hpp`, `src/actor*.cpp`, `include/actor_data.hpp`,
  `src/actor_data.cpp`, and `src/actor_factory.cpp`. `actor::Actor` stores
  creature state, inventory, AI state, properties, and combat data.
- Items: `include/item.hpp`, `src/item*.cpp`, `include/item_data.hpp`,
  `src/item_data.cpp`, and `src/item_factory.cpp`. Item enum IDs and base data
  are code-defined, with subclasses/hooks for item-specific behavior.
- Terrain: `include/terrain.hpp`, `src/terrain*.cpp`,
  `include/terrain_data.hpp`, `src/terrain_data.cpp`, and
  `src/terrain_factory.cpp`. Terrain types are code-defined and often have
  specialized classes for interactions.
- Map and mapgen: `include/map.hpp`, `src/map.cpp`, `include/mapgen.hpp`, and
  `src/mapgen_*.cpp`. The map has global layer arrays in `map::g_*` for terrain,
  items, light, LOS, smell, memory, and blockers.
- Time and turns: `include/game_time.hpp`, `src/game_time.cpp`,
  `src/actor_std_turn.cpp`, and `src/actor_act.cpp`.
- Combat: `src/attack*.cpp`, `include/attack*.hpp`, `src/actor_hit.cpp`,
  `src/knockback.cpp`, and `src/explosion.cpp`.
- Player progression: `src/player_bon.cpp`, `src/player_spells.cpp`,
  `src/insanity.cpp`, and `src/game.cpp`.
- IO/rendering/input: `include/io.hpp`, `include/io_internal.hpp`,
  `src/io*.cpp`, `src/draw_*.cpp`, `src/panel.cpp`, and `src/gfx.cpp`.
- Text and localization: `include/i18n.hpp`, `src/i18n.cpp`,
  `include/text_format.hpp`, `src/text_format.cpp`, and message/data files
  under `installed_files/data/`.
- Persistence: `include/saving.hpp`, `src/saving.cpp`, and `save()/load()`
  functions on individual modules/classes.

## Data And Assets

`installed_files/` is copied into `build/` by CMake and installed beside the
executables. Important data locations:

- `installed_files/data/monsters.xml`: monster data loaded at runtime.
- `installed_files/data/colors/*.xml`: color definitions.
- `installed_files/data/map/rooms.txt`: room/map template input.
- `installed_files/data/messages/*.txt`: inscription and menu text pools.
- `installed_files/gfx/`: fonts and image assets.
- `installed_files/audio/`: OGG music, ambient loops, and sound effects.
- `installed_files/manual.txt`, `credits.txt`, `release_history.txt`: user text.

Data loading is not fully data-driven; many IDs, enums, and behavior hooks are
compiled into C++ code. When adding a new monster/item/terrain/audio asset, check
both the data file and the relevant enum/factory/data module.

## Testing Strategy

Tests use bundled Catch2 v2. `test/src/main.cpp` provides Catch2 main, and
`test/src/stubs.cpp` replaces SDL/rendering/audio-facing functions so logic tests
can run headlessly. Test helpers live in `test/include/test_utils.hpp` and
`test/src/test_utils.cpp`.

Prefer focused tests in `test/test_cases/src/` for deterministic logic:

- Geometry/pathing/light: `test_line_calc.cpp`, `test_fov.cpp`,
  `test_floodfill.cpp`, `test_light.cpp`.
- Combat/effects: `test_attack.cpp`, `test_hit_actor.cpp`,
  `test_knockback.cpp`, `test_explosion.cpp`, `test_throw_items.cpp`.
- Data/model behavior: `test_inventory.cpp`, `test_properties.cpp`,
  `test_item_curse.cpp`, `test_player_bon.cpp`.
- Text/save/utilities: `test_text*.cpp`, `test_saving.cpp`, `test_random.cpp`,
  `test_i18n.cpp`, `test_utf8.cpp`.

`./run-tests.sh` rebuilds `ia-test`, runs from `build/`, and invokes Catch2 with
`-D 3 --abort`. For debugging one area, pass Catch2 filters through the script.

## Common Change Patterns

- New source/header files: include the existing copyright/SPDX header, add the
  files to `COMMON_SRC` or `SRC` in `CMakeLists.txt`, and run at least the
  relevant build.
- New test case files: place them under `test/test_cases/src/`; CMake globbing
  will include them after configuration.
- New state/screen: derive from `State`, add a `StateId`, and follow nearby
  state classes such as inventory, manual, popup, or main menu states.
- Save format changes: update both save and load paths in the same order.
  `src/saving.cpp` calls module `save()/load()` functions sequentially; ordering
  is part of the save format contract.
- Data IDs: when adding enum values, check string-to-ID maps, factories, tests,
  and save/load implications.
- Map mutations: use the helpers in `map`, `terrain`, `item`, and actor modules
  rather than directly changing all `map::g_*` layers unless a nearby function
  already does that.
- Random behavior: use the local `rnd` helpers. Deterministic tests often seed
  explicitly.

## Pitfalls

- The code has substantial module-level global state. Tests that touch session
  state should initialize and clean up the relevant modules, or reuse existing
  test helpers.
- Ownership is often raw-pointer based. Check destructor and cleanup paths before
  moving objects between actors, map layers, inventories, and factories.
- `installed_files/` are copied during CMake configure/build. If runtime data
  changes appear ignored, rebuild or reconfigure so `build/` has a fresh copy.
- The game executable depends on SDL2, SDL2_image, and SDL2_mixer unless built
  with static bundled SDL. Headless tests avoid most SDL runtime needs through
  stubs, but linking still follows the CMake dependency setup.
- Do not edit `third_party/` unless the task is explicitly about vendored code.
- Avoid broad formatting. Use `./clang-format.sh` only when appropriate, and
  watch for unrelated churn.

