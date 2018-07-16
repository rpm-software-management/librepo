ADD_CUSTOM_TARGET (doc-python
    PYTHONPATH=${CMAKE_BINARY_DIR}/librepo/python/python${PYTHON_VERSION_MAJOR} sphinx-build -E -b html
                  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Building Python API documentation with Sphinx")

