# Building PDFium with CMake (MVP)

This branch adds a parallel **CMake** build next to GN. The MVP profile is:

- AGG rasterizer (`PDF_USE_AGG`)
- Shared library `pdfium` by default (`PDFIUM_BUILD_SHARED=ON` → `pdfium.dll` / `.so`)
- Optional V8 Acrobat JS (`PDFIUM_ENABLE_V8`) against a **shared/component** V8 stamp
- No XFA, no Skia, no PartitionAlloc
- Samples: `simple_no_v8`, and `simple_with_v8` when V8 is on

GN / `DEPS` / `BUILD.gn` are unchanged.

## Dependencies

| Dependency | Strategy |
|------------|----------|
| AGG, lcms2, OpenJPEG | Built from `third_party/` |
| zlib, libjpeg, FreeType, ICU, HarfBuzz | `find_package` (system / vcpkg) |
| Abseil, fast_float | `find_package` if present, else FetchContent |
| V8 (optional) | Shared GN sidecar via `PDFIUM_V8_ROOT` (`bin/v8.dll` + import libs) |

### Windows (vcpkg + Clang-cl)

Prerequisites: Visual Studio C++ tools, LLVM (`clang-cl` on `PATH`), CMake, Ninja, and environment variable `VCPKG_ROOT` set to a vcpkg checkout.

```bat
vcpkg install zlib libjpeg-turbo freetype icu harfbuzz abseil --triplet x64-windows
rem Use an x64 Developer Command Prompt (or equivalent vcvars)
cmake -S . -B out/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DPDFIUM_ENABLE_V8=OFF
cmake --build out/cmake --target pdfium simple_no_v8
```

Windows **requires Clang-cl** (upstream PDFium sources use GCC/Clang builtins and inline asm). Stock MSVC `cl.exe` is not supported.

### Optional V8 (Acrobat JS, no XFA)

1. Build a **shared/component** V8 tree matching `DEPS` `v8_revision` (prefer the [pdfium_all](https://github.com/yanxijian/pdfium_all) scripts `fetch_v8` + `build_v8`). Expected stamp: `bin/v8.dll` (+ `v8_libbase.dll`, `v8_libplatform.dll`, often `libc++.dll`) and matching `lib/*.dll.lib`.
2. Set `PDFIUM_V8_ROOT` to that stamp (default under the meta-repo: `.tools/v8-out`) and configure CMake:

```bat
cmake -S . -B out/cmake-v8 -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DPDFIUM_ENABLE_V8=ON ^
  -DPDFIUM_V8_ROOT=%PDFIUM_V8_ROOT%
cmake --build out/cmake-v8 --target pdfium simple_with_v8
```

Consumer init order must follow `samples/simple_with_v8.cc` (V8 platform/isolate before `FPDF_InitLibraryWithConfig`). Prefer `FPDF_GetRecommendedV8Flags()` (`--jitless`). If the stamp includes `snapshot_blob.bin`, it is copied next to the sample.

The GN sidecar builds Chromium **libc++** (`use_custom_libcxx=true`). On Windows, CMake compiles PDFium against that stamped libc++ (and FetchContent Abseil) so ABI matches `v8_monolith`. Pointer-compression feature macros must match the stamp; prefer a stamped `v8-gn.h` + `V8_GN_HEADER` from `build_v8`.

Do **not** enable XFA/Skia/PartitionAlloc in CMake yet.

### Linux

```bash
# Debian/Ubuntu
sudo apt install build-essential cmake ninja-build \
  zlib1g-dev libjpeg-dev libfreetype6-dev libicu-dev libharfbuzz-dev \
  libfontconfig1-dev libabsl-dev

cmake -S . -B out/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DPDFIUM_ENABLE_V8=OFF
cmake --build out/cmake --target pdfium simple_no_v8
```

### macOS

```bash
brew install cmake ninja zlib jpeg freetype icu4c harfbuzz abseil
cmake -S . -B out/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix icu4c);$(brew --prefix)" \
  -DPDFIUM_ENABLE_V8=OFF
cmake --build out/cmake --target pdfium simple_no_v8
```

## Options

| Option | Default | Notes |
|--------|---------|-------|
| `PDFIUM_ENABLE_V8` | OFF | Acrobat JS; requires `PDFIUM_V8_ROOT` |
| `PDFIUM_V8_ROOT` | (empty) | Parent of `v8/include`, plus `bin/` + `lib/` stamp |
| `PDFIUM_BUILD_SHARED` | ON | Shared `pdfium` (`COMPONENT_BUILD` for `FPDF_EXPORT`) |
| `PDFIUM_ENABLE_XFA` | OFF | Not supported in CMake yet |
| `PDFIUM_USE_SKIA` | OFF | Not supported in CMake yet |
| `PDFIUM_USE_AGG` | ON | Required |
| `PDFIUM_BUILD_SAMPLES` | ON | Builds samples |

## Install

```bash
cmake --install out/cmake --prefix <prefix>
```

When `PDFIUM_ENABLE_V8=ON`, CMake skips `install(EXPORT)` (FetchContent Abseil is not export-friendly); the static `pdfium` library and public headers still install.

Consumers can then:

```cmake
find_package(pdfium CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE pdfium::pdfium)
# includes: #include "pdfium/public/fpdfview.h"  (or adjust include layout)
```

Installed headers live under `include/pdfium/public/`. For the same `#include "public/fpdfview.h"` layout as the source tree, add `include/pdfium` to the include path.
