cmake_minimum_required(VERSION 3.20)
project(pdfium VERSION 0.0.0 LANGUAGES C CXX)

set(PDFIUM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PDFIUM_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${PDFIUM_CMAKE_DIR}")

include(GNUInstallDirs)
include("${PDFIUM_CMAKE_DIR}/PdfiumOptions.cmake")
include("${PDFIUM_CMAKE_DIR}/PdfiumDependencies.cmake")
include("${PDFIUM_CMAKE_DIR}/PdfiumPlatform.cmake")

# Third-party in-tree OBJECT libs
add_subdirectory(third_party)

# Core / fpdfsdk OBJECT libs
include("${PDFIUM_CMAKE_DIR}/PdfiumModules.cmake")

add_library(pdfium STATIC
  ${PDFIUM_THIRD_PARTY_OBJECTS}
  ${PDFIUM_MODULE_OBJECTS}
)
add_library(pdfium::pdfium ALIAS pdfium)

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

target_link_libraries(pdfium PUBLIC ${PDFIUM_EXTERNAL_LIBS})
pdfium_apply_platform(pdfium)

# Ensure OBJECT libs build before linking the archive on multi-config generators.
foreach(_t IN LISTS PDFIUM_THIRD_PARTY_TARGETS PDFIUM_MODULE_TARGETS)
  add_dependencies(pdfium ${_t})
endforeach()

if(PDFIUM_BUILD_SAMPLES)
  add_subdirectory(samples)
endif()

# ---- Install / export -------------------------------------------------------
install(TARGETS pdfium
  EXPORT pdfiumTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY public/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pdfium/public
  FILES_MATCHING
    PATTERN "*.h"
)

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
