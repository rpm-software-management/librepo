FILE(GLOB one_file_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.c")

FOREACH(file_path ${one_file_sources})
  GET_FILENAME_COMPONENT(filename "${file_path}" NAME_WLE)
  ADD_EXECUTABLE("${filename}" "${file_path}")
  TARGET_LINK_LIBRARIES("${filename}" librepo)
ENDFOREACH()
