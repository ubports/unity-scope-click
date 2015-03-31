SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)
find_package (Qt5Sql REQUIRED)
pkg_check_modules(JSON_CPP REQUIRED jsoncpp)
pkg_check_modules(LIBPAY REQUIRED pay-2)

add_definitions(
  -DGETTEXT_PACKAGE=\"${PROJECT_NAME}\"
  -DGETTEXT_LOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LOCALEDIR}\"
)

add_library(${STORE_LIB_UNVERSIONED} SHARED
  pay.cpp
  store-query.cpp
  store-scope.cpp
)
set_target_properties(${STORE_LIB_UNVERSIONED} PROPERTIES PREFIX "")

include_directories(
  ${CMAKE_SOURCE_DIR}/libclickscope
  ${JSON_CPP_INCLUDE_DIRS}
  ${LIBPAY_INCLUDE_DIRS}
)

qt5_use_modules (${STORE_LIB_UNVERSIONED} Network Sql)

target_link_libraries (${STORE_LIB_UNVERSIONED}
  ${SCOPE_LIB_NAME}
  ${JSON_CPP_LDFLAGS}
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
  ${LIBPAY_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_CLIENT_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_COMMON_LDFLAGS}
)

install(
  TARGETS ${STORE_LIB_UNVERSIONED}
  LIBRARY DESTINATION "${STORE_LIB_DIR}"
  )

set(STORE_INI_TARGET com.canonical.scopes.clickstore.ini)
configure_file(
  ${STORE_INI_TARGET}.in.in
  ${STORE_INI_TARGET}.in
)

add_custom_target(${STORE_INI_TARGET} ALL
  COMMENT "Merging translations into ${STORE_INI_TARGET}"
  COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${CMAKE_SOURCE_DIR}/po ${STORE_INI_TARGET}.in ${STORE_INI_TARGET} >/dev/null
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${STORE_INI_TARGET}"
  DESTINATION "${STORE_LIB_DIR}"
)

