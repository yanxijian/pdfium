# In-tree third_party targets for CMake MVP (AGG, lcms2, openjpeg).

include("${PDFIUM_CMAKE_DIR}/PdfiumSources.cmake")

function(pdfium_prepend_root out_var)
  set(_out)
  foreach(_f ${ARGN})
    list(APPEND _out "${PDFIUM_SOURCE_DIR}/${_f}")
  endforeach()
  set(${out_var} "${_out}" PARENT_SCOPE)
endfunction()

function(pdfium_third_party_compile_opts target)
  target_compile_features(${target} PRIVATE cxx_std_20)
  if(MSVC)
    target_compile_options(${target} PRIVATE /utf-8 "/FIwindows.h")
    target_compile_definitions(${target} PRIVATE NOMINMAX UNICODE _UNICODE)
  endif()
endfunction()

pdfium_prepend_root(_agg_sources ${PDFIUM_AGG_SOURCES})
add_library(pdfium_agg OBJECT ${_agg_sources})
target_include_directories(pdfium_agg PRIVATE
  "${PDFIUM_SOURCE_DIR}"
  "${PDFIUM_SHIM_INCLUDE_DIR}"
  "${PDFIUM_SOURCE_DIR}/third_party/agg23"
)
target_compile_definitions(pdfium_agg PRIVATE ${PDFIUM_PRIVATE_DEFINES})
target_compile_definitions(pdfium_agg PUBLIC ${PDFIUM_PUBLIC_DEFINES})
pdfium_third_party_compile_opts(pdfium_agg)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(pdfium_agg PRIVATE -Wno-unused-function)
endif()

pdfium_prepend_root(_lcms_sources ${PDFIUM_LCMS_SOURCES})
add_library(pdfium_lcms OBJECT ${_lcms_sources})
target_include_directories(pdfium_lcms PRIVATE
  "${PDFIUM_SOURCE_DIR}"
  "${PDFIUM_SHIM_INCLUDE_DIR}"
  "${PDFIUM_SOURCE_DIR}/third_party/lcms/include"
)
target_compile_definitions(pdfium_lcms PRIVATE ${PDFIUM_PRIVATE_DEFINES})
pdfium_third_party_compile_opts(pdfium_lcms)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(pdfium_lcms PRIVATE -Wno-missing-braces)
endif()

pdfium_prepend_root(_opj_sources ${PDFIUM_OPENJPEG_SOURCES})
add_library(pdfium_openjpeg OBJECT ${_opj_sources})
target_include_directories(pdfium_openjpeg PRIVATE
  "${PDFIUM_SOURCE_DIR}"
  "${PDFIUM_SHIM_INCLUDE_DIR}"
  "${PDFIUM_SOURCE_DIR}/third_party/libopenjpeg"
)
target_compile_definitions(pdfium_openjpeg PRIVATE ${PDFIUM_PRIVATE_DEFINES} OPJ_STATIC)
pdfium_third_party_compile_opts(pdfium_openjpeg)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(pdfium_openjpeg PRIVATE -Wno-implicit-int-float-conversion)
endif()

set(PDFIUM_THIRD_PARTY_OBJECTS
  $<TARGET_OBJECTS:pdfium_agg>
  $<TARGET_OBJECTS:pdfium_lcms>
  $<TARGET_OBJECTS:pdfium_openjpeg>
  PARENT_SCOPE
)

set(PDFIUM_THIRD_PARTY_TARGETS
  pdfium_agg
  pdfium_lcms
  pdfium_openjpeg
  PARENT_SCOPE
)
