---
name: pack-windows-release
description: Clean the generated build directory, cross-compile an Infra Arcana Windows release with MinGW, package build/target/ia.zip, verify the archive, and keep generated outputs out of commits.
---

# Pack Windows Release

Automates the complete process of cleaning, building, packaging, and verifying
a Windows release of Infra Arcana using MinGW cross-compilation.

## Overview

This skill performs four steps:
1. **Clean**: Remove the generated `build/` directory
2. **Build**: Cross-compile for Windows using the MinGW toolchain
3. **Package**: Create `build/target/ia.zip` containing the release
4. **Verify**: Confirm the archive exists and contains `ia/ia.exe`

## Prerequisites

Install MinGW-w64 cross-compiler and build tools:

```bash
sudo apt-get install mingw-w64 cmake build-essential zip
```

## Full Workflow

Run from the repository root:

```bash
# 1. Check for unrelated work first
git status --short

# 2. Clean the generated build directory
rm -rf build

# 3. Cross-compile Windows release
./build-release-windows-cross-compile.sh

# 4. Package into zip
cd build/target
../../pack-windows-release.sh ia

# 5. Verify the package
ls -lh ia.zip
unzip -l ia.zip | head -20
unzip -l ia.zip | grep "ia/ia.exe"

# 6. Return to the repository root and confirm only expected generated files changed
cd ../..
git status --short
```

## Output

The final release package is located at:
```
build/target/ia.zip
```

This zip file contains:
- `ia/ia.exe` - Windows executable
- `ia/data/` - Game data files
- `ia/gfx/` - Graphics and fonts
- `ia/audio/` - Music and sound effects
- `ia/manual.txt` - Game manual

## Verification Only

After packaging, verify the package from `build/target/`:

```bash
# Check zip was created
ls -lh ia.zip

# List contents
unzip -l ia.zip | head -20

# Check executable exists
unzip -l ia.zip | grep "ia/ia.exe"
```

## Individual Steps

### Clean Only

```bash
rm -rf build
```

### Build Only (without clean)

```bash
./build-release-windows-cross-compile.sh
```

### Package Only (after build)

```bash
cd build/target
../../pack-windows-release.sh ia
```

## Git Hygiene

- Run `git status --short` before cleaning so unrelated user changes are known.
- Treat `build/` and `build/target/ia.zip` as generated output. Do not commit
  them.
- If the user asks to commit skill or script changes related to this workflow,
  commit only those source files with a `[docs]`, `[build]`, or similarly scoped
  prefix.

## Troubleshooting

### MinGW not found

```
Error: CMAKE_C_COMPILER not found
```

Install the cross-compiler:
```bash
sudo apt-get install mingw-w64
```

### Toolchain file not found

```
Error: Could not find toolchain file
```

The script expects `Toolchain-cross-mingw32.txt` in the repository root. Verify it exists:
```bash
ls -la Toolchain-cross-mingw32.txt
```

### Build fails with SDL errors

The Windows build uses bundled SDL libraries, not system SDL. If you see SDL-related errors, check that `third_party/SDL/` contains the Windows prebuilt libraries.

### Zip command not found

```bash
sudo apt-get install zip
```

### Archive missing `ia/ia.exe`

Re-run the build step and check for install failures before packaging:

```bash
./build-release-windows-cross-compile.sh
ls -lh build/target/ia
```

## Technical Details

- **Cross-compiler**: x86_64-w64-mingw32-g++
- **Architecture**: 64-bit Windows
- **Build type**: Release (optimized)
- **SDL linking**: Uses bundled prebuilt Windows SDL libraries from `third_party/SDL/`
- **Install prefix**: `build/target/`
- **Binary name**: `ia.exe`
