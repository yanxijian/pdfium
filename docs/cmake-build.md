# Building PDFium with CMake (MVP)

This branch adds a parallel **CMake** build next to GN. The MVP profile is:

- AGG rasterizer (`PDF_USE_AGG`)
- No V8, no XFA, no Skia, no PartitionAlloc
- Static library `pdfium`
- Sample: `simple_no_v8`

GN / `DEPS` / `BUILD.gn` are unchanged.

## Dependencies

| Dependency | Strategy |
|------------|----------|
| AGG, lcms2, OpenJPEG | Built from `third_party/` |
| zlib, libjpeg, FreeType, ICU, HarfBuzz | `find_package` (system / vcpkg) |
| Abseil, fast_float | `find_package` if present, else FetchContent |

### Windows (vcpkg + Clang-cl)

```bat
vcpkg install zlib libjpeg-turbo freetype icu harfbuzz abseil
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
set PATH=C:\Program Files\LLVM\bin;%PATH%
cmake -S . -B out/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build out/cmake --target pdfium simple_no_v8
```

Windows **requires Clang-cl** (upstream PDFium sources use GCC/Clang builtins and inline asm). Stock MSVC `cl.exe` is not supported.

### Linux

```bash
# Debian/Ubuntu
sudo apt install build-essential cmake ninja-build \
  zlib1g-dev libjpeg-dev libfreetype6-dev libicu-dev libharfbuzz-dev \
  libfontconfig1-dev libabsl-dev

cmake -S . -B out/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out/cmake --target pdfium simple_no_v8
```

### macOS

```bash
brew install cmake ninja zlib jpeg freetype icu4c harfbuzz abseil
cmake -S . -B out/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix icu4c);$(brew --prefix)"
cmake --build out/cmake --target pdfium simple_no_v8
```

## Options

| Option | Default | Notes |
|--------|---------|-------|
| `PDFIUM_ENABLE_V8` | OFF | Must stay OFF in MVP |
| `PDFIUM_ENABLE_XFA` | OFF | Must stay OFF in MVP |
| `PDFIUM_USE_SKIA` | OFF | Must stay OFF in MVP |
| `PDFIUM_USE_AGG` | ON | Required |
| `PDFIUM_BUILD_SAMPLES` | ON | Builds `simple_no_v8` |

## Install

```bash
cmake --install out/cmake --prefix /path/to/prefix
```

Consumers can then:

```cmake
find_package(pdfium CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE pdfium::pdfium)
# includes: #include "pdfium/public/fpdfview.h"  (or adjust include layout)
```

Installed headers live under `include/pdfium/public/`. For the same `#include "public/fpdfview.h"` layout as the source tree, add `include/pdfium` to the include path.
