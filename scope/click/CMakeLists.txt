SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)
pkg_check_modules(JSON_CPP REQUIRED jsoncpp)

add_definitions(
  -DGETTEXT_PACKAGE=\"${PROJECT_NAME}\"
  -DGETTEXT_LOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LOCALEDIR}\"
)

add_library(${SCOPE_LIB_UNVERSIONED} SHARED
  departments.cpp
  department-lookup.cpp
  configuration.cpp
  download-manager.cpp
  index.cpp
  interface.cpp
  network_access_manager.cpp
  query.cpp
  reviews.cpp
  scope.cpp
  scope_activation.cpp
  smartconnect.cpp
  package.cpp
  preview.cpp
  ubuntuone_credentials.cpp
  webclient.cpp

  qtbridge.cpp
)

include_directories(
  ${CMAKE_SOURCE_DIR}/libclickscope
  ${JSON_CPP_INCLUDE_DIRS}
)

qt5_use_modules (${SCOPE_LIB_UNVERSIONED} Network)

target_link_libraries (${SCOPE_LIB_UNVERSIONED}
  -L${CMAKE_BINARY_DIR}/libclickscope
  ${SCOPE_LIB_NAME}
  ${JSON_CPP_LDFLAGS}
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_CLIENT_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_COMMON_LDFLAGS}
)

install(
  TARGETS ${SCOPE_LIB_UNVERSIONED}
  LIBRARY DESTINATION "${SCOPE_LIB_DIR}"
  )
