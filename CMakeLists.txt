cmake_minimum_required(VERSION 3.20)
project(pdfium VERSION 0.0.0 LANGUAGES C CXX)

# VolitionToolchain: match MPS / Volition product shell (C++20, /MD).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

set(PDFIUM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PDFIUM_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${PDFIUM_CMAKE_DIR}")

include(GNUInstallDirs)
include("${PDFIUM_CMAKE_DIR}/PdfiumOptions.cmake")
include("${PDFIUM_CMAKE_DIR}/PdfiumDependencies.cmake")
include("${PDFIUM_CMAKE_DIR}/PdfiumPlatform.cmake")

# OBJECT libs must be PIC when linked into libpdfium.so (set before OBJECT targets).
if(PDFIUM_BUILD_SHARED)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Third-party in-tree OBJECT libs
add_subdirectory(third_party)

# Core / fpdfsdk OBJECT libs
include("${PDFIUM_CMAKE_DIR}/PdfiumModules.cmake")

if(PDFIUM_BUILD_SHARED)
  add_library(pdfium SHARED
    ${PDFIUM_THIRD_PARTY_OBJECTS}
    ${PDFIUM_MODULE_OBJECTS}
  )
else()
  add_library(pdfium STATIC
    ${PDFIUM_THIRD_PARTY_OBJECTS}
    ${PDFIUM_MODULE_OBJECTS}
  )
endif()
add_library(pdfium::pdfium ALIAS pdfium)
set_target_properties(pdfium PROPERTIES OUTPUT_NAME pdfium)

target_include_directories(pdfium
  PUBLIC
    $<BUILD_INTERFACE:${PDFIUM_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    "${PDFIUM_SHIM_INCLUDE_DIR}"
    "${PDFIUM_FASTFLOAT_INCLUDE_DIR}"
    "${PDFIUM_SOURCE_DIR}/third_party/agg23"
    "${PDFIUM_SOURCE_DIR}/third_party/lcms/include"
    "${PDFIUM_SOURCE_DIR}/third_party/libopenjpeg"
)

target_compile_definitions(pdfium
  PUBLIC ${PDFIUM_PUBLIC_DEFINES}
  PRIVATE ${PDFIUM_PRIVATE_DEFINES}
)

# Separate exportable deps from FetchContent Abseil / V8 (runtime next to exe).
set(_pdfium_public_libs)
set(_pdfium_private_libs)
foreach(_lib IN LISTS PDFIUM_EXTERNAL_LIBS)
  if(_lib MATCHES "^absl::" OR _lib STREQUAL "pdfium::v8")
    list(APPEND _pdfium_private_libs ${_lib})
  else()
    list(APPEND _pdfium_public_libs ${_lib})
  endif()
endforeach()
target_link_libraries(pdfium PUBLIC ${_pdfium_public_libs})
if(_pdfium_private_libs)
  target_link_libraries(pdfium PRIVATE ${_pdfium_private_libs})
endif()
pdfium_apply_platform(pdfium)

# Ensure OBJECT libs build before linking the archive on multi-config generators.
foreach(_t IN LISTS PDFIUM_THIRD_PARTY_TARGETS PDFIUM_MODULE_TARGETS)
  add_dependencies(pdfium ${_t})
endforeach()

if(PDFIUM_BUILD_SAMPLES)
  add_subdirectory(samples)
endif()

# ---- Install / export -------------------------------------------------------
if(PDFIUM_ENABLE_V8)
  install(TARGETS pdfium
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
else()
  install(TARGETS pdfium
    EXPORT pdfiumTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

install(DIRECTORY public/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pdfium/public
  FILES_MATCHING
    PATTERN "*.h"
)

# FetchContent Abseil (used with V8/libc++) cannot participate in install(EXPORT).
if(NOT PDFIUM_ENABLE_V8)
  install(EXPORT pdfiumTargets
    FILE pdfiumTargets.cmake
    NAMESPACE pdfium::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pdfium
  )

  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "${PDFIUM_CMAKE_DIR}/pdfiumConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pdfiumConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pdfium
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/pdfiumConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pdfiumConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/pdfiumConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pdfium
  )
endif()
