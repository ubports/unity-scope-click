find_program(INTLTOOL_MERGE intltool-merge)
set(SCOPE_INI_TARGET com.canonical.scopes.click.ini)

configure_file(
  ${SCOPE_INI_TARGET}.in.in
  ${SCOPE_INI_TARGET}.in
)

add_custom_target(${SCOPE_INI_TARGET} ALL
  COMMENT "Merging translations into ${SCOPE_INI_TARGET}"
  COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${CMAKE_SOURCE_DIR}/po ${SCOPE_INI_TARGET}.in ${SCOPE_INI_TARGET} >/dev/null
)

install(
  FILES clickscope-screenshot.jpg apps-scope.svg
  DESTINATION "${SCOPE_DATA_DIR}"
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${SCOPE_INI_TARGET}"
  DESTINATION "${SCOPE_LIB_DIR}"
)
