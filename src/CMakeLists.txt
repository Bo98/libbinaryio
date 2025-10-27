set(HEADER_DIR "${BINARYIO_ROOT}/include")
file(GLOB_RECURSE PUBLIC_HEADERS CONFIGURE_DEPENDS "${HEADER_DIR}/*.hpp")
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "*.cpp")

add_library(binaryio STATIC ${SRC_FILES})

target_sources(binaryio PUBLIC FILE_SET HEADERS
                                  BASE_DIRS "${HEADER_DIR}"
                                  FILES "${PUBLIC_HEADERS}")

target_compile_features(binaryio PUBLIC cxx_std_20)

set_target_properties(binaryio PROPERTIES POSITION_INDEPENDENT_CODE ON
                                             VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL})

if(MSVC)
    target_compile_options(binaryio PRIVATE "/W4" "/Zc:throwingNew" "/EHsc")
endif()

install(TARGETS binaryio DESTINATION lib FILE_SET HEADERS DESTINATION include)
install(FILES "${BINARYIO_ROOT}/LICENSE.md" DESTINATION share/binaryio RENAME copyright)
