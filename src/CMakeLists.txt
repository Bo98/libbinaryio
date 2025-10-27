set(HEADER_DIR "${LIBBINARYIO_ROOT}/include")
file(GLOB_RECURSE PUBLIC_HEADERS CONFIGURE_DEPENDS "${HEADER_DIR}/*.hpp")
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "*.cpp")

add_library(libbinaryio STATIC ${SRC_FILES})

target_sources(libbinaryio PUBLIC FILE_SET HEADERS
                                  BASE_DIRS "${HEADER_DIR}"
                                  FILES "${PUBLIC_HEADERS}")

target_compile_features(libbinaryio PUBLIC cxx_std_20)

set_target_properties(libbinaryio PROPERTIES PREFIX ""
                                             POSITION_INDEPENDENT_CODE ON
                                             VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL})

if(MSVC)
    target_compile_options(libbinaryio PRIVATE "/W4" "/Zc:throwingNew" "/EHsc")
endif()

install(TARGETS libbinaryio DESTINATION lib FILE_SET HEADERS DESTINATION include)
install(FILES "${LIBBINARYIO_ROOT}/LICENSE.md" DESTINATION share/binaryio RENAME copyright)
