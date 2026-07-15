# Pdfium CMake options (AGG MVP; optional V8 Acrobat JS, no XFA/Skia/PA)

option(PDFIUM_ENABLE_V8 "Enable V8 JavaScript (Acrobat JS; not XFA)" OFF)
option(PDFIUM_ENABLE_XFA "Enable XFA forms" OFF)
option(PDFIUM_USE_SKIA "Use Skia for graphics" OFF)
option(PDFIUM_USE_AGG "Use AGG for graphics" ON)
option(PDFIUM_USE_PARTITION_ALLOC "Use PartitionAlloc" OFF)
option(PDFIUM_ENABLE_BROTLI "Enable Brotli" OFF)
option(PDFIUM_BUILD_SAMPLES "Build sample programs" ON)
option(PDFIUM_BUILD_SHARED "Build pdfium as a shared library (DLL/SO)" ON)

# Path to a V8 product tree produced by pdfium_all scripts (or equivalent).
# Expected layout: parent of `v8/include`, plus `lib/` (or GN out) and optional snapshot.
set(PDFIUM_V8_ROOT "${PDFIUM_V8_ROOT}" CACHE PATH
  "Root for V8 headers/libs (parent of v8/, or stamped .tools/v8-out)")

if(PDFIUM_ENABLE_XFA OR PDFIUM_USE_SKIA OR PDFIUM_USE_PARTITION_ALLOC)
  message(FATAL_ERROR
    "CMake build does not support PDFIUM_ENABLE_XFA=ON, PDFIUM_USE_SKIA=ON, "
    "or PDFIUM_USE_PARTITION_ALLOC=ON yet. Use the GN build for those "
    "configurations (XFA also requires V8).")
endif()

if(PDFIUM_ENABLE_XFA AND NOT PDFIUM_ENABLE_V8)
  message(FATAL_ERROR "PDFIUM_ENABLE_XFA requires PDFIUM_ENABLE_V8=ON")
endif()

if(NOT PDFIUM_USE_AGG)
  message(FATAL_ERROR "CMake MVP requires PDFIUM_USE_AGG=ON")
endif()

# Public feature defines consumed by headers / consumers.
set(PDFIUM_PUBLIC_DEFINES
  PDF_USE_AGG
)
if(PDFIUM_ENABLE_V8)
  list(APPEND PDFIUM_PUBLIC_DEFINES PDF_ENABLE_V8)
endif()
# FPDF_EXPORT uses COMPONENT_BUILD for dllimport/dllexport (see public/fpdfview.h).
if(PDFIUM_BUILD_SHARED)
  list(APPEND PDFIUM_PUBLIC_DEFINES COMPONENT_BUILD)
endif()

# Private defines for building pdfium internals.
set(PDFIUM_PRIVATE_DEFINES
  FPDF_IMPLEMENTATION
  OPJ_STATIC
  USE_SYSTEM_ZLIB
  USE_SYSTEM_LIBJPEG
  USE_SYSTEM_ICUUC
  DEFINE_PS_TABLES_DATA
)
