SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)
pkg_check_modules(JSON_CPP REQUIRED jsoncpp)
pkg_check_modules(LIBURL_DISPATCHER REQUIRED url-dispatcher-1)

add_definitions(
  -DGETTEXT_PACKAGE=\"${PROJECT_NAME}\"
  -DGETTEXT_LOCALEDIR=\"${CMAKE_INSTALL_LOCALEDIR}\"
)

add_library(${SCOPE_LIB_UNVERSIONED} SHARED
  departments.cpp
  department-lookup.cpp
  configuration.cpp
  download-manager.cpp
  index.cpp
  interface.cpp
  key_file_locator.cpp
  network_access_manager.cpp
  query.cpp
  reviews.cpp
  scope.cpp
  smartconnect.cpp
  preview.cpp
  ubuntuone_credentials.cpp
  webclient.cpp

  qtbridge.cpp
)

include_directories(
    ${JSON_CPP_INCLUDE_DIRS}
    ${LIBURL_DISPATCHER_INCLUDE_DIRS})

link_directories(${LIBURL_DISPATCHER_LIBRARY_DIRS})

qt5_use_modules (${SCOPE_LIB_UNVERSIONED} Network)

target_link_libraries (${SCOPE_LIB_UNVERSIONED}
  ${JSON_CPP_LDFLAGS}
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_CLIENT_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_COMMON_LDFLAGS}
  ${LIBURL_DISPATCHER_LIBRARIES}
)

install(
  TARGETS ${SCOPE_LIB_UNVERSIONED}
  LIBRARY DESTINATION "${SCOPE_LIB_DIR}"
  )
