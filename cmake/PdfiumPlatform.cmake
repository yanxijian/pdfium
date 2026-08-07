# Platform compile options — VolitionToolchain aligned (C++20 + MSVC /MD).

function(pdfium_apply_platform target)
  target_compile_features(${target} PUBLIC cxx_std_20)

  if(MSVC)
    set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY
      "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    target_compile_options(${target} PRIVATE /utf-8 /W3)
    target_compile_definitions(${target} PRIVATE
      NOMINMAX
      UNICODE
      _UNICODE
      _CRT_SECURE_NO_WARNINGS
    )
  else()
    target_compile_options(${target} PRIVATE
      -Wall
      -Wno-unused-parameter
    )
  endif()

  if(WIN32)
    target_link_libraries(${target} PUBLIC advapi32 gdi32 user32)
  elseif(APPLE)
    target_link_libraries(${target} PUBLIC
      "-framework AppKit"
      "-framework CoreFoundation"
      "-framework CoreGraphics"
    )
  elseif(UNIX)
    find_package(Fontconfig REQUIRED)
    target_link_libraries(${target} PUBLIC Fontconfig::Fontconfig)
  endif()
endfunction()
