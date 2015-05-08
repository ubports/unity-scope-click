SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)
find_package (Qt5Sql REQUIRED)
pkg_check_modules(JSON_CPP REQUIRED jsoncpp)
pkg_check_modules(GSETTINGS_QT REQUIRED gsettings-qt)
pkg_check_modules(LIBPAY REQUIRED pay-2)

add_definitions(
  -DGETTEXT_PACKAGE=\"${PROJECT_NAME}\"
  -DGETTEXT_LOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LOCALEDIR}\"
  -DCLICK_INSTALL_HELPER=\"${CMAKE_INSTALL_PREFIX}/lib/unity-scope-click/install-helper\"
  ${GSETTINGS_QT_CFLAGS} ${GSETTINGS_QT_OTHER}
)

add_library(${SCOPE_LIB_NAME} STATIC
  configuration.cpp
  download-manager.cpp
  department-lookup.cpp
  departments.cpp
  departments-db.cpp
  highlights.cpp
  index.cpp
  interface.cpp
  key_file_locator.cpp
  launcher.cpp
  network_access_manager.cpp
  package.cpp
  pay.cpp
  preview.cpp
  reviews.cpp
  qtbridge.cpp
  scope_activation.cpp
  smartconnect.cpp
  ubuntuone_credentials.cpp
  utils.cpp
  webclient.cpp
)

qt5_use_modules(${SCOPE_LIB_NAME} Sql)

include_directories(
  ${JSON_CPP_INCLUDE_DIRS}
  ${LIBPAY_INCLUDE_DIRS}
  ${GSETTINGS_QT_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/libclickscope
)

target_link_libraries (${SCOPE_LIB_NAME}
  ${JSON_CPP_LDFLAGS}
  ${LIBPAY_LDFLAGS}
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_CLIENT_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_COMMON_LDFLAGS}
  ${GSETTINGS_QT_LIBRARIES}
  -lboost_locale
)
