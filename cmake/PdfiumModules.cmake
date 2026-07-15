# Assemble pdfium modules as OBJECT libraries, then one static library.

include("${PDFIUM_CMAKE_DIR}/PdfiumSources.cmake")

function(pdfium_add_object name)
  cmake_parse_arguments(ARG "" "" "SOURCES;DEFINES" ${ARGN})
  set(_abs_sources)
  foreach(_f IN LISTS ARG_SOURCES)
    list(APPEND _abs_sources "${PDFIUM_SOURCE_DIR}/${_f}")
  endforeach()
  add_library(${name} OBJECT ${_abs_sources})
  target_include_directories(${name} PRIVATE
    "${PDFIUM_SOURCE_DIR}"
    "${PDFIUM_SHIM_INCLUDE_DIR}"
    "${PDFIUM_FASTFLOAT_INCLUDE_DIR}"
    "${PDFIUM_SOURCE_DIR}/third_party/agg23"
    "${PDFIUM_SOURCE_DIR}/third_party/lcms/include"
    "${PDFIUM_SOURCE_DIR}/third_party/libopenjpeg"
  )
  target_compile_definitions(${name} PRIVATE
    ${PDFIUM_PRIVATE_DEFINES}
    ${ARG_DEFINES}
  )
  target_compile_definitions(${name} PUBLIC ${PDFIUM_PUBLIC_DEFINES})
  target_compile_features(${name} PRIVATE cxx_std_20)
  target_link_libraries(${name} PRIVATE ${PDFIUM_EXTERNAL_LIBS})
  if(MSVC)
    target_compile_options(${name} PRIVATE /utf-8 "/FIwindows.h")
    target_compile_definitions(${name} PRIVATE NOMINMAX UNICODE _UNICODE)
  endif()
endfunction()

set(_fxcrt_sources ${PDFIUM_FXCRT_SOURCES})
if(WIN32)
  list(APPEND _fxcrt_sources ${PDFIUM_FXCRT_WIN_SOURCES})
else()
  list(APPEND _fxcrt_sources ${PDFIUM_FXCRT_POSIX_SOURCES})
endif()
pdfium_add_object(pdfium_fxcrt SOURCES ${_fxcrt_sources})

pdfium_add_object(pdfium_fxcodec SOURCES ${PDFIUM_FXCODEC_SOURCES})

set(_fxge_sources ${PDFIUM_FXGE_SOURCES})
if(WIN32)
  list(APPEND _fxge_sources ${PDFIUM_FXGE_WIN_SOURCES})
elseif(APPLE)
  list(APPEND _fxge_sources ${PDFIUM_FXGE_MAC_SOURCES})
elseif(UNIX)
  list(APPEND _fxge_sources ${PDFIUM_FXGE_LINUX_SOURCES})
endif()
pdfium_add_object(pdfium_fxge SOURCES ${_fxge_sources})

pdfium_add_object(pdfium_fdrm SOURCES ${PDFIUM_FDRM_SOURCES})
pdfium_add_object(pdfium_cmaps SOURCES ${PDFIUM_CMAPS_SOURCES})
pdfium_add_object(pdfium_font SOURCES ${PDFIUM_FONT_SOURCES})
pdfium_add_object(pdfium_page SOURCES ${PDFIUM_PAGE_SOURCES})
pdfium_add_object(pdfium_parser SOURCES ${PDFIUM_PARSER_SOURCES})

set(_render_sources ${PDFIUM_RENDER_SOURCES})
if(WIN32)
  list(APPEND _render_sources ${PDFIUM_RENDER_WIN_SOURCES})
endif()
pdfium_add_object(pdfium_render SOURCES ${_render_sources})

pdfium_add_object(pdfium_edit SOURCES ${PDFIUM_EDIT_SOURCES})
pdfium_add_object(pdfium_fpdfdoc SOURCES ${PDFIUM_FPDFDOC_SOURCES})
pdfium_add_object(pdfium_fpdftext SOURCES ${PDFIUM_FPDFTEXT_SOURCES})
pdfium_add_object(pdfium_fpdfsdk SOURCES ${PDFIUM_FPDFSDK_SOURCES})
pdfium_add_object(pdfium_formfiller SOURCES ${PDFIUM_FORMFILLER_SOURCES})
pdfium_add_object(pdfium_pwl SOURCES ${PDFIUM_PWL_SOURCES})
pdfium_add_object(pdfium_fxjs SOURCES ${PDFIUM_FXJS_STUB_SOURCES})

set(PDFIUM_MODULE_OBJECTS
  $<TARGET_OBJECTS:pdfium_fxcrt>
  $<TARGET_OBJECTS:pdfium_fxcodec>
  $<TARGET_OBJECTS:pdfium_fxge>
  $<TARGET_OBJECTS:pdfium_fdrm>
  $<TARGET_OBJECTS:pdfium_cmaps>
  $<TARGET_OBJECTS:pdfium_font>
  $<TARGET_OBJECTS:pdfium_page>
  $<TARGET_OBJECTS:pdfium_parser>
  $<TARGET_OBJECTS:pdfium_render>
  $<TARGET_OBJECTS:pdfium_edit>
  $<TARGET_OBJECTS:pdfium_fpdfdoc>
  $<TARGET_OBJECTS:pdfium_fpdftext>
  $<TARGET_OBJECTS:pdfium_fpdfsdk>
  $<TARGET_OBJECTS:pdfium_formfiller>
  $<TARGET_OBJECTS:pdfium_pwl>
  $<TARGET_OBJECTS:pdfium_fxjs>
)

set(PDFIUM_MODULE_TARGETS
  pdfium_fxcrt
  pdfium_fxcodec
  pdfium_fxge
  pdfium_fdrm
  pdfium_cmaps
  pdfium_font
  pdfium_page
  pdfium_parser
  pdfium_render
  pdfium_edit
  pdfium_fpdfdoc
  pdfium_fpdftext
  pdfium_fpdfsdk
  pdfium_formfiller
  pdfium_pwl
  pdfium_fxjs
)
