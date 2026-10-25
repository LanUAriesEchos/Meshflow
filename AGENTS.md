# Meshflow - Agent Notes

Keyboard/mouse sharing app (Synergy/Deskflow fork). C++20, Qt6, CMake, vcpkg manifest mode.
Maintained by Rimora Studio. Targets Windows 10+ and Linux (X11/Wayland).

## Toolchain (this machine)

- Generator: Visual Studio 18 2026, x64
- MSVC: 14.51 (VS 2026 Community at `C:\Program Files\Microsoft Visual Studio\18\Community`)
- Qt: 6.8.3 for `msvc2022_64` at `C:\Qt\6.8.3\msvc2022_64`
  - `windeployqt.exe` lives in `C:\Qt\6.8.3\msvc2022_64\bin` and is NOT on PATH by default.
    Prefix PATH with that dir before configure/build, or `find_program(windeployqt)` fails.
- vcpkg: `C:\dev\vcpkg` (toolchain file at `C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake`)
  - Triplet: `x64-windows`
  - Baseline commit `d5ec528...` must be fetched into the vcpkg clone before first configure
    (shallow clone does not include it): `git -C C:\dev\vcpkg fetch --depth 1 origin d5ec528843d29e3a52d745a64b469f810b2cedbf`
- CMake: 4.4 at `C:\Program Files\CMake\bin\cmake.exe`
- Git: `C:\Program Files\Git\cmd\git.exe`

## Configure (first time or after vcpkg.json change)

```powershell
$env:PATH = 'C:\Qt\6.8.3\msvc2022_64\bin;' + $env:PATH
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
  -DQt6_DIR="C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6" `
  -DVCPKG_QT=OFF `
  -DCMAKE_TOOLCHAIN_FILE="C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET="x64-windows"
```

First run compiles OpenSSL from source (~13 min). Subsequent runs use the vcpkg binary cache and finish in seconds.

## Build

```powershell
$env:PATH = 'C:\Qt\6.8.3\msvc2022_64\bin;' + $env:PATH
cmake --build build --config Release --target meshflow -j      # GUI
cmake --build build --config Release --target meshflow-core -j # core lib/daemon
cmake --build build --config Release -j                       # everything
```

Output: `build/bin/Release/meshflow.exe` (Qt + OpenSSL DLLs staged next to it via windeployqt).

Target names on Windows: `meshflow` (GUI), `meshflow-core`, `meshflow-daemon`. On macOS the GUI target is `Meshflow`.

## Tests

```powershell
cmake --build build --config Release --target unittests -j
ctest --test-dir build --build-config Release --output-on-failure
```

Unit tests live in `src/unittests/` (GTest). Translations must build first or I18N tests fail (CMakeLists already orders this).

## Lint / format

- `clang-format` using repo-root `.clang-format` (run on changed files).
- Spelling: `cspell.json` at repo root.
- REUSE compliance: every source file carries SPDX headers; do not drop them. `REUSE.toml` governs licensing.

## Project layout

- `src/apps/meshflow-gui` - Qt GUI (target `meshflow` on Win, `Meshflow` on macOS)
- `src/apps/meshflow-core` - server/client core
- `src/apps/meshflow-daemon` - Windows background daemon
- `src/lib/meshflow` - shared protocol/IPC library
- `src/lib/{base,arch,net,io,mt,platform,server,client,common,gui}` - layered internals
  - `platform` = OS-specific input capture (Win32, X11, Wayland/libei)
- `src/unittests/` - per-module GTest suites
- `deploy/` - installer/packaging per platform
- `cmake/Libraries.cmake` - platform lib setup, OpenSSL find_package, MSVC flags
- `cmake/vcpkg.json.in` - generates `vcpkg.json` at configure time (adds Qt ports when `-DVCPKG_QT=ON`)

## Key CMake options

- `BUILD_INSTALLER` (ON) - build the installer/packaging target
- `ENABLE_COVERAGE` (OFF) - gcov coverage flags (Unix only)
- `VCPKG_QT` (OFF) - pull Qt from vcpkg instead of the system install
- `BUILD_OSX_BUNDLE` (ON, macOS) - produce .app bundle
- `BUILD_X11_SUPPORT` (ON, Linux) - X11 input backend

## Conventions

- C++20, `CMAKE_CXX_EXTENSIONS OFF` - use standard portable constructs.
- `QT_NO_KEYWORDS` is defined: use `Q_SIGNAL`/`Q_SLOT`/`Q_EMIT`, not `signals`/`slots`/`emit`.
- `CMAKE_AUTOMOC/AUTOUIC/AUTORCC` are ON.
- PascalCase for classes/hooks, camelCase for functions/variables (matches existing code).
- Do not add narrating comments; only comment non-obvious logic. Preserve existing SPDX headers.
