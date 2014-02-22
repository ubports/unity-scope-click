set(CLICK_INTERFACE_TOOL_TARGET click_interface_tool)

include_directories (
  ${CMAKE_SOURCE_DIR}/scope/click
  ${CMAKE_BINARY_DIR}/scope/click
)

add_executable (${CLICK_INTERFACE_TOOL_TARGET}
  click_interface_tool.cpp
)

target_link_libraries (${CLICK_INTERFACE_TOOL_TARGET}
  ${SCOPE_LIB_UNVERSIONED}
)
