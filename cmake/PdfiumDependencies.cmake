# Resolve third-party dependencies for pdfium CMake MVP.
# Codecs (AGG/lcms/openjpeg) are built from in-tree sources.
# Infrastructure packages use find_package; Abseil/fast_float may FetchContent.
#
# VolitionToolchain (product path): PDFIUM_ENABLE_V8=OFF + AbseilPin 20260107.1
# (MSVC STL /MD). V8 ON still FetchContents Abseil for libc++ ABI (non-product).

include(FetchContent)

set(PDFIUM_ABSEIL_PIN_PREFIX "" CACHE PATH
  "AbseilPin install prefix (…/prefix/20260107.1). Empty = find_package / FetchContent")

# Sibling default when unset (Codes/AbseilPin next to pdfium_all or pdfium).
if(NOT PDFIUM_ABSEIL_PIN_PREFIX OR PDFIUM_ABSEIL_PIN_PREFIX STREQUAL "")
  foreach(_cand
      "${CMAKE_SOURCE_DIR}/../AbseilPin/prefix/20260107.1"
      "${CMAKE_SOURCE_DIR}/../../AbseilPin/prefix/20260107.1")
    if(EXISTS "${_cand}/lib/cmake/absl/abslConfig.cmake")
      set(PDFIUM_ABSEIL_PIN_PREFIX "${_cand}" CACHE PATH
        "AbseilPin install prefix (…/prefix/20260107.1). Empty = find_package / FetchContent"
        FORCE)
      message(STATUS "PDFium: auto PDFIUM_ABSEIL_PIN_PREFIX=${PDFIUM_ABSEIL_PIN_PREFIX}")
      break()
    endif()
  endforeach()
endif()

if(PDFIUM_ABSEIL_PIN_PREFIX AND NOT PDFIUM_ABSEIL_PIN_PREFIX STREQUAL "")
  list(PREPEND CMAKE_PREFIX_PATH "${PDFIUM_ABSEIL_PIN_PREFIX}")
  # Force absl_DIR to the AbseilPin prefix ahead of any cached absl_DIR.
  set(absl_DIR "${PDFIUM_ABSEIL_PIN_PREFIX}/lib/cmake/absl" CACHE PATH
    "Abseil CONFIG dir when PDFIUM_ABSEIL_PIN_PREFIX is set" FORCE)
  message(STATUS "PDFium: absl_DIR=${absl_DIR}")
endif()

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(Freetype REQUIRED)
find_package(ICU COMPONENTS uc data REQUIRED)
find_package(harfbuzz CONFIG QUIET)
if(NOT harfbuzz_FOUND)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    # Core + subset (font subsetting APIs used by cpdf_fontsubsetter).
    pkg_check_modules(HARFBUZZ REQUIRED IMPORTED_TARGET harfbuzz)
    pkg_check_modules(HARFBUZZ_SUBSET QUIET IMPORTED_TARGET harfbuzz-subset)
  endif()
endif()

set(_pdfium_abseil_git_tag "20260107.1")

# Prefer CONFIG packages from vcpkg / AbseilPin; fall back to FetchContent.
# When V8 is enabled with Chromium libc++, force FetchContent Abseil so it is
# built with the same C++ ABI as pdfium/V8 (vcpkg / AbseilPin are MSVC STL).
if(PDFIUM_ENABLE_V8)
  set(CMAKE_DISABLE_FIND_PACKAGE_absl TRUE)
  message(STATUS "PDFIUM_ENABLE_V8: FetchContent abseil-cpp (match libc++ ABI)")
  FetchContent_Declare(
    abseil
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG ${_pdfium_abseil_git_tag}
    GIT_SHALLOW TRUE
  )
  set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
  set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(abseil)
else()
  find_package(absl CONFIG QUIET)
  if(NOT absl_FOUND)
    if(PDFIUM_ABSEIL_PIN_PREFIX AND NOT PDFIUM_ABSEIL_PIN_PREFIX STREQUAL "")
      message(FATAL_ERROR
        "PDFIUM_ABSEIL_PIN_PREFIX is set but find_package(absl CONFIG) failed.\n"
        "  Prefix: ${PDFIUM_ABSEIL_PIN_PREFIX}\n"
        "  Expected: ${PDFIUM_ABSEIL_PIN_PREFIX}/lib/cmake/absl/abslConfig.cmake")
    endif()
    message(STATUS "absl CONFIG not found; FetchContent google/abseil-cpp ${_pdfium_abseil_git_tag}")
    FetchContent_Declare(
      abseil
      GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
      GIT_TAG ${_pdfium_abseil_git_tag}
      GIT_SHALLOW TRUE
    )
    set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
    set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(abseil)
  else()
    if(PDFIUM_ABSEIL_PIN_PREFIX AND NOT PDFIUM_ABSEIL_PIN_PREFIX STREQUAL "")
      message(STATUS "PDFium: using AbseilPin absl from ${PDFIUM_ABSEIL_PIN_PREFIX}")
    else()
      message(STATUS "PDFium: using absl from package (vcpkg/system)")
    endif()
  endif()
endif()

find_package(FastFloat CONFIG QUIET)
if(NOT FastFloat_FOUND)
  find_path(FASTFLOAT_INCLUDE_DIR
    NAMES fast_float/fast_float.h
    PATH_SUFFIXES include
  )
endif()
if(NOT FastFloat_FOUND AND NOT FASTFLOAT_INCLUDE_DIR)
  message(STATUS "fast_float not found; FetchContent fastfloat/fast_float")
  FetchContent_Declare(
    fast_float
    GIT_REPOSITORY https://github.com/fastfloat/fast_float.git
    GIT_TAG v8.0.2
    GIT_SHALLOW TRUE
  )
  set(FASTFLOAT_TEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(fast_float)
  set(FASTFLOAT_INCLUDE_DIR "${fast_float_SOURCE_DIR}/include")
endif()

# Collect imported targets / include dirs into PDFIUM_EXTERNAL_LIBS.
set(PDFIUM_EXTERNAL_LIBS
  ZLIB::ZLIB
  JPEG::JPEG
  Freetype::Freetype
  ICU::uc
)

if(TARGET ICU::data)
  list(APPEND PDFIUM_EXTERNAL_LIBS ICU::data)
endif()

# Font subsetting (cpdf_fontsubsetter) needs hb_subset_* from harfbuzz-subset.
set(_pdfium_hb_subset_found FALSE)
if(TARGET harfbuzz::harfbuzz)
  list(APPEND PDFIUM_EXTERNAL_LIBS harfbuzz::harfbuzz)
  if(TARGET harfbuzz::harfbuzz-subset)
    list(APPEND PDFIUM_EXTERNAL_LIBS harfbuzz::harfbuzz-subset)
    set(_pdfium_hb_subset_found TRUE)
  endif()
elseif(TARGET PkgConfig::HARFBUZZ)
  list(APPEND PDFIUM_EXTERNAL_LIBS PkgConfig::HARFBUZZ)
  if(TARGET PkgConfig::HARFBUZZ_SUBSET)
    list(APPEND PDFIUM_EXTERNAL_LIBS PkgConfig::HARFBUZZ_SUBSET)
    set(_pdfium_hb_subset_found TRUE)
  endif()
else()
  message(FATAL_ERROR
    "HarfBuzz not found. Install harfbuzz (e.g. vcpkg install harfbuzz) "
    "or ensure pkg-config can find it.")
endif()
if(NOT _pdfium_hb_subset_found)
  find_library(PDFIUM_HARFBUZZ_SUBSET_LIB
    NAMES harfbuzz-subset libharfbuzz-subset
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
  )
  if(PDFIUM_HARFBUZZ_SUBSET_LIB)
    list(APPEND PDFIUM_EXTERNAL_LIBS "${PDFIUM_HARFBUZZ_SUBSET_LIB}")
    message(STATUS "PDFium: harfbuzz-subset via ${PDFIUM_HARFBUZZ_SUBSET_LIB}")
  else()
    # Last resort: plain -l form (works when the .so is on the default linker path).
    list(APPEND PDFIUM_EXTERNAL_LIBS harfbuzz-subset)
    message(STATUS "PDFium: harfbuzz-subset via -lharfbuzz-subset")
  endif()
endif()

if(TARGET absl::flat_hash_set)
  list(APPEND PDFIUM_EXTERNAL_LIBS
    absl::flat_hash_set
    absl::inlined_vector
    absl::cleanup
  )
elseif(TARGET absl::absl)
  list(APPEND PDFIUM_EXTERNAL_LIBS absl::absl)
else()
  # FetchContent abseil exposes fine-grained targets.
  list(APPEND PDFIUM_EXTERNAL_LIBS
    absl::flat_hash_set
    absl::inlined_vector
    absl::cleanup
  )
endif()

set(PDFIUM_SHIM_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include_shim")
set(PDFIUM_FASTFLOAT_INCLUDE_DIR "${FASTFLOAT_INCLUDE_DIR}")

if(PDFIUM_ENABLE_V8)
  include("${CMAKE_CURRENT_LIST_DIR}/FindPdfiumV8.cmake")
  list(APPEND PDFIUM_EXTERNAL_LIBS pdfium::v8)
endif()
