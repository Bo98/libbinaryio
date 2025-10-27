set(HEADER_DIR ${LIBBINARYIO_ROOT}/include/binaryio)
set(PUBLIC_HEADERS ${HEADER_DIR}/binaryreader.hpp
                   ${HEADER_DIR}/binarywriter.hpp
                   ${HEADER_DIR}/util.hpp)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "*.hpp"
    "*.cpp"
)

add_library(libbinaryio STATIC ${SRC_FILES}
                               ${PUBLIC_HEADERS})

set_property(TARGET libbinaryio PROPERTY CXX_STANDARD 20)
set_property(TARGET libbinaryio PROPERTY PREFIX "")
target_include_directories(libbinaryio PUBLIC ${LIBBINARYIO_ROOT}/include)

if(MSVC)
    target_compile_options(libbinaryio PRIVATE "/W3" "/Zc:throwingNew" "/EHsc")
endif()

set_property(TARGET libbinaryio PROPERTY POSITION_INDEPENDENT_CODE ON)

install(TARGETS libbinaryio DESTINATION lib)
install(FILES ${PUBLIC_HEADERS} DESTINATION include/binaryio)
install(FILES ${LIBBINARYIO_ROOT}/LICENSE.md DESTINATION share/binaryio RENAME copyright)
