# Sample programs for the CMake MVP build.

# Stage runtimes once (pdfium POST_BUILD from parent dir is awkward across
# add_subdirectory). Use a custom target so copies are not raced by samples.
add_custom_target(pdfium_sample_runtimes ALL DEPENDS pdfium)
if(PDFIUM_BUILD_SHARED)
  add_custom_command(TARGET pdfium_sample_runtimes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:pdfium>"
      "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:pdfium>"
    COMMENT "Stage pdfium runtime into samples/"
  )
endif()
if(PDFIUM_ENABLE_V8 AND PDFIUM_V8_RUNTIME_DLLS)
  foreach(_dll IN LISTS PDFIUM_V8_RUNTIME_DLLS)
    get_filename_component(_name "${_dll}" NAME)
    add_custom_command(TARGET pdfium_sample_runtimes POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_dll}"
        "${CMAKE_CURRENT_BINARY_DIR}/${_name}"
      COMMENT "Stage ${_name} into samples/"
    )
  endforeach()
endif()

add_executable(simple_no_v8 simple_no_v8.c)
target_link_libraries(simple_no_v8 PRIVATE pdfium)
target_include_directories(simple_no_v8 PRIVATE "${PDFIUM_SOURCE_DIR}")
add_dependencies(simple_no_v8 pdfium_sample_runtimes)

if(PDFIUM_ENABLE_V8)
  add_executable(simple_with_v8 simple_with_v8.cc)
  target_link_libraries(simple_with_v8 PRIVATE pdfium pdfium::v8)
  target_include_directories(simple_with_v8 PRIVATE "${PDFIUM_SOURCE_DIR}")
  target_compile_features(simple_with_v8 PRIVATE cxx_std_20)
  target_compile_definitions(simple_with_v8 PRIVATE NOMINMAX UNICODE _UNICODE)
  add_dependencies(simple_with_v8 pdfium_sample_runtimes)

  if(PDFIUM_V8_SNAPSHOT)
    add_custom_command(TARGET simple_with_v8 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PDFIUM_V8_SNAPSHOT}"
        "$<TARGET_FILE_DIR:simple_with_v8>/snapshot_blob.bin"
    )
  endif()
  if(PDFIUM_V8_ICUDTL)
    add_custom_command(TARGET simple_with_v8 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PDFIUM_V8_ICUDTL}"
        "$<TARGET_FILE_DIR:simple_with_v8>/icudtl.dat"
    )
  endif()
endif()
