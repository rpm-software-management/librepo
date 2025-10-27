ADD_CUSTOM_TARGET (doc-python
    LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/librepo PYTHONPATH=${CMAKE_BINARY_DIR}/librepo/python sphinx-build -E -b html
                  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Building Python API documentation with Sphinx")

