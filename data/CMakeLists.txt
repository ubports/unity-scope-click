add_schema(com.canonical.unity.clickscope.gschema.xml)

# upstart job for default departments db
set(DEPARTMENTS_UPSTART_JOB click-scope-departments-db.conf)
configure_file(${DEPARTMENTS_UPSTART_JOB}.in ${DEPARTMENTS_UPSTART_JOB} @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${DEPARTMENTS_UPSTART_JOB} DESTINATION ${CMAKE_INSTALL_DATADIR}/upstart/sessions)

install(
  FILES clickscope-screenshot.jpg apps-scope.png ubuntu-logo.png
  DESTINATION "${APPS_DATA_DIR}"
)

install(
  FILES departments.db
  DESTINATION "${APPS_DATA_DIR}"
)

install(
  PROGRAMS update_schema.sh
  DESTINATION "${APPS_DATA_DIR}"
)

