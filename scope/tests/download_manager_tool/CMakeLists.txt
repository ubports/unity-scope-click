set(DOWNLOAD_MANAGER_TOOL_TARGET download_manager_tool)

add_executable(${DOWNLOAD_MANAGER_TOOL_TARGET}
  download_manager_tool.cpp download_manager_tool.h
)

target_link_libraries(${DOWNLOAD_MANAGER_TOOL_TARGET}
  -Wl,-rpath,${CMAKE_BINARY_DIR}/src
  -L${CMAKE_BINARY_DIR}/src
  ${SCOPE_LIB_NAME}
  )
