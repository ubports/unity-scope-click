set(DOWNLOAD_MANAGER_TOOL_TARGET download_manager_tool)

add_executable(
  ${DOWNLOAD_MANAGER_TOOL_TARGET}

  download_manager_tool.cpp
  download_manager_tool.h
)

target_link_libraries(
  ${DOWNLOAD_MANAGER_TOOL_TARGET}

  clickscope)

add_test(${DOWNLOAD_MANAGER_TOOL_TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${DOWNLOAD_MANAGER_TOOL_TARGET})
