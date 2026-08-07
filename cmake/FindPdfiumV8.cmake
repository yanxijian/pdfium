# Locate a V8 tree built for PDFium CMake (Acrobat JS only).
#
# Search order:
#   1. PDFIUM_V8_ROOT CMake/cache/env
#   2. V8_ROOT env
#
# Accepted layouts (prefer component / shared):
#   A) Stamped install (pdfium_all scripts/build_v8.*):
#        <root>/v8/include/...
#        <root>/lib/v8.dll.lib + v8_libbase.dll.lib + v8_libplatform.dll.lib
#        <root>/bin/v8.dll (+ v8_libbase.dll, v8_libplatform.dll, libc++.dll, ...)
#   B) Static monolith: lib/v8_monolith.(lib|a)
#
# Include path is the parent of `v8/` so `#include "v8/include/v8.h"` works.

function(_pdfium_v8_find_lib out_var)
  set(_names ${ARGN})
  set(_found "")
  foreach(_n IN LISTS _names)
    foreach(_d IN LISTS _PDFIUM_V8_LIB_DIRS)
      foreach(_ext lib a)
        set(_cand "${_d}/${_n}.${_ext}")
        if(EXISTS "${_cand}")
          set(_found "${_cand}")
          break()
        endif()
      endforeach()
      if(_found)
        break()
      endif()
    endforeach()
    if(_found)
      break()
    endif()
  endforeach()
  set(${out_var} "${_found}" PARENT_SCOPE)
endfunction()

if(NOT PDFIUM_ENABLE_V8)
  return()
endif()

if(NOT PDFIUM_V8_ROOT AND DEFINED ENV{PDFIUM_V8_ROOT})
  set(PDFIUM_V8_ROOT "$ENV{PDFIUM_V8_ROOT}")
endif()
if(NOT PDFIUM_V8_ROOT AND DEFINED ENV{V8_ROOT})
  set(PDFIUM_V8_ROOT "$ENV{V8_ROOT}")
endif()

if(NOT PDFIUM_V8_ROOT)
  message(FATAL_ERROR
    "PDFIUM_ENABLE_V8=ON but PDFIUM_V8_ROOT is not set.\n"
    "Build V8 with pdfium_all scripts (fetch_v8 / build_v8) or point "
    "PDFIUM_V8_ROOT at a stamped V8 product tree.")
endif()

get_filename_component(PDFIUM_V8_ROOT "${PDFIUM_V8_ROOT}" ABSOLUTE)

set(_v8_hdr "")
if(EXISTS "${PDFIUM_V8_ROOT}/v8/include/v8.h")
  set(_v8_include_root "${PDFIUM_V8_ROOT}")
  set(_v8_hdr "${PDFIUM_V8_ROOT}/v8/include/v8.h")
elseif(EXISTS "${PDFIUM_V8_ROOT}/include/v8.h")
  message(FATAL_ERROR
    "V8 headers found at ${PDFIUM_V8_ROOT}/include but PDFium expects "
    "`v8/include/v8.h` under PDFIUM_V8_ROOT. Point PDFIUM_V8_ROOT at the "
    "parent that contains a `v8/` directory (see scripts/build_v8.ps1).")
endif()

if(NOT _v8_hdr)
  message(FATAL_ERROR
    "v8/include/v8.h not found under PDFIUM_V8_ROOT=${PDFIUM_V8_ROOT}")
endif()

set(_PDFIUM_V8_LIB_DIRS
  "${PDFIUM_V8_ROOT}/lib"
  "${PDFIUM_V8_ROOT}/bin"
  "${PDFIUM_V8_ROOT}/out/Release"
  "${PDFIUM_V8_ROOT}/out/Release/obj"
  "${PDFIUM_V8_ROOT}/out.gn/x64.release"
  "${PDFIUM_V8_ROOT}/out.gn/x64.release/obj"
)

# Prefer shared/component import libs, then static split, then monolith.
_pdfium_v8_find_lib(_v8_lib "v8.dll" v8)
_pdfium_v8_find_lib(_v8_libbase "v8_libbase.dll" v8_libbase)
_pdfium_v8_find_lib(_v8_libplatform "v8_libplatform.dll" v8_libplatform)
_pdfium_v8_find_lib(_v8_libcxx "libc++.dll" "libc++" cxx)
_pdfium_v8_find_lib(_v8_monolith v8_monolith)

set(_v8_shared FALSE)
if(EXISTS "${PDFIUM_V8_ROOT}/bin/v8.dll" OR EXISTS "${PDFIUM_V8_ROOT}/bin/libv8.so"
   OR EXISTS "${PDFIUM_V8_ROOT}/bin/libv8.dylib")
  set(_v8_shared TRUE)
endif()
if(_v8_lib MATCHES "\\.dll\\.lib$" OR _v8_lib MATCHES "v8\\.dll")
  set(_v8_shared TRUE)
endif()

# Chromium libc++ headers (stamped by build_v8.*).
set(_libcxx_inc "")
set(_libcxx_cfg "")
if(EXISTS "${PDFIUM_V8_ROOT}/include/c++/__config")
  set(_libcxx_inc "${PDFIUM_V8_ROOT}/include/c++")
elseif(EXISTS "${PDFIUM_V8_ROOT}/v8/third_party/libc++/src/include/__config")
  set(_libcxx_inc "${PDFIUM_V8_ROOT}/v8/third_party/libc++/src/include")
endif()
foreach(_cand
    "${PDFIUM_V8_ROOT}/include/c++config"
    "${PDFIUM_V8_ROOT}/v8/buildtools/third_party/libc++"
    "${PDFIUM_V8_ROOT}/v8/out.gn/x64.release/gen/third_party/libc++/src/include"
    "${PDFIUM_V8_ROOT}/../v8-workspace/v8/buildtools/third_party/libc++")
  if(EXISTS "${_cand}/__config_site")
    set(_libcxx_cfg "${_cand}")
    break()
  endif()
endforeach()

# Runtime DLLs to stage next to samples / output/bin.
set(PDFIUM_V8_RUNTIME_DLLS "")
if(EXISTS "${PDFIUM_V8_ROOT}/bin")
  file(GLOB _v8_rt "${PDFIUM_V8_ROOT}/bin/*.dll" "${PDFIUM_V8_ROOT}/bin/*.so"
       "${PDFIUM_V8_ROOT}/bin/*.dylib")
  set(PDFIUM_V8_RUNTIME_DLLS ${_v8_rt})
endif()

if(NOT TARGET pdfium::v8)
  add_library(pdfium::v8 INTERFACE IMPORTED)
  set_target_properties(pdfium::v8 PROPERTIES IMPORTED_NO_SYSTEM TRUE)
  target_include_directories(pdfium::v8 INTERFACE
    "${_v8_include_root}"
    "${_v8_include_root}/v8/include"
  )
  if(_libcxx_inc)
    if(NOT _libcxx_cfg)
      message(FATAL_ERROR
        "V8 stamp uses Chromium libc++ but __config_site was not found under "
        "${PDFIUM_V8_ROOT}. Re-run scripts/build_v8.ps1 to stamp include/c++config.")
    endif()
    target_compile_definitions(pdfium::v8 INTERFACE
      _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
      _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_NONE
      _LIBCPP_INSTRUMENTED_WITH_ASAN=0
    )
    if(WIN32)
      target_compile_options(pdfium::v8 INTERFACE
        "SHELL:-I\"${_libcxx_cfg}\""
        "SHELL:-I\"${_libcxx_inc}\""
      )
      # Keep inline virtuals of V8_EXPORT classes local (dllimport class quirk).
      if(_v8_shared)
        target_compile_options(pdfium::v8 INTERFACE "/Zc:dllexportInlines-")
      endif()
    else()
      target_include_directories(pdfium::v8 INTERFACE
        "${_libcxx_cfg}"
        "${_libcxx_inc}"
      )
      target_compile_options(pdfium::v8 INTERFACE
        "SHELL:-Xclang -nostdinc++"
      )
    endif()
    set(PDFIUM_V8_USES_LIBCXX TRUE CACHE INTERNAL "")
  endif()

  set(_v8_link_libs)
  if(_v8_shared OR (_v8_lib AND _v8_libplatform AND NOT _v8_monolith))
    if(NOT _v8_lib OR NOT _v8_libplatform)
      message(FATAL_ERROR
        "Shared V8 stamp incomplete under ${PDFIUM_V8_ROOT}.\n"
        "Expected import libs for v8 + v8_libplatform (and usually v8_libbase).")
    endif()
    list(APPEND _v8_link_libs "${_v8_lib}" "${_v8_libplatform}")
    if(_v8_libbase)
      list(APPEND _v8_link_libs "${_v8_libbase}")
    endif()
    # Prefer static libc++ inside pdfium.dll (DISABLE_VISIBILITY). The V8
    # runtime still ships libc++.dll for its own use.
    _pdfium_v8_find_lib(_v8_libcxx_static "libc++")
    if(_v8_libcxx_static AND NOT _v8_libcxx_static MATCHES "\\.dll\\.lib$")
      list(APPEND _v8_link_libs "${_v8_libcxx_static}")
    elseif(_v8_libcxx)
      list(APPEND _v8_link_libs "${_v8_libcxx}")
    endif()
    target_compile_definitions(pdfium::v8 INTERFACE
      USING_V8_SHARED
      USING_V8_SHARED_PRIVATE
      USING_V8_PLATFORM_SHARED
      USING_V8_BASE_SHARED
    )
    set(PDFIUM_V8_IS_SHARED TRUE CACHE INTERNAL "")
  elseif(_v8_monolith)
    list(APPEND _v8_link_libs "${_v8_monolith}")
    if(_v8_libcxx)
      list(APPEND _v8_link_libs "${_v8_libcxx}")
    elseif(PDFIUM_V8_USES_LIBCXX)
      message(WARNING
        "V8 was built with libc++ but lib/libc++.lib is missing under "
        "${PDFIUM_V8_ROOT}. Re-run build_v8.ps1 to stamp it.")
    endif()
    set(PDFIUM_V8_IS_SHARED FALSE CACHE INTERNAL "")
  else()
    message(FATAL_ERROR
      "Could not find V8 libraries under ${PDFIUM_V8_ROOT}.\n"
      "Expected shared stamp (bin/v8.dll + lib/v8.dll.lib …) or lib/v8_monolith.")
  endif()

  target_link_libraries(pdfium::v8 INTERFACE ${_v8_link_libs})

  if(EXISTS "${PDFIUM_V8_ROOT}/v8/include/v8-gn.h")
    target_compile_definitions(pdfium::v8 INTERFACE V8_GN_HEADER)
  elseif(EXISTS "${PDFIUM_V8_ROOT}/include/v8-gn.h")
    target_include_directories(pdfium::v8 INTERFACE "${PDFIUM_V8_ROOT}/include")
    target_compile_definitions(pdfium::v8 INTERFACE V8_GN_HEADER)
  else()
    target_compile_definitions(pdfium::v8 INTERFACE
      V8_COMPRESS_POINTERS
      V8_COMPRESS_POINTERS_IN_SHARED_CAGE
      V8_31BIT_SMIS_ON_64BIT_ARCH
      V8_HAVE_TARGET_OS
    )
    if(WIN32)
      target_compile_definitions(pdfium::v8 INTERFACE V8_TARGET_OS_WIN)
    elseif(APPLE)
      target_compile_definitions(pdfium::v8 INTERFACE V8_TARGET_OS_MACOS)
    elseif(UNIX)
      target_compile_definitions(pdfium::v8 INTERFACE V8_TARGET_OS_LINUX)
    endif()
  endif()

  if(WIN32)
    target_link_libraries(pdfium::v8 INTERFACE winmm dbghelp)
  elseif(UNIX AND NOT APPLE)
    target_link_libraries(pdfium::v8 INTERFACE pthread dl)
  endif()
endif()

set(PDFIUM_V8_SNAPSHOT "")
foreach(_s
    "${PDFIUM_V8_ROOT}/bin/snapshot_blob.bin"
    "${PDFIUM_V8_ROOT}/snapshot_blob.bin"
    "${PDFIUM_V8_ROOT}/out/Release/snapshot_blob.bin"
    "${PDFIUM_V8_ROOT}/out.gn/x64.release/snapshot_blob.bin")
  if(EXISTS "${_s}")
    set(PDFIUM_V8_SNAPSHOT "${_s}")
    break()
  endif()
endforeach()

set(PDFIUM_V8_ICUDTL "")
foreach(_s
    "${PDFIUM_V8_ROOT}/bin/icudtl.dat"
    "${PDFIUM_V8_ROOT}/icudtl.dat"
    "${PDFIUM_V8_ROOT}/out/Release/icudtl.dat"
    "${PDFIUM_V8_ROOT}/out.gn/x64.release/icudtl.dat")
  if(EXISTS "${_s}")
    set(PDFIUM_V8_ICUDTL "${_s}")
    break()
  endif()
endforeach()

if(PDFIUM_V8_SNAPSHOT)
  target_compile_definitions(pdfium::v8 INTERFACE V8_USE_EXTERNAL_STARTUP_DATA)
endif()

message(STATUS "PDFium V8: root=${PDFIUM_V8_ROOT}")
if(PDFIUM_V8_IS_SHARED)
  message(STATUS "PDFium V8: shared ${_v8_lib} + ${_v8_libplatform}")
elseif(_v8_monolith)
  message(STATUS "PDFium V8: monolith ${_v8_monolith}")
endif()
if(PDFIUM_V8_SNAPSHOT)
  message(STATUS "PDFium V8: snapshot=${PDFIUM_V8_SNAPSHOT}")
endif()
