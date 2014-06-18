SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)
pkg_check_modules(JSON_CPP REQUIRED jsoncpp)

add_definitions(
  -DGETTEXT_PACKAGE=\"${PROJECT_NAME}\"
  -DGETTEXT_LOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LOCALEDIR}\"
)

add_library(${SCOPE_LIB_NAME} STATIC
  configuration.cpp
  download-manager.cpp
  index.cpp
  interface.cpp
  key_file_locator.cpp
  launcher.cpp
  network_access_manager.cpp
  package.cpp
  preview.cpp
  reviews.cpp
  qtbridge.cpp
  scope_activation.cpp
  smartconnect.cpp
  ubuntuone_credentials.cpp
  webclient.cpp
)

include_directories(
  ${JSON_CPP_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/libclickscope
)

target_link_libraries (${SCOPE_LIB_NAME}
  ${JSON_CPP_LDFLAGS}
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_CLIENT_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_COMMON_LDFLAGS}
  -lboost_locale
)
