set(DOWNLOAD_MANAGER_TOOL_TARGET download_manager_tool)

include_directories (
  ${CMAKE_SOURCE_DIR}/scope/click
)

add_executable (${DOWNLOAD_MANAGER_TOOL_TARGET}
  download_manager_tool.cpp
  download_manager_tool.h
)

target_link_libraries (${DOWNLOAD_MANAGER_TOOL_TARGET}
  ${STORE_LIB_UNVERSIONED}
)

if(enable_coverage)
  target_link_libraries(${DOWNLOAD_MANAGER_TOOL_TARGET} gcov)
endif()
