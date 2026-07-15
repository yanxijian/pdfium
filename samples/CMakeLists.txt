# Sample programs for the CMake MVP build.

add_executable(simple_no_v8 simple_no_v8.c)
target_link_libraries(simple_no_v8 PRIVATE pdfium)
target_include_directories(simple_no_v8 PRIVATE "${PDFIUM_SOURCE_DIR}")
