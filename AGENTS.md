# Repository Guidelines

## Project Layout

- C++/CMake project for Infra Arcana.
- Public headers live in `include/`; implementation files live in `src/`.
- Tests use bundled Catch2 and live under `test/`, with test support in `test/include/` and `test/src/`.
- Third-party vendored code is under `third_party/`; avoid modifying it unless the task explicitly requires it.
- Runtime data and installed assets are under `installed_files/`.

## Build And Test Commands

- Configure and build the debug game target: `./build-debug.sh`
- Configure and build tests only: `./build-tests.sh`
- Build and run tests: `./run-tests.sh`
- Run the debug build: `./run-debug.sh`
- Run formatting over project C++ files: `./clang-format.sh`
- Run clang-tidy wrapper: `./clang-tidy.sh`
- Run cppcheck wrapper: `./cppcheck.sh`

The scripts create and use `build/`; treat it as generated output.

## Coding Style

- Follow `.clang-format`: 4-space indentation, no tabs, sorted includes, left pointer alignment, and project brace style.
- New C++ source/header files should include the existing copyright/SPDX header used by nearby files.
- Prefer existing local patterns and helpers over introducing new abstractions.
- Keep changes scoped; do not reformat unrelated files.
- If adding/removing source files, update `CMakeLists.txt` source lists as needed.

## Testing Notes

- Prefer `./run-tests.sh` for verification after behavior changes.
- For smaller edits, at least run the narrowest relevant build or static script when available.
- If SDL/system dependencies are missing and block a full build, report the exact command attempted and the missing dependency/error.

## Git Hygiene

- The working tree may contain user changes. Do not revert or overwrite unrelated changes.
- Do not commit generated build outputs, local binaries, or temporary artifacts.
