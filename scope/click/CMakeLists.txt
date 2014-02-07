SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)

add_library(${SCOPE_LIB_NAME} SHARED
  query.cpp
  scope.cpp
  qtscopebridge.cpp
  ubuntuone_credentials.cpp
  download-manager.cpp
  network_access_manager.cpp
  index.cpp
  webclient.cpp
)

configure_file(config.h.in config.h)

include_directories (${CMAKE_BINARY_DIR}/scope)

qt5_use_modules (${SCOPE_LIB_NAME} Network)

target_link_libraries (${SCOPE_LIB_NAME}
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
)
