# Pdfium CMake MVP options (aligned with pdfium.gni for the MVP profile)

option(PDFIUM_ENABLE_V8 "Enable V8 JavaScript" OFF)
option(PDFIUM_ENABLE_XFA "Enable XFA forms" OFF)
option(PDFIUM_USE_SKIA "Use Skia for graphics" OFF)
option(PDFIUM_USE_AGG "Use AGG for graphics" ON)
option(PDFIUM_USE_PARTITION_ALLOC "Use PartitionAlloc" OFF)
option(PDFIUM_ENABLE_BROTLI "Enable Brotli" OFF)
option(PDFIUM_BUILD_SAMPLES "Build sample programs" ON)

if(PDFIUM_ENABLE_V8 OR PDFIUM_ENABLE_XFA OR PDFIUM_USE_SKIA OR PDFIUM_USE_PARTITION_ALLOC)
  message(FATAL_ERROR
    "CMake MVP only supports PDFIUM_ENABLE_V8=OFF, PDFIUM_ENABLE_XFA=OFF, "
    "PDFIUM_USE_SKIA=OFF, PDFIUM_USE_PARTITION_ALLOC=OFF. "
    "Use the GN build for other configurations.")
endif()

if(NOT PDFIUM_USE_AGG)
  message(FATAL_ERROR "CMake MVP requires PDFIUM_USE_AGG=ON")
endif()

# Public feature defines consumed by headers / consumers.
set(PDFIUM_PUBLIC_DEFINES
  PDF_USE_AGG
)

# Private defines for building pdfium internals.
set(PDFIUM_PRIVATE_DEFINES
  FPDF_IMPLEMENTATION
  OPJ_STATIC
  USE_SYSTEM_ZLIB
  USE_SYSTEM_LIBJPEG
  USE_SYSTEM_ICUUC
  DEFINE_PS_TABLES_DATA
)
