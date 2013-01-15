ADD_CUSTOM_TARGET (doc-html
                  PYTHONPATH=${CMAKE_BINARY_DIR}/librepo/python sphinx-build -b html
                  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Building html documentation")
ADD_CUSTOM_TARGET (doc)
ADD_DEPENDENCIES (doc doc-html)