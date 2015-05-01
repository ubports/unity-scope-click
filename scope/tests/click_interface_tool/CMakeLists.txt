set(CLICK_INTERFACE_TOOL_TARGET click_interface_tool)

include_directories (
  ${CMAKE_SOURCE_DIR}/libclickscope
  ${CMAKE_SOURCE_DIR}/scope
)

add_executable (${CLICK_INTERFACE_TOOL_TARGET}
  click_interface_tool.cpp
)

target_link_libraries (${CLICK_INTERFACE_TOOL_TARGET}
  ${STORE_LIB_UNVERSIONED}
)

if(enable_coverage)
  target_link_libraries(${CLICK_INTERFACE_TOOL_TARGET} gcov)
endif()
