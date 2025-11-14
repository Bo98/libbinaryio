cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(binaryio CXX)

add_library(binaryio INTERFACE)

file(GLOB_RECURSE PUBLIC_HEADERS CONFIGURE_DEPENDS "include/*.hpp")
target_sources(binaryio INTERFACE FILE_SET HEADERS
                                  BASE_DIRS include
                                  FILES "${PUBLIC_HEADERS}")

target_compile_features(binaryio INTERFACE cxx_std_20)

set_target_properties(binaryio PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(CMAKE_VERIFY_INTERFACE_HEADER_SETS AND PROJECT_IS_TOP_LEVEL)
    # These need to be public for *_verify_interface_header_sets to pick it up,
    # but we don't actually want to bleed these out to dependents normally.
    if(MSVC)
        target_compile_options(binaryio INTERFACE /W4)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|^GNU$|^Intel")
        target_compile_options(binaryio INTERFACE -Wall -Wextra -Wpedantic)
    endif()
endif()

install(TARGETS binaryio FILE_SET HEADERS)

include(GNUInstallDirs)
install(FILES LICENSE.md TYPE DOC RENAME copyright)

