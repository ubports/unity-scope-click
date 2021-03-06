SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)
find_package (Qt5Sql REQUIRED)
pkg_check_modules(JSON_CPP REQUIRED jsoncpp)

add_definitions(
  -DGETTEXT_PACKAGE=\"${PROJECT_NAME}\"
  -DGETTEXT_LOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LOCALEDIR}\"
  -DSTORE_DATA_DIR="${STORE_DATA_DIR}"
)

add_library(${APPS_LIB_UNVERSIONED} SHARED
  apps-query.cpp
  apps-scope.cpp
)
set_target_properties(${APPS_LIB_UNVERSIONED} PROPERTIES PREFIX "")

include_directories(
  ${CMAKE_SOURCE_DIR}/libclickscope
  ${JSON_CPP_INCLUDE_DIRS}
)

qt5_use_modules (${APPS_LIB_UNVERSIONED} Sql)

target_link_libraries (${APPS_LIB_UNVERSIONED}
  ${SCOPE_LIB_NAME}
  ${JSON_CPP_LDFLAGS}
  ${UNITY_SCOPES_LDFLAGS}
)

install(
  TARGETS ${APPS_LIB_UNVERSIONED}
  LIBRARY DESTINATION "${APPS_LIB_DIR}"
  )

set(APPS_INI_TARGET clickscope.ini)
configure_file(
  ${APPS_INI_TARGET}.in.in
  ${APPS_INI_TARGET}.in
)

add_custom_target(${APPS_INI_TARGET} ALL
  COMMENT "Merging translations into ${APPS_INI_TARGET}"
  COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${CMAKE_SOURCE_DIR}/po ${APPS_INI_TARGET}.in ${APPS_INI_TARGET} >/dev/null
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${APPS_INI_TARGET}"
  DESTINATION "${APPS_LIB_DIR}"
)
