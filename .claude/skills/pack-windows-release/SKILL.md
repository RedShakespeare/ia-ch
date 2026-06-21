---
name: pack-windows-release
description: Clean build directory, cross-compile Windows release, and package into build/target/ia.zip
---

# Pack Windows Release

Automates the complete process of cleaning, building, and packaging a Windows release of Infra Arcana using MinGW cross-compilation.

## Overview

This skill performs three steps:
1. **Clean**: Remove the entire `build/` directory
2. **Build**: Cross-compile for Windows using MinGW toolchain
3. **Package**: Create `build/target/ia.zip` containing the release

## Prerequisites

Install MinGW-w64 cross-compiler and build tools:

```bash
sudo apt-get install mingw-w64 cmake build-essential zip
```

## Full Process

Run from the repository root:

```bash
# 1. Clean the build directory
rm -rf build/

# 2. Cross-compile Windows release
./build-release-windows-cross-compile.sh

# 3. Package into zip
cd build/target
../../pack-windows-release.sh ia
```

## Output

The final release package is located at:
```
build/target/ia.zip
```

This zip file contains:
- `ia/ia.exe` — Windows executable
- `ia/data/` — Game data files
- `ia/gfx/` — Graphics and fonts
- `ia/audio/` — Music and sound effects
- `ia/manual.txt` — Game manual

## Verification

After running, verify the package:

```bash
# Check zip was created
ls -lh build/target/ia.zip

# List contents
unzip -l build/target/ia.zip | head -20

# Check executable exists
unzip -l build/target/ia.zip | grep "ia.exe"
```

## Individual Steps

### Clean Only

```bash
rm -rf build/
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

## Technical Details

- **Cross-compiler**: x86_64-w64-mingw32-g++
- **Architecture**: 64-bit Windows
- **Build type**: Release (optimized)
- **SDL linking**: Uses bundled prebuilt Windows SDL libraries from `third_party/SDL/`
- **Install prefix**: `build/target/`
- **Binary name**: `ia.exe`
