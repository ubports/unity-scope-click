SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)
pkg_check_modules(JSON_CPP REQUIRED jsoncpp)

add_definitions(
  -DGETTEXT_PACKAGE=\"${PROJECT_NAME}\"
  -DGETTEXT_LOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LOCALEDIR}\"
)

add_library(${SCOPE_LIB_UNVERSIONED} SHARED
  download-manager.cpp
  index.cpp
  interface.cpp
  query.cpp
  reviews.cpp
  scope.cpp
  scope_activation.cpp
  package.cpp
  preview.cpp
  ubuntuone_credentials.cpp
  webclient.cpp

  qtbridge.cpp
)
set_target_properties(${SCOPE_LIB_UNVERSIONED} PROPERTIES PREFIX "")

include_directories(
  ${CMAKE_SOURCE_DIR}/libclickscope
  ${JSON_CPP_INCLUDE_DIRS}
)

qt5_use_modules (${SCOPE_LIB_UNVERSIONED} Network)

target_link_libraries (${SCOPE_LIB_UNVERSIONED}
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
