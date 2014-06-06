find_program(INTLTOOL_MERGE intltool-merge)
set(STORE_INI_TARGET com.canonical.scopes.clickstore.ini)
set(APPS_INI_TARGET clickscope.ini)

configure_file(
  ${STORE_INI_TARGET}.in.in
  ${STORE_INI_TARGET}.in
)

configure_file(
  ${APPS_INI_TARGET}.in.in
  ${APPS_INI_TARGET}.in
)

add_custom_target(${STORE_INI_TARGET} ALL
  COMMENT "Merging translations into ${STORE_INI_TARGET}"
  COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${CMAKE_SOURCE_DIR}/po ${STORE_INI_TARGET}.in ${STORE_INI_TARGET} >/dev/null
)

add_custom_target(${APPS_INI_TARGET} ALL
  COMMENT "Merging translations into ${APPS_INI_TARGET}"
  COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${CMAKE_SOURCE_DIR}/po ${APPS_INI_TARGET}.in ${APPS_INI_TARGET} >/dev/null
)

install(
  FILES clickscope-screenshot.jpg apps-scope.svg
  DESTINATION "${STORE_DATA_DIR}"
)

install(
  FILES clickscope-screenshot.jpg apps-scope.svg
  DESTINATION "${APPS_DATA_DIR}"
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${STORE_INI_TARGET}"
  DESTINATION "${STORE_LIB_DIR}"
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${APPS_INI_TARGET}"
  DESTINATION "${APPS_LIB_DIR}"
)
