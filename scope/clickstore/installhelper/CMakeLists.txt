set(INSTALL_HELPER_TARGET install_helper)

include_directories (
  ${CMAKE_SOURCE_DIR}/scope/click
)

add_executable (${INSTALL_HELPER_TARGET}
  install_helper.cpp
  install_helper.h
)

target_link_libraries (${INSTALL_HELPER_TARGET}
  ${STORE_LIB_UNVERSIONED}
)
