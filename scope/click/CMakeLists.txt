SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)

add_library(
  clickscope SHARED

  query.cpp
  scope.cpp
  download-manager.cpp
  network_access_manager.cpp
  webclient.cpp)

configure_file(config.h.in config.h)

include_directories (${CMAKE_BINARY_DIR}/scope)

qt5_use_modules (clickscope Network)

set_target_properties(
  clickscope

  PROPERTIES
  VERSION ${SCOPE_CLICK_VERSION_MAJOR}.${SCOPE_CLICK_VERSION_MINOR}.${SCOPE_CLICK_VERSION_PATCH}
  SOVERSION ${SCOPE_CLICK_VERSION_MAJOR}
)

target_link_libraries (
  clickscope

  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
)

