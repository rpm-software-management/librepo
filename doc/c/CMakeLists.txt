find_package(Doxygen REQUIRED)
CONFIGURE_FILE("Doxyfile.in.in" "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in" @ONLY)
add_custom_target(doc-c
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    COMMENT "Building C API documentation with Doxygen" VERBATIM)
