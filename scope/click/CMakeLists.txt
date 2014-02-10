SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)

add_library(${SCOPE_LIB_NAME} SHARED
  download-manager.cpp
  index.cpp
  interface.cpp
  key_file_locator.cpp
  network_access_manager.cpp
  query.cpp
  scope.cpp
  ubuntuone_credentials.cpp
  webclient.cpp
)

configure_file(config.h.in config.h)

include_directories (${CMAKE_BINARY_DIR}/scope)

qt5_use_modules (${SCOPE_LIB_NAME} Network)

target_link_libraries (${SCOPE_LIB_NAME}
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
)
